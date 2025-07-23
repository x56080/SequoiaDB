/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*******************************************************************************/
#include "coordCommandRestore.hpp"

#include <string>

#include "catDef.hpp"
#include "coordCommandBase.hpp"
#include "coordContext.hpp"
#include "coordFactory.hpp"
#include "coordTrace.hpp"
#include "coordTransOperator.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossMemPool.hpp"
#include "ossTypes.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnQueryOptions.hpp"
#include "stpAgent.hpp"
#include "stpLogicalTime.hpp"
#include "utilBSON.hpp"

using bson::BSONElement;
using bson::BSONObj;
using std::exception;
using std::string;

namespace
{

// Uses STP to convert a high precision timestamp to a global logical time
INT32 convRealToLogicalTime(const engine::stpHPTime &input, UINT64 *output)
{
   INT32 rc = SDB_OK;
   // Convert to logical time
   engine::stpAgent agent;
   engine::stpClient client;
   if ((rc = agent.checkAvailable()) || (rc = agent.getClient(client)))
   {
      PD_LOG(PDERROR, "Error initializing stp client");
      return rc;
   }
   engine::stpHPTime logicalTime;
   if ((rc = client.convRealTimeToLogicalTime(input, logicalTime)))
   {
      PD_LOG(PDERROR, "Failed to convert timestamp to logical time");
      return rc;
   }
   *output = logicalTime.toMicroSecond();
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

using namespace util;

/*
   _coordCMDRestore definitions
*/

// Check the status of the cluster
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_CHECK, "_coordCMDRestore::_checkRestoreInProgress" )
INT32 _coordCMDRestore::_checkRestoreInProgress(BOOLEAN *inProgress)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_CHECK, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_SET, "_coordCMDRestore::_setRestoreInProgress" )
INT32 _coordCMDRestore::_setRestoreInProgress(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_SET, &rc);
   PD_TRACER(1, PD_PACK_INT(enable));
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

// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_SETCOORDS, "_coordCMDRestore::_setRestoreInProgressCoords" )
INT32 _coordCMDRestore::_setRestoreInProgressCoords(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_SETCOORDS, &rc);
   const string command =
       enable ? CMD_NAME_RESTORE_PREPARE : CMD_NAME_RESTORE_ABORT;
   if ((rc = _cmdCoords(MSG_BS_QUERY_REQ, CMD_ADMIN_PREFIX + command,
                        BSONObj())))
   {
      PD_LOG(PDERROR, "Failed to update status on coord nodes");
      return rc;
   }
   return rc;
}

// Get the latest version of SYSINFO.SYSDCBASE
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_QUERYCAT, "_coordCMDRestore::_queryCataDCBase" )
INT32 _coordCMDRestore::_queryCataDCBase(BSONObj *result)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_QUERYCAT, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_ALTERDC, "_coordCMDRestore::_alterDC" )
INT32 _coordCMDRestore::_alterDC(const BSONObj &query)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_ALTERDC, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_QUERYDATA, "_coordCMDRestore::_queryDataGroups" )
INT32 _coordCMDRestore::_queryDataGroups(MSG_TYPE opCode, const string &clName,
                                         const BSONObj &query, OBJ_VEC *results)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_QUERYDATA, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_CMDCOORDS, "_coordCMDRestore::_cmdCoords" )
INT32 _coordCMDRestore::_cmdCoords(MSG_TYPE opCode, const string &clName,
                                   const BSONObj &query)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_CMDCOORDS, &rc);
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
   coordCMDRestoreToTime definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreToTime,
                                  CMD_NAME_RESTORE_TO_TIME, FALSE);

// Entrypoint for restoreToTime() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_EXE, "coordCMDRestoreToTime::execute" )
INT32 coordCMDRestoreToTime::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                     INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_EXE, &rc);
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
   PD_LOG(PDEVENT, "restoreToTime completed successfully");
   return rc;
}

// Parse the client's request, extracting the targetTime if given
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_PARSE, "coordCMDRestoreToTime::_parseRequest" )
INT32 coordCMDRestoreToTime::_parseRequest(UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_PARSE, &rc);
   // Parse the request message
   BSONObj query;
   if ((rc = extractQuery(_pMsg, &query)))
   {
      PD_LOG(PDERROR, "Extract user query failed");
      return rc;
   }
   // Get the value of the GlobalTime option. Value is not required, in which
   // case restore will be to the latest consistency point.
   // Same with option Time.
   // Also get the bools TestOnly and SkipTest (both optional).
   if ((rc = _parseTime(query, targetTime)) ||
       (rc = fromBsonObj(query, FIELD_NAME_TEST_ONLY, &_optTestOnly, FALSE)) ||
       (rc = fromBsonObj(query, FIELD_NAME_SKIP_TEST, &_optSkipTest, FALSE)))
   {
      PD_LOG(PDERROR, "User query invalid");
      return rc;
   }
   return rc;
}

