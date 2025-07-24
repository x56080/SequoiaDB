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

   Source File Name = coordSession.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORDSESSION_HPP__
#define COORDSESSION_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "netMultiRouteAgent.hpp"
#include "utilMap.hpp"
#include "rtnSessionProperty.hpp"

namespace engine
{

   /*
      _subSessionInfo define
   */
   typedef struct _subSessionInfo
   {
      MsgRouteID  routeID ;
      BOOLEAN     isConnected ;

      _subSessionInfo()
      {
         routeID.value  = MSG_INVALID_ROUTEID ;
         isConnected    = FALSE ;
      }
   } subSessionInfo ;

   typedef _utilMap<UINT64, subSessionInfo, 20 >   COORD_SUBSESSION_MAP ;

   /*
      _coordLastNodeStatus define
    */
   typedef struct _coordLastNodeStatus
   {
      _coordLastNodeStatus()
      {
         _nodeID.value = MSG_INVALID_ROUTEID ;
         _addTick = 0LL ;
      }

      // node ID of last selected node
      MsgRouteID  _nodeID ;
      // tick to add the last selected node
      UINT64      _addTick ;
   } coordLastNodeStatus ;

   typedef _utilMap< UINT32, coordLastNodeStatus, 20 > COORD_LASTNODE_MAP ;

   /*
      coordRequestInfo define
   */
   struct coordRequestInfo
   {
      MsgRouteID        _id ;
      NET_HANDLE        _handle ;

      coordRequestInfo()
      {
         _id.value = MSG_INVALID_ROUTEID ;
         _handle = NET_INVALID_HANDLE ;
      }
      coordRequestInfo( const MsgRouteID &id, NET_HANDLE handle )
      {
         _id.value = id.value ;
         _handle = handle ;
      }
   } ;
   typedef _utilMap<UINT64, coordRequestInfo, 20 >    COORD_REQINFO_MAP ;
   typedef COORD_REQINFO_MAP::iterator                COORD_REQINFO_MAP_IT ;

   /*
      CoordSession define
   */
   class CoordSession : public _rtnSessionProperty
   {
   public:
      CoordSession( pmdEDUCB *pEduCB );
      ~CoordSession(){}

   public:
      INT32    addSubSession( const MsgRouteID &routeID,
                              ISession *pSession );
      void     addSubSessionWithoutCheck( const MsgRouteID &routeID );
      BOOLEAN  delSubSession( const MsgRouteID &routeID );
      INT32    disConnect( const MsgRouteID &routeID );
      INT32    addLastNode( const MsgRouteID &routeID,
                            BOOLEAN primaryRequest ) ;
      UINT64   getLastNode( UINT32 groupID );
      void     removeLastNode( UINT32 groupID ) ;
      void     removeLastNode( const MsgRouteID &nodeID ) ;
      void     getAllSessionRoute( ROUTE_SET &routeMap );
      void     postEvent ( pmdEDUEvent const &data );
      BOOLEAN  isSubsessionConnected( const MsgRouteID &routeID );
      void     addRequest( const UINT64 reqID,
                           const MsgRouteID &routeID,
                           NET_HANDLE handle ) ;
      void     delRequest( const UINT64 reqID );
      //void     delRequest( const MsgRouteID &routeID );
      void     clearRequest();
      BOOLEAN  isValidResponse( const UINT64 reqID ) ;
      BOOLEAN  isValidResponse( const MsgRouteID &routeID,
                                const UINT64 reqID ) ;
      BOOLEAN  isValidResponse( const NET_HANDLE &handle,
                                const UINT64 reqID ) ;

   protected :
      virtual void _onSetInstance () ;

   private:
      CoordSession(){}
      CoordSession( CoordSession &coordSession ){}
      INT32 sessionInit( const MsgRouteID &routeID,
                         const CHAR *pRemoteIP,
                         UINT16 remotePort ) ;

   private:
      pmdEDUCB                   *_pEduCB;
      COORD_SUBSESSION_MAP       _subSessionMap;
      COORD_LASTNODE_MAP         _lastNodeMap;
      ossSpinXLatch              _mutex ;
      COORD_REQINFO_MAP          _requestMap;
   } ;
}

#endif // COORDSESSION_HPP__

