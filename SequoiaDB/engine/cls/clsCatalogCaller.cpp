/*******************************************************************************

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

   Source File Name = clsCatalogCaller.cpp

   Descriptive Name = clsCatalogCaller.hpp

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsCatalogCaller.hpp"
#include "netRouteAgent.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   const INT32 CLS_CALLER_NO_SEND = -1 ;
   const INT32 CLS_CALLER_INTERVAL = 5000 ;

   _clsCatalogCaller::_clsCatalogCaller()
   {
   }

   _clsCatalogCaller::~_clsCatalogCaller()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_CALL, "_clsCatalogCaller::call" )
   INT32 _clsCatalogCaller::call( MsgHeader *header, UINT32 times )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_CALL );
      _clsCataCallerMeta &meta = _meta[MAKE_REPLY_TYPE(header->opCode)] ;
      if ( (SINT32)meta.bufLen < header->messageLength )
      {
         if ( NULL != meta.header )
         {
            SDB_OSS_FREE( meta.header ) ;
            meta.bufLen = 0 ;
         }
         // memory is free in destructor
         meta.header = ( MsgHeader *)SDB_OSS_MALLOC( header->messageLength ) ;
         if ( NULL == meta.header )
         {
            PD_LOG ( PDERROR, "Failed to allocate memory for header" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         meta.bufLen = header->messageLength ;
      }

      ossMemcpy( meta.header, header, header->messageLength ) ;
      PD_LOG( PDEVENT, "send msg[%d] to catalog node.",
              meta.header->opCode ) ;

      /// reset info
      meta.timeout = 0 ;
      meta.sendTimes = times ;

      pmdGetKRCB()->getClsCB()->sendToCatlog( meta.header ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSCATCLR_CALL, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_REMOVE, "_clsCatalogCaller::remove" )
   void _clsCatalogCaller::remove( const MsgHeader *header, INT32 result )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_REMOVE );
      callerMeta::iterator itr = _meta.find( header->opCode ) ;
      if ( _meta.end() != itr && SDB_OK == result )
      {
         PD_LOG( PDEVENT, "response is ok, remove msg[(%d)%d]",
                 IS_REPLY_TYPE( header->opCode ),
                 GET_REQUEST_TYPE( header->opCode ) ) ;

         if ( itr->second.sendTimes > 1 )
         {
            --( itr->second.sendTimes ) ;
         }
         else
         {
            itr->second.timeout = CLS_CALLER_NO_SEND ;
            itr->second.sendTimes = 0 ;
         }
      }

      PD_TRACE_EXIT ( SDB__CLSCATCLR_REMOVE );
      return ;
   }

   void _clsCatalogCaller::remove( INT32 opCode )
   {
      callerMeta::iterator itr = _meta.find( (UINT32)opCode ) ;
      if ( _meta.end() != itr )
      {
         if ( itr->second.sendTimes > 1 )
         {
            --( itr->second.sendTimes ) ;
         }
         else
         {
            itr->second.timeout = CLS_CALLER_NO_SEND ;
            itr->second.sendTimes = 0 ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_HNDTMOUT, "_clsCatalogCaller::handleTimeout" )
   void _clsCatalogCaller::handleTimeout( const UINT32 &millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_HNDTMOUT );
      callerMeta::iterator itr = _meta.begin() ;
      for ( ; itr != _meta.end(); itr++ )
      {
         if ( CLS_CALLER_NO_SEND != itr->second.timeout )
         {
            itr->second.timeout += millisec ;
            if ( CLS_CALLER_INTERVAL <= itr->second.timeout )
            {
               /// don't need to update catalog info
               pmdGetKRCB()->getClsCB()->sendToCatlog( itr->second.header) ;
               itr->second.timeout = 0 ;
            }
         }
      }
      PD_TRACE_EXIT ( SDB__CLSCATCLR_HNDTMOUT );
      return ;
   }

}
