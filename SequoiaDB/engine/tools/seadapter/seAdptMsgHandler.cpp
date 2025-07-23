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

   Source File Name = seAdptMsgHandler.cpp

   Descriptive Name = Search Engine Adapter Message Handler.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptMsgHandler.hpp"

namespace seadapter
{
   _indexMsgHandler::_indexMsgHandler( _pmdAsycSessionMgr *pSessionMgr )
   : _pmdAsyncMsgHandler( pSessionMgr )
   {
   }

   _indexMsgHandler::~_indexMsgHandler()
   {
   }

   void _indexMsgHandler::handleClose( const NET_HANDLE &handle,
                                       _MsgRouteID id )
   {
      if ( _pMgrEDUCB )
      {
         MsgOpReply *pMsg = NULL ;
         pMsg = ( MsgOpReply* )SDB_OSS_MALLOC( sizeof( MsgOpReply ) ) ;
         if ( !pMsg )
         {
            PD_LOG( PDERROR, "Alloc memory[size: %d] failed",
                    sizeof( MsgOpReply ) ) ;
         }
         else
         {
            pMsg->contextID = -1 ;
            pMsg->flags = SDB_NETWORK_CLOSE ;
            pMsg->header.messageLength = sizeof( MsgOpReply ) ;
            pMsg->header.opCode = MSG_COM_REMOTE_DISC ;
            pMsg->header.requestID = 0 ;
            pMsg->header.routeID.value = id.value ;
            pMsg->header.TID = 0 ;
            pMsg->numReturned = 0 ;
            pMsg->startFrom = 0 ;

            _pMgrEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                                PMD_EDU_MEM_ALLOC,
                                                pMsg, (UINT64)handle ) ) ;
         }
      }
   }
}

