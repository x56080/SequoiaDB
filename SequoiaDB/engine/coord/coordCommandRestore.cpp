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

#include <algorithm>
#include <vector>

#include "catDef.hpp"
#include "coordCommandBase.hpp"
#include "coordCommandRestore.hpp"
#include "coordContext.hpp"
#include "coordFactory.hpp"
#include "coordTrace.hpp"
#include "coordTransOperator.hpp"
#include "coordUtil.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossMemPool.hpp"
#include "ossTypes.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnQueryOptions.hpp"
#include "stpAgent.hpp"
#include "stpLogicalTime.hpp"
#include "utilBSON.hpp"
#include <vector>

using bson::BSONElement;
using bson::BSONObj;
using std::exception;

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
      PD_LOG(PDERROR, "Error initializing stp client [rc=%d]", rc);
      return rc;
   }
   engine::stpHPTime logicalTime;
   if ((rc = client.convRealTimeToLogicalTime(input, logicalTime)))
   {
      PD_LOG(PDERROR, "Failed to convert timestamp to logical time [rc=%d]",
             rc);
      return rc;
   }
   *output = logicalTime.toMicroSecond();
   return rc;
}

// Uses STP to convert a global logical time to a real time in string format
INT32 convLogicalTimeToRealTime(UINT64 input, ossPoolString *output)
{
   INT32 rc = SDB_OK;
   if (input == 0)
   {
      *output = "-";
      return rc;
   }
   // Convert to logical time
   engine::stpAgent agent;
   engine::stpClient client;
   if ((rc = agent.checkAvailable()) || (rc = agent.getClient(client)))
   {
      PD_LOG(PDERROR, "Error initializing stp client [rc=%d]", rc);
      return rc;
   }
   ossTimestamp realTime;
   if ((rc = client.convLogicalTimeToRealTime(input, realTime)))
   {
      PD_LOG(PDERROR, "Failed to convert logical time to timestamp [rc=%d]",
             rc);
      return rc;
   }
   CHAR strTime[OSS_TIMESTAMP_STRING_LEN + 1] = {0};
   ossTimestampToString(realTime, strTime);
   *output = strTime;
   return rc;
}

// Returns the string from convLogicalTimeToRealTime
// If the calls to stp fail, returns an empty string
ossPoolString printLogicalTimeToRealTime(UINT64 input)
{
   ossPoolString res;
   if (convLogicalTimeToRealTime(input, &res))
   {
      return "";
   }
   return res;
}

// Extract the query from a message object
INT32 extractQuery(MsgHeader *pMsg, BSONObj *query)
{
   INT32 rc = SDB_OK;
   const CHAR *pQuery = NULL; // pointer to the query buffer
   if ((rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL,
                              NULL, &pQuery, NULL, NULL, NULL)))
   {
      PD_LOG(PDERROR, "Error extracting query [rc=%d]", rc);
      return rc;
   }
   // Using an existing buffer, does not copy, so no need to try/catch
   query->init(pQuery);
   return rc;
}

// Converts epoch seconds to stpHPTime object
INT32 timeFromInt(UINT64 input, engine::stpHPTime* timestamp)
{
   // Seconds precision only, nanoseconds = 0
   *timestamp = engine::stpHPTime(input, 0);
   return SDB_OK;
}

// Converts from a string timestamp to stpHPTime object
INT32 timeFromStr(const ossPoolString &input, engine::stpHPTime *timestamp)
{
   INT32 rc = SDB_OK;
   time_t sec;
   UINT64 usec = 0;
   if ((rc = engine::utilStr2TimeT(input.c_str(), sec, &usec)))
   {
      PD_LOG(PDERROR, "utilStr2TimeT failed [rc=%d]", rc);
      return rc;
   }
   *timestamp = engine::stpHPTime(sec, usec * 1000);
   return rc;
}

