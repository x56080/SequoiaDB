/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pmdSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_SESSION_HPP_
#define PMD_SESSION_HPP_

#include "pmdSessionBase.hpp"
#include "msg.h"
#include "pmdDef.hpp"
#include "rtnContext.hpp"

#include <map>
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   class _rtnContextBuf ;

   /*
      _pmdLocalSession define
   */
   class _pmdLocalSession : public _pmdSession
   {
      public:
         _pmdLocalSession( SOCKET fd ) ;
         virtual ~_pmdLocalSession () ;

         virtual INT32     getServiceType() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;

         virtual INT32     run() ;

      protected:
         INT32          _processMsg( MsgHeader *msg ) ;
         virtual INT32  _onMsgBegin( MsgHeader *msg ) ;
         virtual void   _onMsgEnd( INT32 result, MsgHeader *msg ) ;

         INT32          _recvSysInfoMsg( UINT32 msgSize, CHAR **ppBuff,
                                         INT32 &buffLen ) ;
         INT32          _processSysInfoRequest( const CHAR *msg ) ;

         INT32          _reply( MsgOpReply* responseMsg, const CHAR *pBody,
                                INT32 bodyLen ) ;

      protected:
         virtual void            _onAttach () ;
         virtual void            _onDetach () ;

      // message process functions
      protected:

      protected:
         MsgOpReply           _replyHeader ;
         BOOLEAN              _needReply ;
         BOOLEAN              _needRollback ;

         BSONObj              _errorInfo ;

   } ;
   typedef _pmdLocalSession pmdLocalSession ;

}

#endif //PMD_SESSION_HPP_

