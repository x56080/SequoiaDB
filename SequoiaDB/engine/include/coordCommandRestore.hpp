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
 public:
   _coordCMDRestore();

 protected:
   typedef std::vector<bson::BSONObj> OBJ_VEC;

   INT32 _checkRestoreInProgress(BOOLEAN *inProgress);
   INT32 _setRestoreInProgress(BOOLEAN enable);
   INT32 _updateNodesState(BOOLEAN enable);
   INT32 _queryCataDCBase(bson::BSONObj *result);
   INT32 _alterDC(const bson::BSONObj &query);
   INT32 _queryDataGroups(MSG_TYPE opCode, const ossPoolString &clName,
                          const bson::BSONObj &query,
                          OBJ_VEC *results = NULL);

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
   coordCMDRestoreToTime();
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
class coordCMDRestoreCheck : public _coordCMDRestore
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

 public:
   coordCMDRestoreCheck();
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);

 protected:
   INT32 _parseRequest();
   INT32 _checkClusterState();
   INT32 _checkSession();
   INT32 _getWindow();
   INT32 _calcWindow(const OBJ_VEC &responses);
   INT32 _getWindowRight(const OBJ_VEC &responses);
   INT32 _getWindowLeft(const OBJ_VEC &responses);
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
   coordCMDRestorePrepare();
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);
};

} // namespace engine

#endif // COORD_COMMAND_RESTORE_HPP__
