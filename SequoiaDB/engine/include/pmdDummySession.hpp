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

   Source File Name = pmdDummySession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date     Who     Description
   ====== ======== ======= ==============================================
          2020/8/8 Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_DUMMY_SESSION_HPP_
#define PMD_DUMMY_SESSION_HPP_

#include "pmdSessionBase.hpp"
#include "pmdInnerClient.hpp"

namespace engine
{
   /*
      _pmdDummySession define
   */
   class _pmdDummySession : public _pmdSessionBase
   {
   public:
      _pmdDummySession( BOOLEAN isBusinessSession = FALSE )
      : _pEDUCB( NULL ),
        _eduID( PMD_INVALID_EDUID ),
        _isBusinessSession( isBusinessSession )
      {
      }

      virtual ~_pmdDummySession() {}

   public:
      virtual UINT64           identifyID()    { return 0 ; }
      virtual MsgRouteID       identifyNID() ;
      virtual UINT32           identifyTID()   { return 0 ; }
      virtual UINT64           identifyEDUID() { return 0 ; }

      virtual const CHAR*      sessionName() const { return "Dummy-Session" ; }
      virtual SDB_SESSION_TYPE sessionType() const { return SDB_SESSION_DUMMY ; }
      virtual BOOLEAN          isBusinessSession() const { return _isBusinessSession ; }
      virtual INT32            getServiceType() const { return 0 ; }
      virtual IClient*         getClient()            { return &_client ; }

      virtual void*            getSchedItemPtr()            { return NULL ; }
      virtual void             setSchedItemVer( INT32 ver ) { return ; }

      virtual _pmdEDUCB*       eduCB () const { return _pEDUCB ; }
      virtual EDUID            eduID () const { return _eduID ; }

      void                     attachCB ( _pmdEDUCB *cb ) ;
      void                     detachCB () ;

   protected:
      _pmdEDUCB               *_pEDUCB ;
      EDUID                    _eduID ;
      pmdInnerClient           _client ;
      BOOLEAN                  _isBusinessSession ;
   } ;

   typedef _pmdDummySession pmdDummySession ;
}

#endif //PMD_DUMMY_SESSION_HPP_