// Get the target time input (if provided)
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_PARSETIME, "coordCMDRestoreToTime::_parseTime" )
INT32 coordCMDRestoreToTime::_parseTime(const BSONObj &query,
                                        UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_PARSETIME, &rc);
   // Check that exactly one of Latest: TRUE, GlobalTime and Time are provided
   BOOLEAN latest = FALSE;
   if ((rc = boolFromBsonObj(query, FIELD_NAME_LATEST, &latest, FALSE)))
   {
      PD_LOG_MSG(PDERROR, "%s must be boolean", FIELD_NAME_LATEST);
      return (rc = SDB_INVALIDARG);
   }
   INT32 specifier_count = latest;
   specifier_count += query.hasElement(FIELD_NAME_GLOBAL_TIME) ? 1 : 0;
   specifier_count += query.hasElement(FIELD_NAME_TIME) ? 1 : 0;
   if (specifier_count != 1)
   {
      PD_LOG_MSG(PDERROR, "Exactly one of %s, %s, and %s must be provided",
                 FIELD_NAME_LATEST, FIELD_NAME_GLOBAL_TIME, FIELD_NAME_TIME);
      return (rc = SDB_INVALIDARG);
   }
   if (latest)
   {
      // Restore to latest consistency point
      return rc;
   }
   if (query.hasElement(FIELD_NAME_GLOBAL_TIME))
   {
      // Global time specified
      if ((rc = fromBsonObj(query, FIELD_NAME_GLOBAL_TIME, targetTime)))
      {
         PD_LOG_MSG(PDERROR, "%s must be a valid global time",
                    FIELD_NAME_GLOBAL_TIME);
         return (rc = SDB_INVALIDARG);
      }
      return rc; // success
   }
   // Timestamp specified, convert it to global logical time
   return (rc = _targetTimeFromTimestamp(query, targetTime));
}

// Get the target time from the timestamp input, converted to a global time
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_TIMESTAMP, "coordCMDRestoreToTime::_targetTimeFromTimestamp" )
INT32 coordCMDRestoreToTime::_targetTimeFromTimestamp(const BSONObj &query,
                                                      UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_TIMESTAMP, &rc);
   stpHPTime timestamp;
   // Try and parse as BSON Timestamp
   if (query.getField(FIELD_NAME_TIME).type() == bson::Timestamp)
   {
      if ((rc = timestamp.fromBSONTimestamp(query.getField(FIELD_NAME_TIME))))
      {
         PD_LOG_MSG(PDERROR, "Error parsing %s as Timestamp", FIELD_NAME_TIME);
         return rc;
      }
   }
   else
   {
      PD_LOG_MSG(PDERROR, "Unsupported type for argument %s", FIELD_NAME_TIME);
      return (rc = SDB_INVALIDARG);
   }
   return (rc = convRealToLogicalTime(timestamp, targetTime));
}

// Check that the cluster is awaiting restore and coordinate the operation
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_CHECK, "coordCMDRestoreToTime::_checkStateAndRestore" )
INT32 coordCMDRestoreToTime::_checkStateAndRestore(UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_CHECK, &rc);
   BOOLEAN inProgress;
   if ((rc = _checkRestoreInProgress(&inProgress)))
   {
      return rc;
   }
   if (!inProgress)
   {
      PD_LOG(PDERROR, "Cluster is not in [%s] state", FIELD_NAME_RESTORING);
      return (rc = SDB_RESTORE_NOT_IN_PROGRESS);
   }
   if ((rc = _coordinateRestore(targetTime)))
   {
      return rc;
   }
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_COORDRES, "coordCMDRestoreToTime::_coordinateRestore" )
INT32 coordCMDRestoreToTime::_coordinateRestore(UINT64 targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_COORDRES, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_GETWINDOWS, "coordCMDRestoreToTime::_getGlobalRestoreWindow" )
INT32 coordCMDRestoreToTime::_getGlobalRestoreWindow(UINT64 *minTime,
                                                     UINT64 *maxTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_GETWINDOWS, &rc);
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_PARSEWINDOWS, "coordCMDRestoreToTime::_getMinMaxWindowFromResponses" )
INT32 coordCMDRestoreToTime::_getMinMaxWindowFromResponses(
    const OBJ_VEC &responses, UINT64 *minTime, UINT64 *maxTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_PARSEWINDOWS, &rc);
   // Start at the extremes
   *minTime = 0;
   *maxTime = -1;
   for (OBJ_VEC::const_iterator it = responses.begin(); it != responses.end();
        ++it)
   {
      // Extract {TransInfo:{MinRecoverableTime:..., MaxTransCommitTime:...}}
      BSONObj obj;
      if ((rc = fromBsonObj(*it, FIELD_NAME_TRANS_INFO, &obj)))
      {
         // This should never happen
         PD_LOG(PDERROR, "Failed to extract message from node [rc=%d]", rc);
         return (rc = SDB_SYS);
      }
      // Get the MinRecoverableTime and MaxTransCommitTime values
      UINT64 tmpMin, tmpMax;
      if ((rc = fromBsonObj(obj, FIELD_NAME_TRANS_MIN_RECOVER_TIME, &tmpMin)) ||
          (rc = fromBsonObj(obj, FIELD_NAME_TRANS_MAX_COMMIT_TIME, &tmpMax)))
      {
         PD_LOG(PDERROR, "Invalid database snapshot result [rc=%d]", rc);
         return (rc = SDB_SYS);
      }
      *minTime = tmpMin > (*minTime) ? tmpMin : *minTime; // new greatest min
      *maxTime = tmpMax < (*maxTime) ? tmpMax : *maxTime; // new least max
   }
   PD_LOG(PDINFO, "Global consistency window [%llu, %llu]", *minTime, *maxTime);
   if ((*minTime) > (*maxTime))
   {
      PD_LOG(PDERROR, "No valid global consistency points [%llu > %llu]",
             *minTime, *maxTime);
      return (rc = SDB_RESTORE_NO_CONSISTENT_PIT);
   }
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_RESTORE, "coordCMDRestoreToTime::_restoreWithWindows" )
INT32 coordCMDRestoreToTime::_restoreWithWindows(UINT64 targetTime,
                                                 UINT64 minTime, UINT64 maxTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_RESTORE, &rc);
   if ((rc = _setTargetTime(minTime, maxTime, &targetTime)))
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
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_TARGETTIME, "coordCMDRestoreToTime::_setTargetTime" )
INT32 coordCMDRestoreToTime::_setTargetTime(UINT64 minTime, UINT64 maxTime,
                                            UINT64 *targetTime)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_TARGETTIME, &rc);
   if (DPS_INVALID_TRANS_TIME == *targetTime)
   {
      // No user input so use the latest consistency point
      *targetTime = maxTime;
   }
   else if ((*targetTime) < minTime || (*targetTime) > maxTime)
   {
      PD_LOG_MSG(PDERROR,
                 "Target time [%llu] is outside of the valid consistency "
                 "window [%llu:%llu]",
                 *targetTime, minTime, maxTime);
      return (rc = SDB_INVALIDARG);
   }
   return rc;
}

