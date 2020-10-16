/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
         return SDB_OOM;
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
   _Context(engine::pmdEDUCB *cb) : _cb(cb), ptr(NULL){};
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
INT32 _coordCMDRestore::_checkClusterState(pmdEDUCB *cb)
{
   // Check the local cache
   if (!pmdGetKRCB()->isDBRestoring())
   {
      PD_LOG(PDERROR, "Cluster is not in [%s] state", FIELD_NAME_RESTORING);
      return SDB_RESTORE_NOT_IN_PROGRESS;
   }
   return _checkDCForState(cb);
}

// Query the catalog to confirm - check RestoreInProgress in SYSINFO.SYSDCBASE
INT32 _coordCMDRestore::_checkDCForState(pmdEDUCB *cb)
{
   INT32 rc = SDB_OK;
   BSONObj document;
   if ((rc = _queryCataDCBase(cb, &document)))
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
      return SDB_SYS;
   }
   if (!field.trueValue())
   {
      PD_LOG(PDERROR, "Catalog status conflicts with coordinator");
      return SDB_SYS;
   }
   return rc;
}

// Update the catalog with RestoreInProgress: false
INT32 _coordCMDRestore::_resetState(pmdEDUCB *cb)
{
   INT32 rc = SDB_OK;
   BSONObj query;
   PD_LOG(PDINFO, "Resetting cluster state");
   try
   {
      query = BSON(FIELD_NAME_ACTION << CMD_VALUE_NAME_DISABLE_RESTORING);
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return SDB_OOM;
   }
   if ((rc = _alterDC(cb, query)))
   {
      PD_LOG(PDERROR, "Failed to update DC state across nodes");
      return rc;
   }
   // Update the local cache
   pmdGetKRCB()->setDBRestoring(FALSE);
   return rc;
}

// Get the latest version of SYSINFO.SYSDCBASE
INT32 _coordCMDRestore::_queryCataDCBase(pmdEDUCB *cb, BSONObj *result)
{
   INT32 rc = SDB_OK;
   rtnContextBuf buf; // cleans up when it goes out of scope
   OBJ_VEC results;
   rtnQueryOptions queryOpt;
   queryOpt.setFlag(FLG_QUERY_WITH_RETURNDATA);
   queryOpt.setCLFullName(CAT_SYSDCBASE_COLLECTION_NAME);
   queryOpt.setQuery(BSON(FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR));
   // Perform the query
   if ((rc = queryOnCataAndPushToVec(queryOpt, cb, results, &buf)))
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
      return SDB_OOM;
   }
   return rc;
}

// Run an ALTERDC command
INT32 _coordCMDRestore::_alterDC(pmdEDUCB *cb, const BSONObj &query)
{
   INT32 rc = SDB_OK;
   _Operator op(&rc, CMD_NAME_ALTER_DC); // Auto-cleaning
   if (rc)
   {
      return rc;
   }
   _QueryMsg msg(&rc, cb, CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC,
                 MSG_CAT_ALTER_IMAGE_REQ, query);
   if (rc)
   {
      return rc;
   }
   if ((rc = op.ptr->init(_pResource, cb, getTimeout())))
   {
      PD_LOG(PDERROR, "Failed to init operator");
      return rc;
   }
   INT64 contextID;
   if ((rc = op.ptr->execute(msg.header, cb, contextID, NULL)))
   {
      PD_LOG(PDWARNING, "Failed to execute operator");
      return rc;
   }
   return rc;
}

