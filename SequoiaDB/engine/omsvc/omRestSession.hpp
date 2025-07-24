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

   Source File Name = omRestSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/29/2015  Lin YouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_REST_SESSION_HPP_
#define OM_REST_SESSION_HPP_

#include "pmdRestSession.hpp"
#include "omCommandInterface.hpp"
#include "omTransferProcessor.hpp"

using namespace bson ;

namespace engine
{
   /*
      _omRestSession define
   */
   class _omRestSession : public _pmdRestSession
   {
      public:
         _omRestSession( SOCKET fd ) ;
         virtual ~_omRestSession() ;

         virtual SDB_SESSION_TYPE sessionType() const ;

      protected:
         virtual INT32     _processMsg( HTTP_PARSE_COMMON command, 
                                        const CHAR *pFilePath ) ;

      protected:
         INT32             _processOMRestMsg( HTTP_PARSE_COMMON command,
                                              const CHAR *pFilePath ) ;
         INT32             _setSpecifyNode( const CHAR *pSdbHostName,
                                            const CHAR *pSdbSvcName,
                                            list<omNodeInfo> &nodeList ) ;
         INT32             _processSdbTransferMsg( restAdaptor *pAdaptor,
                                                const CHAR *pClusterName,
                                                const CHAR *pBusinessName ) ;
         INT32             _getBusinessAccessNode( const CHAR *pClusterName,
                                                   const CHAR *pBusinessName,
                                                   const CHAR *pSdbUser,
                                                   const CHAR *pSdbPasswd,
                                                  list<omNodeInfo> &nodeList ) ;
         INT32             _getBusinessAuth( const CHAR *pClusterName,
                                             const CHAR *pBusinessName,
                                             string &user, string &passwd ) ;
         INT32             _getBusinessInfo( const CHAR *pClusterName,
                                             const CHAR *pBusinessName,
                                             string &businessType, 
                                             string &deployMode ) ;
         INT32             _queryTable( const string &tableName, 
                                        const BSONObj &selector, 
                                        const BSONObj &matcher,
                                        const BSONObj &order, 
                                        const BSONObj &hint, SINT32 flag,
                                        SINT64 numSkip, SINT64 numReturn, 
                                        list<BSONObj> &records ) ;
         BOOLEAN           _isClusterExist( const CHAR *pClusterName ) ;

         omRestCommandBase *_createSsqlCommand( restAdaptor *pAdaptor,
                                                const string &localAgentHost,
                                                const string &localAgentPort ) ;

         INT32             _processSsqlTransferMsg( restAdaptor *pAdaptor,
                                                    const CHAR *pClusterName,
                                                    const CHAR *pBusinessName ) ;

      private:
         omRestCommandBase *_createCommand( HTTP_PARSE_COMMON command,
                                            const CHAR *pFilePath ) ;

   } ;
   typedef _omRestSession omRestSession ;

}

#endif //OM_REST_SESSION_HPP_