// Get the Time option. If Time is 0 then time is DPS_INVALID_TRANS_TIME and
// the latest consistency point will be used.
INT32 _parseTime(const BSONObj &query, UINT64 *t)
{
   using namespace engine;
   INT32 rc = SDB_OK;
   // Time option is required
   if (!query.hasElement(FIELD_NAME_TIME)) {
      PD_LOG_MSG(PDERROR, "Option %s required", FIELD_NAME_TIME);
      return (rc = SDB_INVALIDARG);
   }
   bson::BSONElement ele = query.getField(FIELD_NAME_TIME);
   stpHPTime timestamp;
   // Get the timestamp. TIME can be int, string, or timestamp.
   if (ele.isNumber())
   {
      // If TIME is an integer it is the timestamp in epoch seconds
      UINT64 secs;
      if ((rc = util::fromBsonObj(query, FIELD_NAME_TIME, &secs)))
      {
         // Already checked it was a number, the only reason this would fail is
         // if it is a negative number
         PD_LOG_MSG(PDERROR, "Time cannot be negative");
         return rc;
      }
      if (0 == secs)
      {
         // Use latest consistency point
         PD_LOG(PDINFO, "Restoring to latest consistency point");
         *t = DPS_INVALID_TRANS_TIME;
         return rc;
      }
      timeFromInt(secs, &timestamp);
   }
   else if (ele.type() == bson::String)
   {
      // If TIME is a string it is a timestamp
      ossPoolString timestamp_string;
      util::fromBsonObj(query, FIELD_NAME_TIME, &timestamp_string);
      if ((rc = timeFromStr(timestamp_string, &timestamp)))
      {
         PD_LOG_MSG(PDERROR, "Error parsing [%s] as a timestamp [rc=%d]",
                    timestamp_string.c_str(), rc);
         return rc;
      }
   }
   else if (ele.type() == bson::Timestamp)
   {
      // Try and parse as BSON Timestamp
      if ((rc = timestamp.fromBSONTimestamp(query.getField(FIELD_NAME_TIME))))
      {
         PD_LOG_MSG(PDERROR, "Error parsing timestamp object [rc=%d]", rc);
         return rc;
      }
   }
   else
   {
      PD_LOG_MSG(PDERROR, "Unsupported type [%d] for argument %s", ele.type(),
                 FIELD_NAME_TIME);
      return (rc = SDB_INVALIDARG);
   }
   if ((rc = convRealToLogicalTime(timestamp, t)))
   {
      PD_LOG_MSG(PDERROR, "Stp error converting timestamp [rc=%d]", rc);
      return rc;
   }
   PD_LOG(PDINFO, "Restore target time [%llu]", *t);
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
         PD_LOG(PDERROR, "Get more results failed [rc=%d]", rc);
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

// Utility funciton to build the warning based on a nodes name/group/window
INT32 _buildNodeWarning(const bson::BSONObj &response, bson::BSONObj *output)
{
   INT32 rc = SDB_OK;
   using namespace engine::util;
   BSONObj transInfo;
   UINT64 nodeMinTime;
   UINT64 nodeMaxTime;
   ossPoolString nodeName;
   ossPoolString groupName;
   ossPoolString nodeMinTimeStr;
   ossPoolString nodeMaxTimeStr;
   if ((rc = fromBsonObj(response, FIELD_NAME_NODE_NAME, &nodeName)) ||
       (rc = fromBsonObj(response, FIELD_NAME_GROUPNAME, &groupName)) ||
       (rc = fromBsonObj(response, FIELD_NAME_TRANS_INFO, &transInfo)) ||
       (rc = fromBsonObj(transInfo, FIELD_NAME_TRANS_MIN_RECOVER_TIME,
                            &nodeMinTime)) ||
       (rc = fromBsonObj(transInfo, FIELD_NAME_TRANS_MAX_RECOVER_TIME,
                            &nodeMaxTime)))
   {
      // This should never happen
      PD_LOG(PDERROR, "Failed to parse message from node [msg=%s, rc=%d]",
             response.toPoolString().c_str(), rc);
      return (rc = SDB_SYS);
   }
   if ((rc = convLogicalTimeToRealTime(nodeMinTime, &nodeMinTimeStr)) ||
       (rc = convLogicalTimeToRealTime(nodeMaxTime, &nodeMaxTimeStr)))
   {
      PD_LOG(PDERROR,
             "Failed to convert logical time to timestamp [rc=%d]", rc);
      return rc;
   }
   try
   {
      bson::BSONObjBuilder builder;
      builder.append(FIELD_NAME_NODE_NAME, nodeName);
      builder.append(FIELD_NAME_GROUPNAME, groupName);
      builder.append(FIELD_NAME_TRANS_MIN_RECOVER_TIME, nodeMinTimeStr);
      builder.append(FIELD_NAME_TRANS_MAX_RECOVER_TIME, nodeMaxTimeStr);
      *output = builder.obj();
      PD_LOG(PDINFO, "Created node warning object %s",
             output->toPoolString().c_str());
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Exception when creating node warning [%s]",
             e.what());
      return (rc = SDB_OOM);
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
   _QueryMsg(INT32 *rc, engine::pmdEDUCB *cb, const ossPoolString &clName,
             MSG_TYPE opCode, const BSONObj &query)
       : _cb(cb), _buff(NULL), _size(0), header(NULL)
   {
      if ((*rc = msgBuildQueryMsg(&_buff, &_size, clName.c_str(), 0, 0, 0, -1,
                                  &query, NULL, NULL, NULL, cb)))
      {
         _release();
         PD_LOG(PDERROR, "Msg build failed [rc=%d]", *rc);
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
   engine::rtnContextCoord::sharePtr ptr;
   explicit _Context(engine::pmdEDUCB *cb) : _cb(cb), ptr(){};
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
   _Operator(INT32 *rc, const ossPoolString &cmdName) : ptr(NULL)
   {
      if ((*rc = engine::coordGetFactory()->create(cmdName.c_str(), ptr)))
      {
         _release();
         PD_LOG(PDERROR, "Failed to create operator [rc=%d]", *rc);
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

_coordCMDRestore::_coordCMDRestore() : _pMsg(NULL), _cb(NULL), _buf(NULL) {}

// Check the status of the cluster
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_CHECK, "_coordCMDRestore::_checkRestoreInProgress" )
INT32 _coordCMDRestore::_checkRestoreInProgress(BOOLEAN *inProgress)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_CHECK, &rc);
   // Check the DC
   BSONObj document;
   if ((rc = _queryCataDCBase(&document)))
   {
      PD_LOG(PDERROR, "Failed to get [%s] status from catalog [rc=%d]",
             FIELD_NAME_RESTORE, rc);
      return rc; // System error
   }
   BSONElement field = document.getField(FIELD_NAME_RESTORE);
   if (field.eoo())
   {
      PD_LOG(PDWARNING, "Missing field [%s] in document from catalog",
             FIELD_NAME_RESTORE);
      *inProgress = FALSE;
   }
   else
   {
      // found the field
      *inProgress = field.trueValue();
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
   BSONObj query;
   PD_LOG(PDINFO, "Setting cluster state [%s] = [%d]", FIELD_NAME_RESTORE,
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
      PD_LOG(PDERROR, "Failed to update DC state across nodes [rc=%d]", rc);
      return rc;
   }
   // Update the coord and data nodes
   if ((rc = _updateNodesState(enable)))
   {
      PD_LOG(PDERROR, "Error setting restore state on nodes [rc=%d]", rc);
      return rc;
   }
   return rc;
}

// Runs restorePrepare or restoreAbort on all nodes (not just group primaries)
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_UPDATENODES, "_coordCMDRestore::_updateNodesState" )
INT32 _coordCMDRestore::_updateNodesState(BOOLEAN enable)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_UPDATENODES, &rc);
   const ossPoolString command =
       enable ? CMD_NAME_RESTORE_PREPARE : CMD_NAME_RESTORE_ABORT;
   _Context context(_cb);                          // Auto-cleaning
   _QueryMsg msg(&rc, _cb, CMD_ADMIN_PREFIX + command, MSG_BS_QUERY_REQ,
                 BSONObj()); // Auto-cleaning
   if (rc)
   {
      PD_LOG(PDERROR, "Error creating query [rc=%d]", rc);
      return rc;
   }
   // Get the groups list
   CoordGroupList groups;
   if ((rc = _pResource->updateGroupList(groups, _cb, NULL, TRUE, TRUE, FALSE)))
   {
      PD_LOG(PDERROR, "Get data groups failed [rc=%d]", rc);
      return rc;
   }
   // Get the nodes list
   SET_ROUTEID nodes ;
   rc = coordGetGroupNodes( _pResource, _cb, BSONObj(), NODE_SEL_ALL,
                            groups, nodes, NULL, FALSE ) ;
   // Run the query
   ROUTE_RC_MAP errNodes;
   SET_ROUTEID sucNodes;
   if ((rc = executeOnNodes(msg.header, _cb, nodes, errNodes, &sucNodes, NULL,
                            NULL)))
   {
      PD_LOG(PDERROR, "Failed to update status on data nodes [rc=%d]", rc);
      return rc;
   }
   if (!errNodes.empty())
   {
      PD_LOG(PDWARNING, "Failed to update status on one or more data nodes");
      if (_buf)
      {
         *_buf = rtnContextBuf(
             coordBuildErrorObj(_pResource, rc, _cb, &errNodes, sucNodes.size()));
      }
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
      PD_LOG(PDERROR, "Failed during catalog query [rc=%d]", rc);
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
      PD_LOG(PDERROR, "Error creating operator [rc=%d]", rc);
      return rc;
   }
   _QueryMsg msg(&rc, _cb, CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC,
                 MSG_CAT_ALTER_IMAGE_REQ, query);
   if (rc)
   {
      PD_LOG(PDERROR, "Error creating query [rc=%d]", rc);
      return rc;
   }
   if ((rc = op.ptr->init(_pResource, _cb, getTimeout())))
   {
      PD_LOG(PDERROR, "Failed to init operator [rc=%d]", rc);
      return rc;
   }
   INT64 contextID;
   if ((rc = op.ptr->execute(msg.header, _cb, contextID, _buf)))
   {
      PD_LOG(PDERROR, "Failed to execute operator [rc=%d]", rc);
      return rc;
   }
   return rc;
}

// Run the given query against the data groups
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORE_QUERYDATA, "_coordCMDRestore::_queryDataGroups" )
INT32 _coordCMDRestore::_queryDataGroups(MSG_TYPE opCode,
                                         const ossPoolString &clName,
                                         const BSONObj &query, OBJ_VEC *results)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORE_QUERYDATA, &rc);
   CoordGroupList groups;
   _Context context(_cb);                          // Auto-cleaning
   _QueryMsg msg(&rc, _cb, clName, opCode, query); // Auto-cleaning
   if (rc)
   {
      PD_LOG(PDERROR, "Error creating query [rc=%d]", rc);
      return rc;
   }
   // Get the groups list
   if ((rc = _pResource->updateGroupList(groups, _cb, NULL, TRUE, TRUE, FALSE)))
   {
      PD_LOG(PDERROR, "Get data groups failed [rc=%d]", rc);
      return rc;
   }
   // Run the query
   if ((rc = executeOnDataGroup(msg.header, _cb, groups, TRUE, NULL, NULL,
                                &(context.ptr), _buf)))
   {
      PD_LOG(PDERROR, "Execute on data groups failed [rc=%d]", rc);
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
      PD_LOG(PDERROR, "Failed to gather query results [rc=%d]", rc);
      return rc;
   }
   // Check the number of results is correct
   if (results->size() != groups.size())
   {
      PD_LOG(PDERROR, "The number of results [%u] does not match the number of "
                      "groups [%u]", results->size(), groups.size());
      return (rc = SDB_SYS);
   }
   return rc;
}