// Run the given query against the data groups
INT32 _coordCMDRestore::_queryDataGroups(pmdEDUCB *cb, MSG_TYPE opCode,
                                         const string &clName,
                                         const BSONObj &query, OBJ_VEC *results)
{
   INT32 rc = SDB_OK;
   CoordGroupList groups;
   _Context context(cb);                          // Auto-cleaning
   _QueryMsg msg(&rc, cb, clName, opCode, query); // Auto-cleaning
   if (rc)
   {
      return rc;
   }
   // Get the groups list
   if ((rc = _pResource->updateGroupList(groups, cb, NULL, TRUE, TRUE, FALSE)))
   {
      PD_LOG(PDERROR, "Get data groups failed");
      return rc;
   }
   // Run the query
   if ((rc = executeOnDataGroup(msg.header, cb, groups, TRUE, NULL, NULL,
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
   if ((rc = gatherQueryResults(cb, context.ptr, results)))
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
   _optTestOnly = FALSE;
   _optSkipTest = FALSE;
   if ((rc = _parseRequest(pMsg, &targetTime)))
   {
      return rc;
   }
   if ((rc = _checkStateAndRestore(cb, targetTime)))
   {
      return rc;
   }
   if (!_optTestOnly && (rc = _resetState(cb)))
   {
      return rc;
   }
   PD_LOG(PDEVENT, "restoreToPIT completed successfully");
   return SDB_OK;
}

// Parse the client's request, extracting the targetTime if given
INT32 coordCMDRestoreToPIT::_parseRequest(MsgHeader *pMsg, UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   // Parse the request message
   BSONObj query;
   if ((rc = extractQuery(pMsg, &query)))
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
INT32 coordCMDRestoreToPIT::_checkStateAndRestore(pmdEDUCB *cb,
                                                  UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   if ((rc = _checkClusterState(cb)))
   {
      return rc;
   }
   return _coordinateRestore(cb, targetTime);
}

INT32 coordCMDRestoreToPIT::_coordinateRestore(pmdEDUCB *cb, UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   UINT64 minTime = DPS_INVALID_TRANS_TIME;
   UINT64 maxTime = DPS_INVALID_TRANS_TIME;
   if ((rc = _getGlobalRestoreWindow(cb, &minTime, &maxTime)))
   {
      return rc;
   }
   return _restoreWithWindows(cb, targetTime, minTime, maxTime);
}

// Query the primary of each data group for their restore window and set the
// min/max times
INT32 coordCMDRestoreToPIT::_getGlobalRestoreWindow(pmdEDUCB *cb,
                                                    UINT64 *minTime,
                                                    UINT64 *maxTime)
{
   INT32 rc = SDB_OK;
   OBJ_VEC results;
   PD_LOG(PDINFO, "Gathering restore windows");
   // Query the nodes for the database snapshot
   if ((rc = _queryDataGroups(cb, MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                              BSONObj(), &results)))
   {
      PD_LOG(PDERROR, "Query to collect windows from data groups failed");
      return rc;
   }

   return _getMinMaxWindowFromResponses(results, minTime, maxTime);
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
         return SDB_SYS;
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
         return SDB_SYS;
      }
      *minTime = tmpMin > (*minTime) ? tmpMin : *minTime; // new greatest min
      *maxTime = tmpMax < (*maxTime) ? tmpMax : *maxTime; // new least max
   }
   PD_LOG(PDINFO, "Global consistency window [%llu, %llu]", *minTime, *maxTime);
   if ((*minTime) > (*maxTime))
   {
      PD_LOG(PDERROR, "No valid global consistency points");
      return SDB_RESTORE_NO_CONSISTENT_PIT;
   }
   return rc;
}

INT32 coordCMDRestoreToPIT::_restoreWithWindows(pmdEDUCB *cb, UINT64 targetTime,
                                                UINT64 minTime, UINT64 maxTime)
{
   INT32 rc = SDB_OK;
   if ((rc = _setTargetTimestamp(minTime, maxTime, &targetTime)))
   {
      return rc;
   }
   // Perform the test run (unless SkipTest)
   if (!_optSkipTest && (rc = _restoreDataGroups(cb, targetTime, TRUE)))
   {
      PD_LOG(PDERROR, "Failed the restore test run. Aborting.");
      return rc;
   }
   // Perform the real run (unless TestOnly)
   if (!_optTestOnly && (rc = _restoreDataGroups(cb, targetTime, FALSE)))
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
   if (DPS_INVALID_TRANS_TIME == *targetTime)
   {
      // No user input so use the latest consistency point
      *targetTime = maxTime;
   }
   else if ((*targetTime) < minTime || (*targetTime) > maxTime)
   {
      PD_LOG(PDERROR, "Target time is outside of the valid consistency window");
      return SDB_INVALIDARG;
   }
   return SDB_OK;
}

// Execute restoreToPIT() on all of the data groups
INT32 coordCMDRestoreToPIT::_restoreDataGroups(pmdEDUCB *cb, UINT64 targetTime,
                                               BOOLEAN test)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDINFO, "Restoring cluster to %llu", targetTime);
   // The command is a query-type message on the "$restore to pit" collection.
   // The query body is a {"GlobalTime": "123"} where 123 is the time.
   BSONObj query;
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
         // Adds the field "SkipTest: true"
         builder.appendBool(FIELD_NAME_SKIP_TEST, TRUE);
      }
      query = builder.obj();
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return SDB_OOM;
   }
   if ((rc = _queryDataGroups(cb, MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_PIT, query,
                              NULL)))
   {
      PD_LOG(PDERROR, "One or more nodes failed restoreToPIT");
      return rc;
   }
   return SDB_OK;
}

/*
   coordCMDRestoreAbort definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreAbort, CMD_NAME_RESTORE_ABORT,
                                  FALSE);

// Entrypoint for restoreToPIT() on the coordinator
INT32 coordCMDRestoreAbort::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                    INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   if ((rc = _checkClusterState(cb)))
   {
      return rc;
   }
   if ((rc = _resetState(cb)))
   {
      return rc;
   }
   PD_LOG(PDEVENT, "restoreAbort completed successfully");
   return SDB_OK;
}

} // namespace engine
