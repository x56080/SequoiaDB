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

   Source File Name = netRoute.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef NETROUTE_HPP_
#define NETROUTE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "netDef.hpp"
#include <map>

using namespace std ;

namespace engine
{

   class _netRoute : public SDBObject
   {
      public:
         _netRoute() ;
         ~_netRoute() ;
         INT32 route( const _MsgRouteID &id,
                      CHAR *host, UINT32 hostLen,
                      CHAR *service, UINT32 svcLen ) ;

         INT32 route( const _MsgRouteID &id,
                      _netRouteNode &node ) ;

         INT32 route( const CHAR* host,
                      const CHAR* service,
                      MSG_ROUTE_SERVICE_TYPE type,
                      _MsgRouteID &id ) ;

         INT32 route( const MsgRouteID &routeID,
                      netUDPEndPoint &endPoint ) ;

         /// return err when update an existing node.
         INT32 update( const _MsgRouteID &id,
                       const CHAR *host,
                       const CHAR *service,
                       BOOLEAN *newAdd = NULL ) ;
         INT32 update( const _MsgRouteID &id,
                       const _netRouteNode &node,
                       BOOLEAN *newAdd = NULL ) ;
         INT32 update( const _MsgRouteID &oldID,
                       const _MsgRouteID &newID ) ;

         void  del( const _MsgRouteID &id, BOOLEAN &hasDel ) ;

         void  clear() ;

         OSS_INLINE void setLocal( const _MsgRouteID &id )
         {
            _local = id ;
         }

         OSS_INLINE const _MsgRouteID &local()
         {
            return _local ;
         }

         static INT32 getUDPEndPoint( const CHAR *hostName,
                                       const CHAR *serviceName,
                                       netUDPEndPoint &endPoint ) ;
         static INT32 getTCPEndPoint( const CHAR *hostName,
                                      const CHAR *serviceName,
                                      netTCPEndPoint &endPoint ) ;

      protected :
         NET_ROUTE_MAP     _route ;
         MsgRouteID        _local ;
         ossSpinSLatch     _mtx ;
   } ;

   typedef class _netRoute netRoute ;

}

#endif

