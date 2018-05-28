/*******************************************************************************

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
   typedef _utilMap<UINT32, MsgRouteID, 20 >       COORD_LASTNODE_MAP ;

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
      void     addLastNode( const MsgRouteID &routeID );
      MsgRouteID getLastNode( UINT32 groupID );
      void     removeLastNode( UINT32 groupID ) ;
      void     removeLastNode( UINT32 groupID, const MsgRouteID &nodeID ) ;
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

