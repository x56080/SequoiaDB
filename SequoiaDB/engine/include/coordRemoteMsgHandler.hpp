/*******************************************************************************

<<<<<<< HEAD
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
=======

   Copyright (C) 2011-2021 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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
<<<<<<< HEAD
                                   BOOLEAN isPositive ) ;
=======
                                   BOOLEAN isPositive,
                                   netUserDataHolder *userDataHolder ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   private:
      _coordDataSourceMgr     *_pDSMgr ;

   } ;
   typedef _coordDataSourceMsgHandler coordDataSourceMsgHandler ;

}

#endif /* COORD_REMOTEMSGHANDLER_HPP__ */