/*
   coordCMDRestoreToTime definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreToTime,
                                  CMD_NAME_RESTORE_TO_TIME, FALSE);

coordCMDRestoreToTime::coordCMDRestoreToTime()
    : _targetTime(DPS_INVALID_TRANS_TIME)
{
}

// Entrypoint for restoreToTime() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_EXE, "coordCMDRestoreToTime::execute" )
INT32 coordCMDRestoreToTime::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                     INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_EXE, &rc);
   _pMsg = pMsg;
   _cb = cb;
   _buf = buf;

   if ((rc = _parseRequest()))
   {
      PD_LOG(PDERROR, "Error parsing request [rc=%d]", rc);
      return rc;
   }

   if ((rc = _checkRestore()))
   {
      PD_LOG(PDERROR, "Error checking restore state [rc=%d]", rc);
      return rc;
   }

   if ((rc = _doRestore()))
   {
      PD_LOG(PDERROR, "Error running restore [rc=%d]", rc);
      return rc;
   }

   if ((rc = _setRestoreInProgress(FALSE)))
   {
      PD_LOG(PDERROR, "Error setting restore state [rc=%d]", rc);
      return rc;
   }
   PD_LOG(PDEVENT, "restoreToTime completed successfully");
   return rc;
}

// Parse the client's request, extracting the targetTime if given
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_PARSE, "coordCMDRestoreToTime::_parseRequest" )
INT32 coordCMDRestoreToTime::_parseRequest()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_PARSE, &rc);
   // Parse the request message
   BSONObj query;
   if ((rc = extractQuery(_pMsg, &query)))
   {
      PD_LOG(PDERROR, "Extract user query failed [rc=%d]", rc);
      return rc;
   }
   // Get the value of the Time option. Value is required.
   if ((rc = _parseTime(query, &_targetTime)))
   {
      PD_LOG(PDERROR, "User query invalid [rc=%d]", rc);
      return rc;
   }
   return rc;
}

// Run restoreCheck for the given time
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_CHECK, "coordCMDRestoreToTime::_checkRestore" )
INT32 coordCMDRestoreToTime::_checkRestore()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_CHECK, &rc);

   // Run restoreCheck
   _Operator op(&rc, CMD_NAME_RESTORE_CHECK); // Auto-cleaning
   if (rc)
   {
      PD_LOG(PDERROR, "Error creating operator [rc=%d]", rc);
      return rc;
   }
   BSONObj query;
   if ((rc = extractQuery(_pMsg, &query)))
   {
      PD_LOG(PDERROR, "Extract user query failed [rc=%d]", rc);
      return rc;
   }
   _QueryMsg msg(&rc, _cb, CMD_ADMIN_PREFIX CMD_NAME_RESTORE_CHECK,
                 MSG_BS_QUERY_REQ, query);
   if (rc)
   {
      PD_LOG(PDERROR, "Error creating query [rc=%d]", rc);
      return rc;
   }
   if ((rc = op.ptr->init(_pResource, _cb, getTimeout())))
   {
      PD_LOG(PDERROR, "Failed to init operator [rc=%d]", rc);
      return rc;
   }
   INT64 contextID;
   if ((rc = op.ptr->execute(msg.header, _cb, contextID, _buf)))
   {
      PD_LOG(PDERROR, "Failed to execute operator [rc=%d]", rc);
      return rc;
   }

   // Parse the result of restoreCheck
   ossPoolString strtime;
   stpHPTime hptime;
   try
   {
      BSONObj result = BSONObj(_buf->data());
      if ((rc = util::fromBsonObj(result, FIELD_NAME_TIME, &strtime)) ||
          (rc = timeFromStr(strtime, &hptime)))
      {
         PD_LOG_MSG(PDERROR,
                    "Error extracting timestamp from restoreCheck response");
         return (rc = SDB_SYS);
      }
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create result object");
      return (rc = SDB_OOM);
   }
   if ((rc = convRealToLogicalTime(hptime, &_targetTime)))
   {
      PD_LOG_MSG(PDERROR, "Stp error converting timestamp");
      return rc;
   }
   return rc;
}

// Build the query and perform restoreToTime() on all of the data groups
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPIT_DO, "coordCMDRestoreToTime::_doRestore" )
INT32 coordCMDRestoreToTime::_doRestore()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPIT_DO, &rc);
   BSONObj query;

   // Start the transaction
   coordTransHandler trans(_cb, _pResource);
   if ((rc = trans.getRc()))
   {
      PD_LOG(PDERROR, "Failed to begin transaction for restoreToTime [rc=%d]",
             rc);
      return rc;
   }

   // Build the query
   try
   {
      BSONObjBuilder builder;
      builder.append(FIELD_NAME_GLOBAL_TIME, (INT64)_targetTime);
      builder.append(FIELD_NAME_TRANSACTION_ID_SN,
                     (INT64)(_cb->getTransID().getGlobSN()));
      builder.append(FIELD_NAME_TRANSACTION_ID_NODEID,
                     (INT32)(_cb->getTransID().getNodeID()));
      query = builder.obj();
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Failed to create query");
      return (rc = SDB_OOM);
   }

   // Call restoreToTime on the groups
   if ((rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_RESTORE_TO_TIME, query,
                              NULL)))
   {
      PD_LOG(PDERROR, "One or more nodes failed restoreToTime [rc=%d]", rc);
      // trans will perform rollback (in its destructor) because commit wasn't
      // called
      return rc;
   }

   if ((rc = trans.commit()))
   {
      PD_LOG(PDERROR, "Failed to commit restoreToTime [rc=%d]", rc);
      return rc;
   }
   return rc;
}

/*
   coordCMDRestoreCheck definitions
*/
COORD_IMPLEMENT_CMD_AUTO_REGISTER(coordCMDRestoreCheck,
                                  CMD_NAME_RESTORE_CHECK, TRUE);

