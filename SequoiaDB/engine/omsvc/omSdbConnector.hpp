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

   Source File Name = omSdbConnector.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_SDB_CONNECTOR_HPP_
#define OM_SDB_CONNECTOR_HPP_

#include "oss.hpp"
#include "msg.h"
#include "msgDef.h"
#include "ossSocket.hpp"
#include "../bson/bson.h"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   /*
      _omSdbConnector define
   */
   class _omSdbConnector : public SDBObject
   {
      public:
         _omSdbConnector() ;
         ~_omSdbConnector() ;
         INT32    init( const string &hostName, UINT32 port, 
                        const string &user, const string &passwd,
                        INT32 preferedInstance ) ;

         INT32    sendMessage( const MsgHeader *msg ) ;
         INT32    recvMessage( MsgHeader **msg ) ;

         INT32    close() ;

      private:
         INT32    _connect( const CHAR *host, UINT32 port ) ;
         INT32    _sendRequest( const CHAR *request, INT32 reqSize ) ;
         INT32    _recvResponse( CHAR *response, INT32 resSize ) ;
         INT32    _requestSysInfo() ;
         INT32    _authority( const string &user, const string &passwd ) ;
         INT32    _negotiation( const string &user, const string &passwd ) ;
         INT32    _setAttr( INT32 preferedInstance ) ;

      private:
         string      _hostName ;
         UINT32      _port ;
         _ossSocket  *_pSocket ;
         BOOLEAN     _init ;
   } ;
   
   typedef _omSdbConnector omSdbConnector ;

}

#endif //OM_SDB_CONNECTOR_HPP_

