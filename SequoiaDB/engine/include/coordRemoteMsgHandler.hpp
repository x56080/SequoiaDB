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

   Source File Name = coordRemoteMsgHandler.hpp

   Descriptive Name = Remote message handler on coordinator.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_REMOTEMSGHANDLER_HPP__
#define COORD_REMOTEMSGHANDLER_HPP__

#include "pmdRemoteMsgEventHandler.hpp"

namespace engine
{
   class _coordDataSourceMgr ;

   /**
    * _coordDataSourceMsgHandler define
    * Data source message handler for coordinator, used to handle network events
    * with data source. It's binded to the net route agent for data source when
    * data source manager is initialized.
   */
   class _coordDataSourceMsgHandler : public _pmdRemoteMsgHandler
   {
   public:
      _coordDataSourceMsgHandler( _pmdRemoteSessionMgr *pRSManager,
                                  _coordDataSourceMgr *pDSMgr ) ;
      virtual ~_coordDataSourceMsgHandler() ;

      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

      virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

      virtual INT32 handleConnect( const NET_HANDLE &handle,
                                   _MsgRouteID id,
                                   BOOLEAN isPositive ) ;

   private:
      _coordDataSourceMgr     *_pDSMgr ;

   } ;
   typedef _coordDataSourceMsgHandler coordDataSourceMsgHandler ;

}

#endif /* COORD_REMOTEMSGHANDLER_HPP__ */

