/*******************************************************************************

<<<<<<< HEAD
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
=======
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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   Source File Name = sequoiaFSMCSMsgHandler.hpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#ifndef __SEQUOIAFSMCSHANDLER_HPP__
#define __SEQUOIAFSMCSHANDLER_HPP__

#include "pmdAsyncHandler.hpp"
#include "netMsgHandler.hpp"
#include "netDef.hpp"
#include "sequoiaFSCommon.hpp"

using namespace engine;

#define MCSTimer 1

namespace sequoiafs
{
   class sequoiaFSMCS;
   class mcsMsghandler : public _pmdAsyncMsgHandler
   {
      public:
         mcsMsghandler(_pmdAsycSessionMgr *pSessionMgr,
                       sequoiaFSMCS *mcs);
         virtual ~mcsMsghandler();
         
         virtual INT32 handleMsg(const NET_HANDLE &handle,
                       const _MsgHeader *header,
                       const CHAR *msg );
         virtual void  handleClose(const NET_HANDLE &handle, 
                            _MsgRouteID id );
         INT32 handleRegRequest(const NET_HANDLE &handle, 
                                const _mcsRegReq *request);
         INT32 handleNotifyRequest(const NET_HANDLE &handle, 
                                   const _mcsNotifyReq *request);

      private:
         sequoiaFSMCS *_mcs;
   };

   class _mcsTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _mcsTimerHandler (_pmdAsycSessionMgr * pSessionMgr);
         virtual ~_mcsTimerHandler();

      protected:
         virtual UINT64  _makeTimerID(UINT32 timerID);
   } ;
}

#endif
