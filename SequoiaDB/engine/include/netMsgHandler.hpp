/******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef NETMSGHANDLER_HPP_
#define NETMSGHANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"

namespace engine
{
   /*
      _netMsgHandler define
   */
   class _netMsgHandler : public SDBObject
   {
      public:
        _netMsgHandler(){}
        virtual ~_netMsgHandler(){}
      public:
        // callback to handle message
        // - handle: net handle
        // - header: header of message
        // - msg: content of message
        // - msgUserData: user data with the receive message
        virtual INT32   handleMsg( const NET_HANDLE &handle,
                                   const _MsgHeader *header,
                                   const CHAR *msg,
                                   UINT64 msgUserData ) = 0 ;

        // callback to handle connection close
        // - handle: net handle
        // - id: route ID to peer node
        virtual void    handleClose( const NET_HANDLE &handle,
                                     _MsgRouteID id )
        {
        }

        // callback to handle connection establish
        // - handle: net handle
        // - id: route ID to peer node
        // - isPositive: TRUE means connection is launched from this node
        //               FALSE means connection is launched from peer node
        // - userDataHolder: holder of user data, pass user data to handle
        //                   session, may create user data of type specified
        //                   by handle session
        virtual INT32 handleConnect( const NET_HANDLE &handle,
                                     _MsgRouteID id,
                                     BOOLEAN isPositive,
                                     netUserDataHolder *userDataHolder )
        {
           return SDB_OK ;
        }

        // callback on before sending message
        // - handle: net handle
        // - id: route ID to peer node
        // - header: message header to send
        virtual INT32   onSendMsg( const NET_HANDLE &handle,
                                   const MsgRouteID &id,
                                   MsgHeader *header )
        {
           return SDB_OK ;
        }

        // callback on after receiving message
        // - handle: net handle
        // - id: route ID to peer node
        // - header: message header to send
        // - availableSize: size of available messages in socket ( including
        //                  current message and blocking messages )
        // - userDataHolder: holder of user data, pass user data to handle
        //                   session
        virtual INT32   onReceiveMsg( const NET_HANDLE &handle,
                                      const MsgRouteID &id,
                                      MsgHeader *header,
                                      UINT32 availableSize,
                                      netUserDataHolder *userDataHolder )
        {
           return SDB_OK ;
        }

        // callback on IO service stop
        virtual void    onStop() {}
   } ;

   typedef _netMsgHandler INetMsgHandler ;

}

#endif // NETMSGHANDLER_HPP_
