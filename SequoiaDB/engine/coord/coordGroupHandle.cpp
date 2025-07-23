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

   Source File Name = coordGroupHandle.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordGroupHandle.hpp"

namespace engine
{

   /*
      _coordGroupHandler implement
   */
   _coordGroupHandler::_coordGroupHandler()
   {
   }

   _coordGroupHandler::~_coordGroupHandler()
   {
   }

   void _coordGroupHandler::prepareForSend( pmdSubSession *pSub,
                                            _coordGroupSel *pSel,
                                            _coordGroupSessionCtrl *pCtrl )
   {
      if ( !pSel->isPrimary() && pSel->isPreferedPrimary() )
      {
         MsgHeader *pMsg = pSub->getReqMsg() ;
         BOOLEAN isResend = pCtrl->getRetryTimes() > 0 ? TRUE : FALSE ;

         if ( MSG_BS_QUERY_REQ == pMsg->opCode )
         {
            MsgOpQuery *pQuery = ( MsgOpQuery* )pMsg ;
            if ( FALSE == isResend )
            {
               pQuery->flags |= FLG_QUERY_PRIMARY ;
            }
            else
            {
               pQuery->flags &= ~FLG_QUERY_PRIMARY ;
            }
         }
         else if ( pMsg->opCode > ( SINT32 )MSG_LOB_BEGIN &&
                   pMsg->opCode < ( SINT32 )MSG_LOB_END )
         {
            MsgOpLob *pLobMsg = ( MsgOpLob* )pMsg ;
            if ( FALSE == isResend )
            {
               pLobMsg->flags |= FLG_LOBREAD_PRIMARY ;
            }
            else
            {
               pLobMsg->flags &= ~FLG_LOBREAD_PRIMARY ;
            }
         }
      }
   }

}

