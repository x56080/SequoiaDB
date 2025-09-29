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

   Source File Name = pmdRemoteConnection.hpp

   Descriptive Name = pmd remote Connection

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
#ifndef PMD_REMOTE_CONNECTION_HPP__
#define PMD_REMOTE_CONNECTION_HPP__

#include "oss.hpp"
#include "netRouteAgent.hpp"

namespace engine
{

   /*
      _pmdRemoteConnection define
   */
   class _pmdRemoteConnection : public SDBObject
   {
      public:
         _pmdRemoteConnection() ;
         ~_pmdRemoteConnection() ;

         INT32 init( netRouteAgent *agent,
                     BOOLEAN isConnExtern,
                     const MsgRouteID &routeID,
                     const NET_HANDLE &handle ) ;

         BOOLEAN isConnected() const ;
         const NET_HANDLE& getNetHandle() const ;
         const MsgRouteID& getRouteID() const ;
         BOOLEAN isExtern() const ;
         netRouteAgent* getRouteAgent() ;

         void  disconnect() ;
         INT32 connect() ;

         void  forceClose() ;

         /*
            Send failed, will set handle to invalid.
         */
         INT32 syncSend( MsgHeader *header ) ;
         INT32 syncSend( MsgHeader *header, void *body, UINT32 bodyLen ) ;
         INT32 syncSendv( MsgHeader *header, const netIOVec &iov ) ;

      private:
         netRouteAgent     *_routeAgent ;
         MsgRouteID        _routeID ;
         NET_HANDLE        _handle ;
         BOOLEAN           _isConnExtern ;   // Whether it's a connection to
                                             // a extern cluster node.
   } ;
   typedef _pmdRemoteConnection pmdRemoteConnection ;

}

#endif /* PMD_REMOTE_CONNECTION_HPP__ */

