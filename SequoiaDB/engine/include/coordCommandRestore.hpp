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

#ifndef COORD_COMMAND_RESTORE_HPP__
#define COORD_COMMAND_RESTORE_HPP__

#include <vector>

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "coordTransOperator.hpp"
#include "msg.h"
#include "ossMemPool.hpp"
#include "ossTypes.hpp"

#include "../bson/bson.h"

namespace engine
{

/*
   _coordCMDRestore
   Abstract base class for coordinator restore commands.
*/
class _coordCMDRestore : public _coordCommandBase
{
 protected:
   typedef std::vector<bson::BSONObj> OBJ_VEC;

   INT32 _checkRestoreInProgress(BOOLEAN *inProgress);
   INT32 _setRestoreInProgress(BOOLEAN enable);
   INT32 _setRestoreInProgressNodes(BOOLEAN enable, BOOLEAN coord,
                                    BOOLEAN data);
   INT32 _queryCataDCBase(bson::BSONObj *result);
   INT32 _alterDC(const bson::BSONObj &query);
   INT32 _queryDataGroups(MSG_TYPE opCode, const ossPoolString &clName,
                          const bson::BSONObj &query,
                          OBJ_VEC *results = NULL);
   INT32 _cmdCoords(MSG_TYPE opCode, const ossPoolString &clName,
                    const BSONObj &query);

 protected:
   MsgHeader *_pMsg;
   pmdEDUCB *_cb;
};

/*
   coordCMDRestoreToTime
   Coordinator handler for restoreToTime().
   The cluster must be in RestoreInProgress state.
   Performs restoreToTime() on data nodes and resets the cluster state.
*/
class coordCMDRestoreToTime : public _coordCMDRestore
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

 public:
   coordCMDRestoreToTime(){};
   virtual ~coordCMDRestoreToTime(){};
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);

 protected:
   INT32 _parseRequest(UINT64 *targetTime);
   INT32 _parseTime(const BSONObj &query, UINT64 *targetTime);
   INT32 _targetTimeFromTimestamp(const BSONObj &query, UINT64 *targetTime);
   INT32 _checkStateAndRestore(UINT64 targetTime);
   INT32 _coordinateRestore(UINT64 targetTime);
   INT32 _getGlobalRestoreWindow(UINT64 *minTime, UINT64 *maxTime);
   INT32 _getMinMaxWindowFromResponses(const OBJ_VEC &responses,
                                       UINT64 *minTime, UINT64 *maxTime);
   INT32 _restoreWithWindows(UINT64 targetTime, UINT64 minTime, UINT64 maxTime);
   INT32 _setTargetTime(UINT64 minTime, UINT64 maxTime, UINT64 *targetTime);
   INT32 _generateQueryAndRestore(UINT64 targetTime, BOOLEAN test);
   INT32 _buildRestoreQuery(UINT64 targetTime, BOOLEAN test, BSONObj *query);

 protected:
   BOOLEAN _optTestOnly;
   BOOLEAN _optSkipTest;
};

/*
   coordCMDRestoreAbort
   Coordinator handler for restoreAbort();
   The cluster must be in RestoreInProgress state.
   Resets the cluster state.
*/
class coordCMDRestoreAbort : public _coordCMDRestore
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

 public:
   coordCMDRestoreAbort(){};
   virtual ~coordCMDRestoreAbort(){};
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);
};

/*
   coordCMDRestorePrepare
   Coordinator handler for restorePrepare();
   The cluser must not be in RestooreInProgress state.
   The cluster will enter the RestoreInProgress state.
*/
class coordCMDRestorePrepare : public _coordCMDRestore
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

 public:
   coordCMDRestorePrepare(){};
   virtual ~coordCMDRestorePrepare(){};
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);
};

} // namespace engine

#endif // COORD_COMMAND_RESTORE_HPP__