coordCMDRestoreCheck::coordCMDRestoreCheck()
    : _targetTime(DPS_INVALID_TRANS_TIME), _minTime(DPS_MIN_TRANS_TIME),
      _maxTime(DPS_MAX_TRANS_TIME), _summary()
{
}

// Entrypoint for restoreCheck() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_EXE, "coordCMDRestoreCheck::execute" )
INT32 coordCMDRestoreCheck::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                    INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_EXE, &rc);
   _pMsg = pMsg;
   _cb = cb;
   _buf = buf;

   if ((rc = _parseRequest()))
   {
      PD_LOG(PDERROR, "Error parsing request [rc=%d]", rc);
      return rc;
   }

   if ((rc = _checkClusterState()))
   {
      PD_LOG(PDERROR, "Error checking cluster state [rc=%d]", rc);
      return rc;
   }

   if ((rc = _checkSession()))
   {
      PD_LOG(PDERROR, "Error checking session [rc=%d]");
      return rc;
   }

   if ((rc = _getWindow()))
   {
      PD_LOG(PDERROR, "Error getting window [rc=%d]", rc);
      return rc;
   }

   if ((rc = _setTime()))
   {
      PD_LOG(PDERROR, "Error setting time [rc=%d]", rc);
      return rc;
   }

   if ((rc = _runCheckOnNodes()))
   {
      PD_LOG(PDERROR, "Error running check on nodes [rc=%d]", rc);
      return rc;
   }

   if ((rc = _summarize()))
   {
      PD_LOG(PDERROR, "Error summarizing results [rc=%d]", rc);
      return rc;
   }

   PD_LOG(PDEVENT, "restoreCheck [Time:%llu] completed successfully",
          _targetTime);
   return rc;
}

