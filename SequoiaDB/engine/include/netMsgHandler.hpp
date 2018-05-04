/******************************************************************************


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
        virtual INT32   handleMsg( const NET_HANDLE &handle,
                                   const _MsgHeader *header,
                                   const CHAR *msg ) = 0 ;

        virtual void    handleClose( const NET_HANDLE &handle,
                                     _MsgRouteID id )
        {
        }

        virtual void    handleConnect( const NET_HANDLE &handle,
                                       _MsgRouteID id,
                                       BOOLEAN isPositive )
        {
        }

        virtual void    onStop() {}

   } ;

   typedef _netMsgHandler INetMsgHandler ;

}

#endif // NETMSGHANDLER_HPP_

