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

   Source File Name = pmdInnerClient.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/11/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_INNER_CLIENT_HPP_
#define PMD_INNER_CLIENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "pmdDef.hpp"
#include "netDef.hpp"

#include <string>

using namespace std ;

namespace engine
{

   class _pmdEDUCB ;
   class _netRouteAgent ;

   /*
      _pmdInnerClient define
   */
   class _pmdInnerClient : public IClient
   {
      public:
         _pmdInnerClient() ;
         virtual ~_pmdInnerClient() ;

         void attachCB( _pmdEDUCB *cb ) { _pEDUCB = cb ; }
         void detachCB() { _pEDUCB = NULL ; }

         void setClientInfo( _netRouteAgent *pNetRouter, NET_HANDLE handle ) ;
         void enableAuth( BOOLEAN bAuth ) { _isAuthed = bAuth ; }

         void setFromInfo( const CHAR *ip, UINT16 port ) ;

      public:
         virtual SDB_CLIENT_TYPE clientType() const ;
         virtual const CHAR*     clientName() const ;

         virtual INT32        authenticate( MsgHeader *pMsg ) ;
         virtual INT32        authenticate( const CHAR *username,
                                            const CHAR *password ) ;
         virtual void         logout() ;
         virtual INT32        disconnect() ;

         virtual BOOLEAN      isAuthed() const ;
         virtual BOOLEAN      isConnected() const ;
         virtual BOOLEAN      isClosed() const ;

         virtual UINT16       getLocalPort() const ;
         virtual const CHAR*  getLocalIPAddr() const ;
         virtual UINT16       getPeerPort() const ;
         virtual const CHAR*  getPeerIPAddr() const ;
         virtual const CHAR*  getUsername() const ;
         virtual const CHAR*  getPassword() const ;

         virtual const CHAR*  getFromIPAddr() const ;
         virtual UINT16       getFromPort() const ;

      public:
         _netRouteAgent*      getNetAgent() { return _pRTAgent ; }

      protected:
         void                 _makeName() ;

      protected:
         BOOLEAN              _isAuthed ;
         _netRouteAgent       *_pRTAgent ;
         NET_HANDLE           _netHandle ;
         _pmdEDUCB*           _pEDUCB ;
         string               _username ;
         string               _password ;

         UINT16               _localPort ;
         UINT16               _peerPort ;
         CHAR                 _localIP[ PMD_IPADDR_LEN + 1 ] ;
         CHAR                 _peerIP[ PMD_IPADDR_LEN + 1 ] ;
         CHAR                 _clientName[ PMD_CLIENTNAME_LEN + 1 ] ;

         CHAR                 _fromIP[ PMD_IPADDR_LEN + 1 ] ;
         UINT16               _fromPort ;

   } ;
   typedef _pmdInnerClient pmdInnerClient ;

}

#endif //PMD_INNER_CLIENT_HPP_

