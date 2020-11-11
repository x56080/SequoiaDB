/*******************************************************************************

   Copyright (C) 2011-2020 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

#include "coordCommandRestore.hpp"

#include <string>

#include "catDef.hpp"
#include "coordCommandBase.hpp"
#include "coordContext.hpp"
#include "coordFactory.hpp"
#include "coordTransOperator.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossMemPool.hpp"
#include "ossTypes.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnQueryOptions.hpp"

using bson::BSONElement;
using bson::BSONObj;
using std::exception;
using std::string;

namespace
{

INT32 getElementFromBSON(const BSONObj &input, const string &fieldName,
                         BSONElement *output)
{
   INT32 rc = SDB_OK;
   *output = input.getField(fieldName);
   if (output->eoo())
   {
      // Field not found
      return (rc = SDB_FIELD_NOT_EXIST);
   }
   return rc;
}

// The following two functions share a lot of code. When using c++11 this can be
// simplified using function pointers for the verification part.

// From a BSONObj input, get the value of the field,
// it should be a long int, and store it in output as a UINT64
INT32 globalTimeFromBSON(const BSONObj &input, const string &fieldName,
                         BOOLEAN required, UINT64 *output)
{
   INT32 rc = SDB_OK;
   BSONElement field;
   if ((rc = getElementFromBSON(input, fieldName, &field)))
   {
      if (!required)
      {
         // Field was not found but is not required
         return (rc = SDB_OK);
      }
      return rc;
   }
   // Non-number values will return 0
   INT64 value = field.numberLong();
   if (0 >= value)
   {
      // Field is a negative number or 0 or not a number
      PD_LOG(PDERROR, "Invalid global time (%i)", value);
      return (rc = SDB_INVALIDARG);
   }
   *output = (UINT64)value;
   return rc;
}

// From a BSONObj input, get the value of the field, it should be a boolean
INT32 boolFromBSON(const BSONObj &input, const string &fieldName,
                   BOOLEAN required, BOOLEAN *output)
{
   INT32 rc = SDB_OK;
   BSONElement field;
   if ((rc = getElementFromBSON(input, fieldName, &field)))
   {
      if (!required)
      {
         // Field was not found but is not required
         return (rc = SDB_OK);
      }
      return rc;
   }
   if (!field.isBoolean())
   {
      PD_LOG(PDERROR, "Invalid value for %s", fieldName.c_str());
      return (rc = SDB_INVALIDARG);
   }
   *output = field.boolean();
   return rc;
}

// Extract the query from a message object
INT32 extractQuery(MsgHeader *pMsg, BSONObj *query)
{
   INT32 rc = SDB_OK;
   CHAR *pQuery = NULL; // pointer to the query buffer
   if ((rc = msgExtractQuery((CHAR *)pMsg, NULL, NULL, NULL, NULL, &pQuery,
                             NULL, NULL, NULL)))
   {
      return rc;
   }
   // Using an existing buffer, does not copy, so no need to try/catch
   query->init(pQuery);
   return rc;
}

// Given a context push the results into a vector
INT32 gatherQueryResults(engine::pmdEDUCB *cb,
                         engine::rtnContextCoord *pContext,
                         std::vector<bson::BSONObj> *results)
{
   INT32 rc = SDB_OK;
   engine::rtnContextBuf buf;
   while (pContext)
   {
      rc = pContext->getMore(1, buf, cb);
      if (SDB_DMS_EOC == rc)
      {
         // End of results
         rc = SDB_OK;
         break;
      }
      else if (rc)
      {
         PD_LOG(PDERROR, "Get more results failed");
         return rc;
      }
      try
      {
         results->push_back(BSONObj(buf.data()).copy());
      }
      catch (exception &e)
      {
         PD_LOG(PDERROR, "Copying results failed");
         return (rc = SDB_OOM);
      }
   }
   return rc;
}

// Class to build a basic query message. Handles the release of the buffer upon
// destruction instead of leaving the responsibility to the caller.
class _QueryMsg
{
   engine::pmdEDUCB *_cb;
   CHAR *_buff;
   INT32 _size;

   // Release the buffer!
   void _release()
   {
      if (NULL != _buff)
      {
         msgReleaseBuffer(_buff, _cb);
         _buff = NULL;
      }
      header = NULL;
   };

 public:
   MsgHeader *header;

   // Constructor uses a pointer to store the rc.
   // If the build fails it releases the buffer.
   _QueryMsg(INT32 *rc, engine::pmdEDUCB *cb, const string &clName,
             MSG_TYPE opCode, const BSONObj &query)
       : _cb(cb), _buff(NULL), _size(0), header(NULL)
   {
      if ((*rc = msgBuildQueryMsg(&_buff, &_size, clName.c_str(), 0, 0, 0, -1,
                                  &query, NULL, NULL, NULL, cb)))
      {
         _release();
         PD_LOG(PDERROR, "Msg build failed");
         return;
      }
      header = (MsgHeader *)_buff;
      header->opCode = opCode;
   };

   ~_QueryMsg() { _release(); };
};

// Class to hold and auto-cleanup context.
// Use its ptr in place of a pContext.
class _Context
{
   engine::pmdEDUCB *_cb;

 public:
   engine::rtnContextCoord *ptr;
   explicit _Context(engine::pmdEDUCB *cb) : _cb(cb), ptr(NULL){};
   ~_Context()
   {
      if (ptr)
      {
         engine::pmdGetKRCB()->getRTNCB()->contextDelete(ptr->contextID(), _cb);
      }
   };
};

// Class to hold and auto-cleanup a coordOperator.
// Use its ptr in place of a pOperator.
class _Operator
{
 public:
   engine::coordOperator *ptr;

 private:
   void _release()
   {
      engine::coordGetFactory()->release(ptr);
      ptr = NULL;
   };

 public:
   _Operator(INT32 *rc, const string &cmdName) : ptr(NULL)
   {
      if ((*rc = engine::coordGetFactory()->create(cmdName.c_str(), ptr)))
      {
         _release();
         PD_LOG(PDERROR, "Failed to create operator");
         return;
      }
   };
   ~_Operator() { _release(); };
};

} // namespace

namespace engine
{

/*
   _coordCMDRestore definitions
*/

