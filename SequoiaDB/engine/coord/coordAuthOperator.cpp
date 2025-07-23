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

   Source File Name = coordAuthOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordAuthOperator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAuthOperator implement
   */
   _coordAuthOperator::_coordAuthOperator()
   {
   }

   _coordAuthOperator::~_coordAuthOperator()
   {
   }

   const CHAR* _coordAuthOperator::getName() const
   {
      return "Auth" ;
   }

   BOOLEAN _coordAuthOperator::isReadOnly() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHOPR_EXEC, "_coordAuthOperator::execute" )
   INT32 _coordAuthOperator::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHOPR_EXEC ) ;

      rc = forward( pMsg, cb, TRUE, contextID, NULL, NULL, NULL, buf ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_AUTHOPR_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordAuthOperator::_onSucReply( const MsgOpReply *pReply )
   {
      if ( pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) )
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

