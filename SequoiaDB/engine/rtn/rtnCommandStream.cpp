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

   Source File Name = rtnCommandStream.cpp

   Descriptive Name = Runtime Stream Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft
   Last Changed =

*******************************************************************************/
#include "rtnCommandStream.hpp"
#include "rtnContext.hpp"
#include "rtnContextChangeStream.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   /*
      _rtnCMDWatch implement
    */
   IMPLEMENT_CMD_AUTO_REGISTER( _rtnCMDWatch )

   _rtnCMDWatch::_rtnCMDWatch()
   : _rtnCommand()
   {
   }

   _rtnCMDWatch::~_rtnCMDWatch()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNCMDWATCH_INIT, "_rtnCMDWatch::init" )
   INT32 _rtnCMDWatch::init( INT32 flags,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pMatcherBuff,
                             const CHAR *pSelectBuff,
                             const CHAR *pOrderByBuff,
                             const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCMDWATCH_INIT ) ;

      try
      {
         _boOptions = BSONObj( pMatcherBuff ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize command [%s], "
                 "occurred exception %s", name(), e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCMDWATCH_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNCMDWATCH_DOIT, "_rtnCMDWatch::doit" )
   INT32 _rtnCMDWatch::doit( _pmdEDUCB *cb,
                             SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB,
                             SDB_DPSCB *dpsCB,
                             INT16 w,
                             INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCMDWATCH_DOIT ) ;

      SDB_ASSERT( cb, "cb is invalid" ) ;
      SDB_ASSERT( rtnCB, "rtnCB is invalid" ) ;
      SDB_ASSERT( dmsCB, "dmsCB is invalid" ) ;
      SDB_ASSERT( pContextID, "context ID is invalid" ) ;

      rtnContextChangeStream::sharePtr contextPtr ;
      INT64 contextID = -1 ;
      
      rc = _checkPrivileges( cb );
      PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc ) ;

      PD_CHECK( SDB_DB_REBUILDING != PMD_DB_STATUS(),
                SDB_RTN_IN_REBUILD, error, PDERROR,
                "Failed to start watch, node is in rebuild" ) ;
      PD_CHECK( SDB_DB_FULLSYNC != PMD_DB_STATUS(),
                SDB_CLS_FULL_SYNC, error, PDERROR,
                "Failed to start watch, node is in full sync" ) ;
      PD_CHECK( SDB_DB_SHUTDOWN != PMD_DB_STATUS(),
                SDB_DATABASE_DOWN, error, PDERROR,
                "Failed to start watch, node is in shutdown" ) ;

      rc = rtnCB->contextNew( RTN_CONTEXT_CHANGE_STREAM, contextPtr, contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create change stream context, rc: %d", rc ) ;

      rc = contextPtr->open( _boOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open change stream context, rc: %d", rc ) ;

      if ( NULL != pContextID )
      {
         *pContextID = contextID ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCMDWATCH_DOIT, rc ) ;
      return rc ;

   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb );
         contextID = -1;
      }
      goto done ;
   }

   extern INT32 checkPrivilegesByWatchOptions( pmdEDUCB *cb, const BSONObj &options );

   // PD_TRACE_DECLARE_FUNCTION( SDB__RTNCMDWATCH_CHECKPRIVILEGES, "_rtnCMDWatch::_checkPrivileges" )
   INT32 _rtnCMDWatch::_checkPrivileges( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      if ( !cb->getSession()->privilegeCheckEnabled() )
      {
         goto done;
      }

      rc = checkPrivilegesByWatchOptions( cb, _boOptions );
      PD_RC_CHECK( rc, PDERROR, "Failed to check privileges, rc: %d", rc );
   done:
      return rc;
   error:
      goto done;
   }
}