// Check the status of the cluster
INT32 _coordCMDRestore::_checkRestoreInProgress(BOOLEAN *inProgress)
{
   INT32 rc = SDB_OK;
   // Check the local cache
   *inProgress = pmdGetKRCB()->isDBRestoring();
   // Check the DC
   BSONObj document;
   if ((rc = _queryCataDCBase(&document)))
   {
      PD_LOG(PDERROR, "Failed to get [%s] status from catalog",
             FIELD_NAME_RESTORING);
      return rc; // System error
   }
   BSONElement field = document.getField(FIELD_NAME_RESTORING);
   if (field.eoo())
   {
      PD_LOG(PDERROR, "Missing field [%s] in document from catalog",
             FIELD_NAME_RESTORING);
      return (rc = SDB_SYS);
   }
   if (*inProgress != field.trueValue())
   {
      PD_LOG(PDERROR, "[%s] status mismatch between DC and cache",
             FIELD_NAME_RESTORING);
      return (rc = SDB_SYS);
   }
   return rc;
}

// Update the catalog DC RestoreInProgress value
// @param   enable   Whether to enable or diable the state
INT32 _coordCMDRestore::_setRestoreInProgress(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   BSONObj query;
   PD_LOG(PDINFO, "Setting cluster state [%s] = [%d]", FIELD_NAME_RESTORING,
          enable);
   // Update the DC
   try
   {
      if (enable)
      {
         query = BSON(FIELD_NAME_ACTION << CMD_VALUE_NAME_ENABLE_RESTORING);
      }
      else
      {
         query = BSON(FIELD_NAME_ACTION << CMD_VALUE_NAME_DISABLE_RESTORING);
      }
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return (rc = SDB_OOM);
   }
   if ((rc = _alterDC(query))) // this will update cata and data
   {
      PD_LOG(PDERROR, "Failed to update DC state across nodes");
      return rc;
   }
   // Update the coords
   if ((rc = _setRestoreInProgressCoords(enable)))
   {
      return rc;
   }
   return rc;
}

INT32 _coordCMDRestore::_setRestoreInProgressCoords(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   const string command =
       enable ? CMD_NAME_PREPARE_FLASHBACK : CMD_NAME_RESTORE_ABORT;
   if ((rc = _cmdCoords(MSG_BS_QUERY_REQ, CMD_ADMIN_PREFIX + command,
                        BSONObj())))
   {
      PD_LOG(PDERROR, "Failed to update status on coord nodes");
      return rc;
   }
   return rc;
}

