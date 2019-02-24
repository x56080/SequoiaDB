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

   Source File Name = rtnCoordAuth.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCoordAuth.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOAUTH_EXECUTE, "rtnCoordAuth::execute" )
   INT32 rtnCoordAuth::execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOAUTH_EXECUTE ) ;
      rc = forward( pMsg, cb, MSG_AUTH_VERIFY_RES,
                    TRUE, contextID ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOAUTH_EXECUTE, rc ) ;
      return rc ;
   }

   void rtnCoordAuth::_onSucReply( const MsgOpReply *pReply )
   {
      if ( pReply->header.messageLength > sizeof( MsgOpReply ) )
      {
         try
         {
            BSONObj obj( ( const CHAR* )pReply + sizeof( MsgOpReply ) ) ;
            BSONElement e = obj.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object == e.type() )
            {
               updateSessionByOptions( e.embeddedObject() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
            /// ignore
         }
      }
   }

}

