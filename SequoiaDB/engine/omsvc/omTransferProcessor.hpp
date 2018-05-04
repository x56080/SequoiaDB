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

   Source File Name = omTransferProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/08/2015  Lin YouBin Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_TRNASFERPROCESSOR_HPP__
#define OM_TRNASFERPROCESSOR_HPP__

#include "pmdProcessor.hpp"
#include "pmdRemoteSession.hpp"
#include "omSdbConnector.hpp"
#include <map>
#include <string>

using namespace bson;

namespace engine
{
   typedef struct _omNodeInfo 
   {
      string hostName ;
      string service ;
      string user ;
      string passwd ;
      INT32 preferedInstance ;

      _omNodeInfo() 
      {
         hostName = "" ;
         service  = "" ;
         user     = "" ;
         passwd   = "" ;
         preferedInstance = 1 ;
      }

      _omNodeInfo( const _omNodeInfo& nodeInfo )
      {
         hostName = nodeInfo.hostName ;
         service  = nodeInfo.service ;
         user     = nodeInfo.user ;
         passwd   = nodeInfo.passwd ;
         preferedInstance = nodeInfo.preferedInstance ;
      }

   } omNodeInfo ;

   class _omTransferProcessor : public pmdProcessor
   {
      public:
         _omTransferProcessor( list<_omNodeInfo> &nodeList ) ;

         virtual            ~_omTransferProcessor() ;

          

      public:
         virtual INT32                 processMsg( MsgHeader *msg,
                                                   rtnContextBuf &contextBuff,
                                                   INT64 &contextID,
                                                   BOOLEAN &needReply ) ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

         void                          attach( pmdSession *session ) ;
         void                          detach() ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

      protected:
         void                          _clearRemoteSession( 
                                             pmdRemoteSessionMgr *rsManager,
                                             pmdRemoteSession *remoteSession ) ;
         INT32                         _sendMsg2Target( 
                                                     const omNodeInfo &nodeInfo, 
                                                     MsgHeader *msg, 
                                                     omSdbConnector **connector,
                                                     MsgHeader **result ) ;

      private:
         list< _omNodeInfo >           _nodeList ;
   };
}

#endif /* OM_TRNASFERPROCESSOR_HPP__ */



