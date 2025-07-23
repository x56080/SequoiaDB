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

   Source File Name = sequoiaFSMsgHandler.cpp

   Descriptive Name = sequoiafs meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/
#include "sequoiaFSSession.hpp"
#include "pmd.hpp"
#include "pmdObjBase.hpp"

using namespace engine;

namespace sequoiafs
{
   BEGIN_OBJ_MSG_MAP(_fsMsgSession, _pmdAsyncSession)
      //ON_MSG
   END_OBJ_MSG_MAP()
   
   _fsMsgSession::_fsMsgSession(UINT64 sessionID)
   :_pmdAsyncSession (sessionID),
    _quit( FALSE ),
    _timeout( 0 )
   {
   }

   _fsMsgSession::~_fsMsgSession()
   {
   }
         
   SDB_SESSION_TYPE _fsMsgSession::sessionType() const
   {
      return SDB_SESSION_FS_SRC;
   }

   EDU_TYPES _fsMsgSession::eduType() const
   {
      return EDU_TYPE_REPLAGENT;
   }

   BOOLEAN _fsMsgSession::timeout(UINT32 interval)
   {
      return _quit ;
   }

   void _fsMsgSession::onRecieve(const NET_HANDLE netHandle,
                                  MsgHeader *msg)
   {
      ;
   }

   void _fsMsgSession::onTimer(UINT64 timerID, UINT32 interval)
   {
      return  ;
   }

   void _fsMsgSession::_onAttach()
   {
      ;
   }

   void _fsMsgSession::_onDetach()
   {
      ;
   }

}

