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

   Source File Name = tpServerSession.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_SERVER_SESSION_HPP__
#define TP_SERVER_SESSION_HPP__

#include "tpCBCommon.hpp"
#include "msg.hpp"

namespace engine
{

   /*
      _tpSubSession define
    */
   class _tpServerSession : public SDBObject
   {
   public:
      _tpServerSession( SDB_TPCB *tpCB ) ;
      virtual ~_tpServerSession() ;

   public:
      OSS_INLINE const MsgRouteID &getSourceRID() const
      {
         return _sourceRID ;
      }

      OSS_INLINE void setSourceRID( const MsgRouteID &routeID )
      {
         _sourceRID = routeID ;
      }

      OSS_INLINE void resetSourceRID()
      {
         _sourceRID.value = MSG_INVALID_ROUTEID ;
      }

      INT32 getPrimaryRID( MsgRouteID &routeID ) ;
      INT32 getServerRID( MsgRouteID &routeID ) ;

   protected :
      MsgRouteID           _sourceRID ;
      tpCatalogManager *   _catalogManager ;
   } ;

   typedef class _tpServerSession tpServerSession ;

}

#endif // TP_SERVER_SESSION_HPP__
