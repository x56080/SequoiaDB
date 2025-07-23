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

   Source File Name = stp.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "utilStr.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossVer.h"
#include "pmd.hpp"
#include "pmdProc.hpp"
#include "utilPidFile.hpp"
#include "stpCB.hpp"

using namespace std ;

namespace engine
{

   void pmdOnQuit ()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
   }

   static INT32 _pmdSystemInit( const CHAR *confPath )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != confPath, "config path is invalid" ) ;

      SDB_START_TYPE startType = SDB_START_NORMAL ;
      BOOLEAN bOk = TRUE ;

      // analysis the start type
      rc = pmdGetStartup().init( confPath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check start up file [%s], "
                   "rc: %d", confPath, rc ) ;

      startType = pmdGetStartup().getStartType() ;
      bOk = pmdGetStartup().isOK() ;

      PD_LOG( PDEVENT, "Start up from %s, data is %s",
              pmdGetStartTypeStr( startType ),
              bOk ? "normal" : "abnormal" ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 pmdThreadMainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      STPCB *stpCB = stpGetSTPCB() ;
      stpOptions *options = stpCB->getOptions() ;

      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR confPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR pidFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      INT32 delSig[] = { 17, 0 } ; // del SIGCHLD
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      BOOLEAN daemonMode = FALSE ;

      pmdSetDBRole( SDB_ROLE_STP ) ;

      // 1. get root path
      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         cout << "Get current path failed: " << rc << endl ;
         goto error ;
      }

      // 3. create pid file
      rc = utilBuildFullPath( currentPath, STP_LOG_PATH, OSS_MAX_PATHSIZE,
                              confPath ) ;
      if ( SDB_OK != rc )
      {
         cout << "Build dialog path failed: " << rc << endl ;
         goto error ;
      }

      // make sure the dir exist
      rc = ossMkdir( confPath ) ;
      if ( SDB_OK != rc && SDB_FE != rc )
      {
         cout << "Create dialog dir: " << confPath << " failed: "
                   << rc << endl ;
         goto error ;
      }

      rc = utilBuildFullPath( confPath, STP_PID_FILE_NAME,
                              OSS_MAX_PATHSIZE, pidFile ) ;
      if ( SDB_OK != rc )
      {
         cout << "Build dialog path failed: " << rc << endl ;
         goto error ;
      }

      rc = createPIDFile( pidFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to create pid file, rc: %d", rc ) ;
         rc = SDB_OK ;
      }

      // 2. enable dialog
      rc = utilBuildFullPath( confPath, STP_DIAGLOG_FILE_NAME,
                              OSS_MAX_PATHSIZE, dialogFile ) ;
      if ( SDB_OK != rc )
      {
         cout << "Build dialog path failed: " << rc << endl ;
         goto error ;
      }
      sdbEnablePD( dialogFile ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start stp[%s]...", verText) ;

      // 4. init param
      options = stpCB->getOptions() ;
      rc = options->initialize( argc, argv, currentPath, daemonMode ) ;
      if ( SDB_OK != rc )
      {
         goto done ;
      }
      if ( daemonMode )
      {
         goto done ;
      }

      setPDLevel( (PDLEVEL)( options->getDiagLevel() ) ) ;
      options->logOptions() ;

      // 6. handlers and init global mem
      rc = pmdEnableSignalEvent( confPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 delSig ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to enable trap, rc: %d", rc ) ;

#if defined( _LINUX )
      signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

      // 7. register agent cb
      PMD_REGISTER_CB( stpCB ) ;

      // system init
      rc = _pmdSystemInit( confPath ) ;
      if ( rc )
      {
         goto error ;
      }

      // 8. initialize pipe manager
      rc = sdbGetSystemPipeManager()->init( options->getServiceName(), TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, rc: %d",
                   rc ) ;

      // 9. init krcb
      rc = krcb->init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init krcb, rc: %d", rc ) ;

      // 10. change process name
      pmdRenameProcess( argc, argv, options->getServiceName() ) ;

      // Now master thread get into big loop and check shutdown flag
      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs ( 1 ) ;
         krcb->onTimer( OSS_ONE_SEC ) ;
      }

      rc = krcb->getShutdownCode() ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
      removePIDFile( pidFile ) ;
      krcb->destroy() ;
      if ( krcb->needRestart() )
      {
         pmdGetStartup().restart( TRUE, rc ) ;
      }
      pmdGetStartup().final() ;
      pmdDisableSignalEvent() ;
      PD_LOG( PDEVENT, "Stop program, exit code: %d",
              krcb->getShutdownCode() ) ;
      return rc == SDB_OK ? 0 : 1 ;

   error:
      goto done ;
   }

}

INT32 main( INT32 argc, CHAR **argv )
{
   return engine::pmdThreadMainEntry( argc, argv ) ;
}