// Build the query and perform restoreToTime() on all of the data groups
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_DO, "coordCMDRestoreToTime::_generateQueryAndRestore" )
INT32 coordCMDRestoreToTime::_generateQueryAndRestore(UINT64 targetTime,
                                                      BOOLEAN test)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_DO, &rc);
   PD_TRACER(1, PD_PACK_INT(test));
   BSONObj query;
   PD_LOG(PDINFO, "Restoring cluster to %llu", targetTime);
   if (test)
   {
      if ((rc = _buildRestoreQuery(targetTime, test, &query)) ||
          (rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                                 CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_TIME,
                                 query, NULL)))
      {
         PD_LOG(PDERROR, "One or more nodes failed restoreToTime test");
         return rc;
      }
      return rc;
   }
   // Start the transaction
   coordTransHandler trans(_cb, _pResource);
   if ((rc = trans.getRc()))
   {
      PD_LOG(PDERROR, "Failed to begin transaction for restoreToTime");
      return rc;
   }
   if ((rc = _buildRestoreQuery(targetTime, test, &query)) ||
       (rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_TIME, query,
                              NULL)))
   {
      PD_LOG(PDERROR, "One or more nodes failed restoreToTime");
      // trans will perform rollback (in its destructor) because commit wasn't
      // called
      return rc;
   }
   if ((rc = trans.commit()))
   {
      PD_LOG(PDERROR, "Failed to commit restoreToTime");
      return rc;
   }
   return rc;
}

// Build the message query for the restoreToTime command
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_BUILDQUERY, "coordCMDRestoreToTime::_buildRestoreQuery" )
INT32 coordCMDRestoreToTime::_buildRestoreQuery(UINT64 targetTime, BOOLEAN test,
                                                BSONObj *query)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_BUILDQUERY, &rc);
   // The command is a query-type message on the "$restore to pit" collection.
   // The query body is a {"GlobalTime": "123"} where 123 is the time.
   try
   {
      BSONObjBuilder builder;
      builder.append(FIELD_NAME_GLOBAL_TIME, (INT64)targetTime);
      if (test)
      {
         // The test run to check that the operation would succeed.
         // Adds the field "TestOnly: 1"
         builder.append(FIELD_NAME_TEST_ONLY, TRUE);
      }
      else
      {
         // The real run that performs the restore.
         // Adds the field "SkipTest: 1" and the transaction info
         builder.append(FIELD_NAME_SKIP_TEST, TRUE);
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

/*
   coordCMDRestoreAbort definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreAbort, CMD_NAME_RESTORE_ABORT,
                                  FALSE);

// Entrypoint for restoreAbort() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREABORT_EXE, "coordCMDRestoreAbort::execute" )
INT32 coordCMDRestoreAbort::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                    INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREABORT_EXE, &rc);
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
   coordCMDRestorePrepare definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestorePrepare,
                                  CMD_NAME_RESTORE_PREPARE, FALSE);

// Entrypoint for restorePrepare() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPREPARE_EXE, "coordCMDRestorePrepare::execute" )
INT32 coordCMDRestorePrepare::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                      INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPREPARE_EXE, &rc);
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
   PD_LOG(PDEVENT, "restorePrepare completed successfully");
   return rc;
}

} // namespace engine