// Get the latest version of SYSINFO.SYSDCBASE
INT32 _coordCMDRestore::_queryCataDCBase(BSONObj *result)
{
   INT32 rc = SDB_OK;
   rtnContextBuf buf; // cleans up when it goes out of scope
   OBJ_VEC results;
   rtnQueryOptions queryOpt;
   queryOpt.setFlag(FLG_QUERY_WITH_RETURNDATA);
   queryOpt.setCLFullName(CAT_SYSDCBASE_COLLECTION_NAME);
   queryOpt.setQuery(BSON(FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR));
   // Perform the query
   if ((rc = queryOnCataAndPushToVec(queryOpt, _cb, results, &buf)))
   {
      PD_LOG(PDERROR, "Failed during catalog query");
      return rc;
   }
   // Extract the result. Note that SYSINFO.SYSDCBASE only contains one doc.
   try
   {
      *result = results.front().copy();
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to copy BSON object");
      return (rc = SDB_OOM);
   }
   return rc;
}

// Run an ALTERDC command
INT32 _coordCMDRestore::_alterDC(const BSONObj &query)
{
   INT32 rc = SDB_OK;
   _Operator op(&rc, CMD_NAME_ALTER_DC); // Auto-cleaning
   if (rc)
   {
      return rc;
   }
   _QueryMsg msg(&rc, _cb, CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC,
                 MSG_CAT_ALTER_IMAGE_REQ, query);
   if (rc)
   {
      return rc;
   }
   if ((rc = op.ptr->init(_pResource, _cb, getTimeout())))
   {
      PD_LOG(PDERROR, "Failed to init operator");
      return rc;
   }
   INT64 contextID;
   if ((rc = op.ptr->execute(msg.header, _cb, contextID, NULL)))
   {
      PD_LOG(PDWARNING, "Failed to execute operator");
      return rc;
   }
   return rc;
}

// Run the given query against the data groups
INT32 _coordCMDRestore::_queryDataGroups(MSG_TYPE opCode, const string &clName,
                                         const BSONObj &query, OBJ_VEC *results)
{
   INT32 rc = SDB_OK;
   CoordGroupList groups;
   _Context context(_cb);                          // Auto-cleaning
   _QueryMsg msg(&rc, _cb, clName, opCode, query); // Auto-cleaning
   if (rc)
   {
      return rc;
   }
   // Get the groups list
   if ((rc = _pResource->updateGroupList(groups, _cb, NULL, TRUE, TRUE, FALSE)))
   {
      PD_LOG(PDERROR, "Get data groups failed");
      return rc;
   }
   // Run the query
   if ((rc = executeOnDataGroup(msg.header, _cb, groups, TRUE, NULL, NULL,
                                &(context.ptr), NULL)))
   {
      PD_LOG(PDERROR, "Execute on data groups failed");
      return rc;
   }
   if (!results)
   {
      // Only gather results if a pointer was provided
      return rc;
   }
   // Get the results
   if ((rc = gatherQueryResults(_cb, context.ptr, results)))
   {
      PD_LOG(PDERROR, "Failed to gather query results");
      return rc;
   }
   // Check the number of results is correct
   if (results->size() != groups.size())
   {
      PD_LOG(PDERROR, "The number of results [%u] does not match the number of "
                      "groups [%u]");
      return SDB_SYS;
   }
   return rc;
}

// Run the given command on the coord nodes
INT32 _coordCMDRestore::_cmdCoords(MSG_TYPE opCode, const string &clName,
                                   const BSONObj &query)
{
   INT32 rc = SDB_OK;
   CoordGroupList groups;
   _QueryMsg msg(&rc, _cb, clName, opCode, query); // Auto-cleaning
   if (rc)
   {
      return rc;
   }

   // Run on all nodes in the coordinator group only
   INT32 tmpRole[SDB_ROLE_MAX] = {0};
   tmpRole[SDB_ROLE_COORD] = 1;
   coordCtrlParam ctrlParam;
   ctrlParam.resetRole();
   ctrlParam.setParseRole(tmpRole);
   ROUTE_RC_MAP failedNodes;
   if ((rc = executeOnNodes(msg.header, _cb, ctrlParam, 0, failedNodes)))
   {
      PD_LOG(PDERROR, "Execute on coord group failed");
      return rc;
   }
   return rc;
}

