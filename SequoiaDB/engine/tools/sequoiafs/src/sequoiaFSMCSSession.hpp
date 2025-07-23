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

   Source File Name = sequoiaFSMsgHandler.hpp

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
#ifndef __SEQUOIAFSMCSSESSION_HPP__
#define __SEQUOIAFSMCSSESSION_HPP__

#include "pmdAsyncSession.hpp"

using namespace engine;

namespace sequoiafs
{
   class _mcsMsgSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      public:
         _mcsMsgSession(UINT64 sessionID);
         virtual ~_mcsMsgSession();

         virtual SDB_SESSION_TYPE sessionType() const;
         virtual const CHAR*      className() const { return "mcs-msg" ; }

         virtual EDU_TYPES eduType () const ;
         virtual BOOLEAN canAttachMeta() const { return FALSE ; }

         // called by net io thread
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         // called by self thread
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;
         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      private:
         _dpsLogWrapper                *_logger ;

         MsgRouteID                    _syncSrc ;
         MsgRouteID                    _lastSyncNode ;
         BOOLEAN                       _quit ;

         BOOLEAN                       _isFirstToSync ;
         UINT32                        _timeout ;
         UINT64                        _requestID ;
         UINT32                        _syncFailedNum ;

         UINT32                        _fullSyncIgnoreTimes ;
   } ;

}

#endif
