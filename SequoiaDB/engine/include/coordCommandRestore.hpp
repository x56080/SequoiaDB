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
#include "coordCommandCommon.hpp"
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
   rtnContextBuf *_buf;
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
   INT32 _parseRequest();
   INT32 _checkRestore();
   INT32 _doRestore();

 protected:
   UINT64 _targetTime;
};

/*
   coordCMDRestoreCheck
   Coordinator handler for restoreCheck().
   The cluster must be in RestoreInProgress state.
   Performs restoreCheck() on data nodes and returns the valid window.
*/
class coordCMDRestoreCheck : public _coordCMDRestore, public _coordAggrCmdBase
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

 public:
   coordCMDRestoreCheck(){};
   virtual ~coordCMDRestoreCheck(){};
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);

 protected:
   INT32 _parseRequest();
   INT32 _checkClusterState();
   INT32 _getWindow();
   INT32 _calcWindow(const OBJ_VEC &responses);
   INT32 _setTime();
   INT32 _runCheckOnNodes();
   INT32 _summarize();

 private:
   UINT64 _targetTime;
   UINT64 _minTime;
   UINT64 _maxTime;
   BSONObj _summary;
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