// Parse the client's request, extracting the targetTime
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_PARSE, "coordCMDRestoreCheck::_parseRequest" )
INT32 coordCMDRestoreCheck::_parseRequest()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_PARSE, &rc);
   // Parse the request message
   BSONObj query;
   if ((rc = extractQuery(_pMsg, &query)))
   {
      PD_LOG(PDERROR, "Extract user query failed [rc=%d]", rc);
      return rc;
   }
   // Get the value of the Time option. Value is required.
   if ((rc = _parseTime(query, &_targetTime)))
   {
      PD_LOG(PDERROR, "User query invalid [rc=%d]", rc);
      return rc;
   }
   return rc;
}

// Check that the cluster is awaiting restore
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_CHECKSTATE, "coordCMDRestoreCheck::_checkClusterState" )
INT32 coordCMDRestoreCheck::_checkClusterState()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_CHECKSTATE, &rc);
   BOOLEAN inProgress;
   if ((rc = _checkRestoreInProgress(&inProgress)))
   {
      PD_LOG(PDERROR, "Error checking restore state [rc=%d]", rc);
      return rc;
   }
   if (!inProgress)
   {
      PD_LOG(PDERROR, "Cluster is not in [%s] state", FIELD_NAME_RESTORE);
      return (rc = SDB_RESTORE_NOT_IN_PROGRESS);
   }
   return rc;
}