/*
   coordCMDRestoreToPIT definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreToPIT, CMD_NAME_RESTORE_TO_PIT,
                                  FALSE);

// Entrypoint for restoreToPIT() on the coordinator
INT32 coordCMDRestoreToPIT::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                    INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   UINT64 targetTime = DPS_INVALID_TRANS_TIME; // Global time to restore to
   _pMsg = pMsg;
   _cb = cb;
   _optTestOnly = FALSE;
   _optSkipTest = FALSE;
   if ((rc = _parseRequest(&targetTime)))
   {
      return rc;
   }

   if ((rc = _checkStateAndRestore(targetTime)))
   {
      return rc;
   }

   if (!_optTestOnly && (rc = _setRestoreInProgress(FALSE)))
   {
      return rc;
   }
   PD_LOG(PDEVENT, "restoreToPIT completed successfully");
   return rc;
}

// Parse the client's request, extracting the targetTime if given
INT32 coordCMDRestoreToPIT::_parseRequest(UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   // Parse the request message
   BSONObj query;
   if ((rc = extractQuery(_pMsg, &query)))
   {
      PD_LOG(PDERROR, "Extract user query failed");
      return rc;
   }
   // Get the value of the GlobalTime option. Value is not required, in which
   // case restore will be to the latest consistency point.
   // Also get the bools TestOnly and SkipTest (both optional).
   if ((rc = globalTimeFromBSON(query, FIELD_NAME_GLOBAL_TIME, FALSE,
                                targetTime)) ||
       (rc = boolFromBSON(query, FIELD_NAME_TEST_ONLY, FALSE, &_optTestOnly)) ||
       (rc = boolFromBSON(query, FIELD_NAME_SKIP_TEST, FALSE, &_optSkipTest)))
   {
      PD_LOG(PDERROR, "User query invalid");
      return rc;
   }
   return rc;
}

// Check that the cluster is awaiting restore and coordinate the operation
INT32 coordCMDRestoreToPIT::_checkStateAndRestore(UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   BOOLEAN inProgress;
   // acquire restore lock to make sure there is only one running,
   // we do it before checking inprogress state so that we are not seeing
   // another almost finishing restore with following timing hole:
   //    Running PIT restore, with both inprogress = true and locked = true
   //    New restore does state check and found inprogress = true
   //    Running PIT finished, set inprogress = false and locked = false
   //    New restore gets in. But we shouldn't perform PIT restore.
   if ((rc = _updateRestoreLock(TRUE)))
   {
      PD_LOG(PDERROR, "There is already a restore running.");
      return (rc = SDB_RESTORE_RUNNING);
   }

   if ((rc = _checkRestoreInProgress(&inProgress)))
   {
      _updateRestoreLock(FALSE);
      return rc;
   }
   if (!inProgress)
   {
      _updateRestoreLock(FALSE);
      PD_LOG(PDERROR, "Cluster is not in [%s] state", FIELD_NAME_RESTORING);
      return (rc = SDB_RESTORE_NOT_IN_PROGRESS);
   }

   return (rc = _coordinateRestore(targetTime));
}

INT32 coordCMDRestoreToPIT::_coordinateRestore(UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   UINT64 minTime = DPS_INVALID_TRANS_TIME;
   UINT64 maxTime = DPS_INVALID_TRANS_TIME;
   if ((rc = _getGlobalRestoreWindow(&minTime, &maxTime)))
   {
      return rc;
   }
   return (rc = _restoreWithWindows(targetTime, minTime, maxTime));
}

// Query the primary of each data group for their restore window and set the
// min/max times
INT32 coordCMDRestoreToPIT::_getGlobalRestoreWindow(UINT64 *minTime,
                                                    UINT64 *maxTime)
{
   INT32 rc = SDB_OK;
   OBJ_VEC results;
   PD_LOG(PDINFO, "Gathering restore windows");
   // Query the nodes for the database snapshot
   if ((rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                              BSONObj(), &results)))
   {
      PD_LOG(PDERROR, "Query to collect windows from data groups failed");
      return rc;
   }

   return (rc = _getMinMaxWindowFromResponses(results, minTime, maxTime));
}

// Get the greatest min and least max values from the node query results
INT32 coordCMDRestoreToPIT::_getMinMaxWindowFromResponses(
    const OBJ_VEC &responses, UINT64 *minTime, UINT64 *maxTime)
{
   INT32 rc = SDB_OK;
   // Start at the extremes
   *minTime = 0;
   *maxTime = -1;
   for (OBJ_VEC::const_iterator it = responses.begin(); it != responses.end();
        ++it)
   {
      // Extract {TransInfo:{MinRecoverableTime or MaxTransCommitTime}}
      BSONElement field; // for the TransInfo field, which is an embedded object
      if ((rc = getElementFromBSON(*it, FIELD_NAME_TRANS_INFO, &field)) ||
          !field.isABSONObj())
      {
         // This should never happen
         return (rc = SDB_SYS);
      }
      BSONObj obj = field.embeddedObject();
      // Get the MinRecoverableTime and MaxTransCommitTime values
      UINT64 tmpMin, tmpMax;
      if ((rc = globalTimeFromBSON(obj, FIELD_NAME_TRANS_MIN_RECOVER_TIME, TRUE,
                                   &tmpMin)) ||
          (rc = globalTimeFromBSON(obj, FIELD_NAME_TRANS_MAX_COMMIT_TIME, TRUE,
                                   &tmpMax)))
      {
         PD_LOG(PDERROR, "Invalid database snapshot result");
         return (rc = SDB_SYS);
      }
      *minTime = tmpMin > (*minTime) ? tmpMin : *minTime; // new greatest min
      *maxTime = tmpMax < (*maxTime) ? tmpMax : *maxTime; // new least max
   }
   PD_LOG(PDINFO, "Global consistency window [%llu, %llu]", *minTime, *maxTime);
   if ((*minTime) > (*maxTime))
   {
      PD_LOG(PDERROR, "No valid global consistency points");
      return (rc = SDB_RESTORE_NO_CONSISTENT_PIT);
   }
   return rc;
}

INT32 coordCMDRestoreToPIT::_restoreWithWindows(UINT64 targetTime,
                                                UINT64 minTime, UINT64 maxTime)
{
   INT32 rc = SDB_OK;
   if ((rc = _setTargetTimestamp(minTime, maxTime, &targetTime)))
   {
      return rc;
   }
   // Perform the test run (unless SkipTest)
   if (!_optSkipTest && (rc = _generateQueryAndRestore(targetTime, TRUE)))
   {
      PD_LOG(PDERROR, "Failed the restore test run. Aborting.");
      return rc;
   }
   // Perform the real run (unless TestOnly)
   if (!_optTestOnly && (rc = _generateQueryAndRestore(targetTime, FALSE)))
   {
      PD_LOG(PDERROR, "Failed during restore to point-in-time on nodes.");
      return rc;
   }
   return rc;
}

// Determine the target consistency point - whether the user provided value fits
// in the global min/max or the max value as a default
INT32 coordCMDRestoreToPIT::_setTargetTimestamp(UINT64 minTime, UINT64 maxTime,
                                                UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   if (DPS_INVALID_TRANS_TIME == *targetTime)
   {
      // No user input so use the latest consistency point
      *targetTime = maxTime;
   }
   else if ((*targetTime) < minTime || (*targetTime) > maxTime)
   {
      PD_LOG(PDERROR, "Target time is outside of the valid consistency window");
      return (rc = SDB_INVALIDARG);
   }
   return rc;
}

// Build the query and perform restoreToPIT() on all of the data groups
INT32 coordCMDRestoreToPIT::_generateQueryAndRestore(UINT64 targetTime,
                                                     BOOLEAN test)
{
   INT32 rc = SDB_OK;
   BSONObj query;
   PD_LOG(PDINFO, "Restoring cluster to %llu", targetTime);
   if (test)
   {
      if ((rc = _buildRestoreQuery(targetTime, test, &query)) ||
          (rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                                 CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_PIT,
                                 query, NULL)))
      {
         PD_LOG(PDERROR, "One or more nodes failed restoreToPIT test");
         return rc;
      }
      return rc;
   }
   // Start the transaction
   coordTransHandler trans(_cb, _pResource);
   if ((rc = trans.getRc()))
   {
      PD_LOG(PDERROR, "Failed to begin transaction for restoreToPIT");
      return rc;
   }
   if ((rc = _buildRestoreQuery(targetTime, test, &query)) ||
       (rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_PIT, query,
                              NULL)))
   {
      PD_LOG(PDERROR, "One or more nodes failed restoreToPIT");
      // trans will perform rollback (in its destructor) because commit wasn't
      // called
      return rc;
   }
   if ((rc = trans.commit()))
   {
      PD_LOG(PDERROR, "Failed to commit restoreToPIT");
      return rc;
   }
   return rc;
}

// Build the message query for the restoreToPIT command
INT32 coordCMDRestoreToPIT::_buildRestoreQuery(UINT64 targetTime, BOOLEAN test,
                                               BSONObj *query)
{
   INT32 rc = SDB_OK;
   // The command is a query-type message on the "$restore to pit" collection.
   // The query body is a {"GlobalTime": "123"} where 123 is the time.
   try
   {
      BSONObjBuilder builder;
      builder.append(FIELD_NAME_GLOBAL_TIME, (INT64)targetTime);
      if (test)
      {
         // The test run to check that the operation would succeed.
         // Adds the field "TestOnly: true"
         builder.appendBool(FIELD_NAME_TEST_ONLY, TRUE);
      }
      else
      {
         // The real run that performs the restore.
         // Adds the field "SkipTest: true" and the transaction info
         builder.appendBool(FIELD_NAME_SKIP_TEST, TRUE);
         builder.append(FIELD_NAME_TRANSACTION_ID_SN,
                        (INT64)(_cb->getTransID().getGlobSN()));
         builder.append(FIELD_NAME_TRANSACTION_ID_NODEID,
                        (INT32)(_cb->getTransID().getNodeID()));
      }
      *query = builder.obj();
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return (rc = SDB_OOM);
   }
   return rc;
}

// Update the catalog DC RestoreLocked value.
// We use this value to guarantee that there should only be one restoreToPIT
// running at a time.
// @param   enable   Whether to enable or diable the state
INT32 coordCMDRestoreToPIT::_updateRestoreLock(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   BSONObj query;
   PD_LOG(PDINFO, "Setting cluster restore locked [%s] = [%d]",
          FIELD_NAME_RESTORE_LOCKED, enable);

   try
   {
      if (enable)
      {
         query = BSON(FIELD_NAME_ACTION << CMD_VALUE_NAME_RESTORE_LOCK);
      }
      else
      {
         query = BSON(FIELD_NAME_ACTION << CMD_VALUE_NAME_RESTORE_UNLOCK);
      }
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return (rc = SDB_OOM);
   }

   // Update the DC: this will update cata and data
   if ((rc = _alterDC(query)))
   {
      PD_LOG(PDERROR, "Failed to update DC state across nodes");
      return rc;
   }
   return rc;
}

/*
   coordCMDRestoreAbort definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreAbort, CMD_NAME_RESTORE_ABORT,
                                  FALSE);

// Entrypoint for restoreAbort() on the coordinator
INT32 coordCMDRestoreAbort::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                    INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   BOOLEAN inProgress; // whether RestoreInProgress is already set
   _pMsg = pMsg;
   _cb = cb;
   if ((rc = _checkRestoreInProgress(&inProgress)))
   {
      return rc;
   }
   if (!inProgress)
   {
      // Treat as a warning only, not an error
      PD_LOG(PDWARNING, "Cluster is not in [%s] state", FIELD_NAME_RESTORING);
      return rc;
   }
   if ((rc = _setRestoreInProgress(FALSE)))
   {
      return rc;
   }
   PD_LOG(PDEVENT, "restoreAbort completed successfully");
   return rc;
}

/*
   coordCMDRestorePrepareFlashback definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestorePrepareFlashback,
                                  CMD_NAME_PREPARE_FLASHBACK, FALSE);

// Entrypoint for restorePrepareFlashback() on the coordinator
INT32 coordCMDRestorePrepareFlashback::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                               INT64 &contextID,
                                               rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   BOOLEAN inProgress; // whether RestoreInProgress is already set
   _pMsg = pMsg;
   _cb = cb;
   if ((rc = _checkRestoreInProgress(&inProgress)))
   {
      return rc;
   }
   if (inProgress)
   {
      // Treat as a warning only, not an error
      PD_LOG(PDWARNING, "Cluster already in [%s] state", FIELD_NAME_RESTORING);
      return rc;
   }
   if ((rc = _setRestoreInProgress(TRUE)))
   {
      return rc;
   }
   PD_LOG(PDEVENT, "restorePrepareFlashback completed successfully");
   return rc;
}

} // namespace engine
