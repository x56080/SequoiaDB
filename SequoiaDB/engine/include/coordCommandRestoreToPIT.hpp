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

#ifndef COORD_COMMAND_RESTORETOPIT_HPP__
#define COORD_COMMAND_RESTORETOPIT_HPP__

#include <string>
#include <vector>

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "msg.h"
#include "ossTypes.hpp"

#include "../bson/bson.h"

namespace engine
{

/*
   coordCMDRestoreToPIT
   This is the coordinator driver for the command restoreToPIT();
*/
class coordCMDRestoreToPIT : public _coordCommandBase
{
   COORD_DECLARE_CMD_AUTO_REGISTER();

   typedef std::vector<bson::BSONObj> OBJ_VEC;

 public:
   coordCMDRestoreToPIT(){};
   virtual ~coordCMDRestoreToPIT(){};
   // execute is the entrypoint
   virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID,
                         rtnContextBuf *buf);

 protected:
   INT32 _parseRequest(MsgHeader *pMsg, UINT64 *targetTime);
   INT32 _checkStateAndRestore(pmdEDUCB *cb, UINT64 targetTime);
   INT32 _checkClusterState(pmdEDUCB *cb);
   INT32 _checkDCForState(pmdEDUCB *cb);
   INT32 _coordinateRestore(pmdEDUCB *cb, UINT64 targetTime);
   INT32 _getGlobalRestoreWindow(pmdEDUCB *cb, UINT64 *minTime,
                                 UINT64 *maxTime);
   INT32 _getMinMaxWindowFromResponses(const OBJ_VEC &responses,
                                       UINT64 *minTime, UINT64 *maxTime);
   INT32 _restoreWithWindows(pmdEDUCB *cb, UINT64 targetTime, UINT64 minTime,
                             UINT64 maxTime);
   INT32 _setTargetTimestamp(UINT64 minTime, UINT64 maxTime,
                             UINT64 *targetTime);
   INT32 _restoreDataGroups(pmdEDUCB *cb, UINT64 targetTime);
   INT32 _resetState(pmdEDUCB *cb);
   INT32 _queryCataDCBase(pmdEDUCB *cb, bson::BSONObj *result);
   INT32 _alterDC(pmdEDUCB *cb, const bson::BSONObj &query);
   INT32 _queryDataGroups(pmdEDUCB *cb, MSG_TYPE opCode,
                          const std::string &clName, const bson::BSONObj &query,
                          OBJ_VEC *results);
};

} // namespace engine

#endif // COORD_COMMAND_RESTORETOPIT_HPP__