// Check that the current session is not in a transaction
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_CHECKSESSION, "coordCMDRestoreCheck::_checkSession" )
INT32 coordCMDRestoreCheck::_checkSession()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_CHECKSESSION, &rc);
   if (_cb->isTransaction())
   {
      PD_LOG(PDERROR, "Current session in a transaction, aborting");
      return (rc = SDB_OPERATION_INCOMPATIBLE);
   }
   return rc;
}

// Query each data group for their restore window and calculate the global vals
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_GETWINDOW, "coordCMDRestoreCheck::_getWindow" )
INT32 coordCMDRestoreCheck::_getWindow()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_GETWINDOW, &rc);
   OBJ_VEC results;
   // Query the nodes for the database snapshot
   if ((rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                              BSONObj(), &results)))
   {
      PD_LOG(PDERROR,
             "Query to collect windows from data groups failed [rc=%d]", rc);
      return rc;
   }

   return (rc = _calcWindow(results));
}

// Calculate the restore window from the node snapshot results.
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_CALCWINDOW, "coordCMDRestoreCheck::_calcWindow" )
INT32 coordCMDRestoreCheck::_calcWindow(const OBJ_VEC &responses)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_CALCWINDOW, &rc);
   // Must get right side of window first, to warn users of any nodes with a
   // min that is too high
   if ((rc = _getWindowRight(responses)) ||
       (rc = _getWindowLeft(responses)))
   {
      PD_LOG(PDERROR, "Failed to get right and left side of window [rc = %d]",
             rc);
   }
   PD_LOG(PDINFO, "Restore window [%llu, %llu]", _minTime, _maxTime);
   return rc;
}

// Right side of window is min(MaxRecoverableTime of all nodes)
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_WINDOWRIGHT, "coordCMDRestoreCheck::_getWindowRight" )
INT32 coordCMDRestoreCheck::_getWindowRight(const OBJ_VEC &responses)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_WINDOWRIGHT, &rc);
   OBJ_VEC warningNodes; // nodes that need a later backup
   for (OBJ_VEC::const_iterator it = responses.begin(); it != responses.end();
        ++it)
   {
      PD_LOG(PDDEBUG, "Node response: %s", it->toPoolString().c_str());
      // Extract {TransInfo:{MaxRecoverableTime}}
      BSONObj transInfo; // subobject
      UINT64 nodeMaxRecoverableTime;
      if ((rc = fromBsonObj(*it, FIELD_NAME_TRANS_INFO, &transInfo)) ||
          (rc = fromBsonObj(transInfo, FIELD_NAME_TRANS_MAX_RECOVER_TIME,
                            &nodeMaxRecoverableTime)))
      {
         // Invalid response
         PD_LOG(PDERROR, "Failed to parse message from node [msg=%s, rc=%d]",
                it->toPoolString().c_str(), rc);
         return (rc = SDB_SYS);
      }
      // Treat DPS_INVALID_TRANS_TIME as max time - the node hasn't done any
      // transactional ops
      if (nodeMaxRecoverableTime == DPS_INVALID_TRANS_TIME)
      {
         nodeMaxRecoverableTime = DPS_MAX_TRANS_TIME;
      }
      if (nodeMaxRecoverableTime < _targetTime)
      {
         // Node needs a later backup
         INT32 tmprc = SDB_OK;
         bson::BSONObj warning;
         if ((tmprc = _buildNodeWarning(*it, &warning)))
         {
            PD_LOG(PDERROR, "Failed to build node warning [rc=%d]", tmprc);
         }
         warningNodes.push_back(warning);
      }
      _maxTime = OSS_MIN(_maxTime, nodeMaxRecoverableTime);
   }
   if (!warningNodes.empty())
   {
      // One or more nodes need a later backup
      INT32 tmprc = SDB_OK;
      ossPoolString msg =
          "One or more nodes require later backups to reach the target time";
      bson::BSONObj warningDetails;
      ossPoolString targetTimeStr;
      if ((tmprc = convLogicalTimeToRealTime(_targetTime, &targetTimeStr)))
      {
         PD_LOG(PDERROR,
                "Failed to convert logical time to timestamp [rc=%d]", tmprc);
      }
      try
      {
         bson::BSONObjBuilder builder;
         builder.append(FIELD_NAME_DETAIL, msg);
         builder.append(FIELD_NAME_TIME, targetTimeStr);
         bson::BSONArrayBuilder arrBuilder(
             builder.subarrayStart(FIELD_NAME_ERROR_NODES));
         for (OBJ_VEC::iterator it = warningNodes.begin();
              it != warningNodes.end(); ++it)
         {
            arrBuilder.append(*it);
         }
         arrBuilder.done();
         warningDetails = builder.obj();
      }
      catch (exception &e)
      {
         PD_LOG(PDERROR, "Exception when creating warning summary [%s]",
                e.what());
         return (rc = SDB_OOM);
      }
      *_buf = rtnContextBuf(warningDetails);
      PD_LOG_MSG(PDERROR, msg.c_str());
      return (rc = SDB_INVALIDARG);
   }
   return rc;
}

// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_WINDOWLEFT, "coordCMDRestoreCheck::_getWindowLeft" )
INT32 coordCMDRestoreCheck::_getWindowLeft(const OBJ_VEC &responses)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_WINDOWLEFT, &rc);
   BOOLEAN windowIsValid = TRUE;
   OBJ_VEC warningNodes; // nodes that need an earlier backup
   // Left side of window is the greatest MinRecoverableTime from all nodes.
   for (OBJ_VEC::const_iterator it = responses.begin(); it != responses.end();
        ++it)
   {
      // Extract {TransInfo:{MinRecoverableTime}}
      BSONObj transInfo; // subobject
      UINT64 nodeMinTime;
      if ((rc = fromBsonObj(*it, FIELD_NAME_TRANS_INFO, &transInfo)) ||
          (rc = fromBsonObj(transInfo, FIELD_NAME_TRANS_MIN_RECOVER_TIME,
                            &nodeMinTime)))
      {
         // This should never happen -
         PD_LOG(PDERROR, "Failed to parse message from node [msg=%s, rc=%d]",
                it->toPoolString().c_str(), rc);
         return (rc = SDB_SYS);
      }
      if (nodeMinTime > _maxTime || (DPS_INVALID_TRANS_TIME != _targetTime &&
                                     (_targetTime < nodeMinTime)))
      {
         if (nodeMinTime > _maxTime)
         {
            // There is no valid window
            windowIsValid = FALSE;
         }
         INT32 tmprc = SDB_OK;
         bson::BSONObj warning;
         if ((tmprc = _buildNodeWarning(*it, &warning)))
         {
            PD_LOG(PDERROR, "Failed to build node warning [rc=%d]", tmprc);
         }
         warningNodes.push_back(warning);
      }
      _minTime = OSS_MAX(_minTime, nodeMinTime);
   }
   if (!warningNodes.empty())
   {
      // One or more nodes need an earlier backup
      INT32 tmprc = SDB_OK;
      ossPoolString msg;
      if (windowIsValid)
      {
         msg = "Some nodes require earlier backups to reach the "
               "target time";
         rc = SDB_INVALIDARG;
      }
      else
      {
         msg = "No available global consistency point";
         rc = SDB_RESTORE_NO_CONSISTENT_PIT;
      }
      bson::BSONObj warningDetails;
      ossPoolString targetTimeStr;
      ossPoolString maxTimeStr;
      if ((tmprc = convLogicalTimeToRealTime(_targetTime, &targetTimeStr)) ||
          (tmprc = convLogicalTimeToRealTime(_maxTime, &maxTimeStr)))
      {
         PD_LOG(PDERROR,
                "Failed to convert logical time to timestamp [rc=%d]", tmprc);
      }
      try
      {
         bson::BSONObjBuilder builder;
         builder.append(FIELD_NAME_DETAIL, msg);
         builder.append(FIELD_NAME_TIME, targetTimeStr);
         builder.append(FIELD_NAME_TRANS_MAX_RECOVER_TIME, maxTimeStr);
         bson::BSONArrayBuilder arrBuilder(
             builder.subarrayStart(FIELD_NAME_ERROR_NODES));
         for (OBJ_VEC::iterator it = warningNodes.begin();
              it != warningNodes.end(); ++it)
         {
            arrBuilder.append(*it);
         }
         arrBuilder.done();
         warningDetails = builder.obj();
      }
      catch (exception &e)
      {
         PD_LOG(PDERROR, "Exception when creating warning summary [%s]",
                e.what());
         return (rc = SDB_OOM);
      }
      *_buf = rtnContextBuf(warningDetails);
      PD_LOG_MSG(PDERROR, msg.c_str());
      return rc;
   }
   PD_LOG(PDINFO, "Restore window [%llu, %llu]", _minTime, _maxTime);
   return rc;
}

// Set the time based on user input and global window
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_SETTIME, "coordCMDRestoreCheck::_setTime" )
INT32 coordCMDRestoreCheck::_setTime()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_SETTIME, &rc);
   if (_minTime > _maxTime)
   {
      PD_LOG_MSG(PDERROR, "No valid global consistency points");
      PD_LOG(PDERROR, "%s [%llu] > %s [%llu]",
             FIELD_NAME_TRANS_MIN_RECOVER_TIME,
             FIELD_NAME_TRANS_MAX_RECOVER_TIME, _minTime, _maxTime);
      return (rc = SDB_RESTORE_NO_CONSISTENT_PIT);
   }

   if (DPS_INVALID_TRANS_TIME == _targetTime)
   {
      PD_LOG(PDINFO, "Restoring to latest consistency point");
      _targetTime = _maxTime;
      return rc;
   }

   if (_targetTime < _minTime || _targetTime > _maxTime)
   {
      PD_LOG_MSG(PDERROR,
                 "Target time out of available consistency window [target: %s, "
                 "min:%s, max:%s]",
                 printLogicalTimeToRealTime(_targetTime).c_str(),
                 printLogicalTimeToRealTime(_minTime).c_str(),
                 printLogicalTimeToRealTime(_maxTime).c_str());
      return (rc = SDB_INVALIDARG);
   }

   // Given _targetTime is valid
   return rc;
}

// Perform restoreCheck() on all groups
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_RUNCHK, "coordCMDRestoreCheck::_runCheckOnNodes" )
INT32 coordCMDRestoreCheck::_runCheckOnNodes()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_RUNCHK, &rc);
   BSONObj query;

   try
   {
      query = BSON(FIELD_NAME_GLOBAL_TIME << (SINT64)_targetTime);
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Exception when creating query [%s]", e.what());
      return (rc = SDB_OOM);
   }

   if ((rc = _queryDataGroups(MSG_BS_QUERY_REQ,
                              CMD_ADMIN_PREFIX CMD_NAME_RESTORE_CHECK, query)))
   {
      PD_LOG(PDERROR, "One or more nodes failed restoreCheck [rc=%d]", rc);
      return rc;
   }

   return rc;
}

// Summarize the check results
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTORECHK_SUMMARIZE, "coordCMDRestoreCheck::_summarize" )
INT32 coordCMDRestoreCheck::_summarize()
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTORECHK_SUMMARIZE, &rc);
   /* Summary:
         {
            Time: TIMESTAMP,
            MaxRecoverableTime: TIMESTAMP,
            MinRecoverableTime: TIMESTAMP,
         }
   */
   ossPoolString targetTime, minTime, maxTime, logLimitTime;
   if ((rc = convLogicalTimeToRealTime(_targetTime, &targetTime)) ||
       (rc = convLogicalTimeToRealTime(_minTime, &minTime)) ||
       (rc = convLogicalTimeToRealTime(_maxTime, &maxTime)))
   {
      PD_LOG(PDERROR, "Time conversion error [rc=%d]", rc);
      return rc;
   }

   try
   {
      bson::BSONObjBuilder bb;
      bb.append(FIELD_NAME_TIME, targetTime);
      bb.append(FIELD_NAME_TRANS_MIN_RECOVER_TIME, minTime);
      bb.append(FIELD_NAME_TRANS_MAX_RECOVER_TIME, maxTime);
      _summary = bb.obj();
   }
   catch (exception &e)
   {
      PD_LOG(PDERROR, "Exception when creating summary [%s]", e.what());
      return (rc = SDB_OOM);
   }

   // return summary to client
   *_buf = rtnContextBuf(_summary);

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
   _pMsg = pMsg;
   _cb = cb;
   _buf = buf;

   if ((rc = _setRestoreInProgress(FALSE)))
   {
      PD_LOG(PDERROR, "Error setting restore state [rc=%d]", rc);
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

coordCMDRestorePrepare::coordCMDRestorePrepare()
{
}

// Entrypoint for restorePrepare() on the coordinator
// PD_TRACE_DECLARE_FUNCTION( COORD_RESTOREPREPARE_EXE, "coordCMDRestorePrepare::execute" )
INT32 coordCMDRestorePrepare::execute(MsgHeader *pMsg, pmdEDUCB *cb,
                                      INT64 &contextID, rtnContextBuf *buf)
{
   INT32 rc = SDB_OK;
   PD_TRACER_BEGIN(COORD_RESTOREPREPARE_EXE, &rc);
   _pMsg = pMsg;
   _cb = cb;
   _buf = buf;

   if ((rc = _setRestoreInProgress(TRUE)))
   {
      // Error, unset RestoreInProgress
      if (SDB_INVALIDARG == rc)
      {
         PD_LOG_MSG(PDERROR, "Node(s) have mvccon or globtranson disabled");
      }
      PD_LOG(PDERROR, "restorePrepare failed. Aborting. [rc=%d]", rc);
      _setRestoreInProgress(FALSE); // unset
      return rc;
   }
   PD_LOG(PDEVENT, "restorePrepare completed successfully");
   return rc;
}

} // namespace engine
