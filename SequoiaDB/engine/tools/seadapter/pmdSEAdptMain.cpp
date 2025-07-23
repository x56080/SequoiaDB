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

   Source File Name = pmdSEAdapterMain.cpp

   Descriptive Name = Search engine adapter main entry.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdProc.hpp"
#include "seAdptDef.hpp"
#include "seAdptMgr.hpp"
#include "omagentDef.hpp"
#include "utilProcessLock.hpp"
#include "pmdPipeManager.hpp"

namespace seadapter
{
   INT32 pmdResolveArguments( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      CHAR exePath[ OSS_MAX_PATHSIZE + 1 ] = {0} ;

      rc = ossGetEWD( exePath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Get module path failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = sdbGetSeAdptOptions()->init( argc, argv, exePath ) ;
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         goto done ;
      }
      else if ( rc )
      {
         ossPrintf( "Initialize configuration failed[ %d ]", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _pmdSystemInit( const CHAR *confPath )
   {
      INT32 rc                 = SDB_OK ;
      BOOLEAN bOk              = TRUE ;
      SDB_START_TYPE startType = SDB_START_NORMAL ;

      SDB_ASSERT( NULL != confPath, "config path is invalid" ) ;

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

   INT32 buildDialogPath( CHAR *dialogPath )
   {
      // Create the log file directory.
      INT32 rc = SDB_OK ;
      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      const CHAR *logPath = sdbGetSeAdptOptions()->getLogPath() ;

      if ( 0 != ossStrlen( logPath ) )
      {
         // use setting work path
         if ( !ossGetRealPath( logPath, dialogPath, OSS_MAX_PATHSIZE ) )
         {
            ossPrintf( "Invalid log path: %s" OSS_NEWLINE, logPath ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      // use default work path
      else
      {
         rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
         if ( rc )
         {
            ossPrintf( "Get working directory failed: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         ossChDir( currentPath ) ;

         // conf/log
         rc = utilBuildFullPath( currentPath, SDBCM_LOG_PATH,
                                 OSS_MAX_PATHSIZE, dialogPath ) ;
         if ( rc )
         {
            ossPrintf( "Build log path failed: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         // conf/log/seadapterlog
         rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE, SEADPT_LOG_DIR ) ;
         if ( rc )
         {
            ossPrintf( "Build log path failed: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }
         // conf/log/seadapterlog/svcname
         rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE,
                           sdbGetSeAdptOptions()->getSvcName() ) ;
         if ( rc )
         {
            ossPrintf( "Build log path failed: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }

      rc = ossMkdir( dialogPath ) ;
      if ( rc )
      {
         if ( SDB_FE != rc )
         {
            ossPrintf( "Make dialog path[ %s ] failed: %d" OSS_NEWLINE, dialogPath, rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void pmdOnQuit()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
      iPmdProc::stop( 0 ) ;
   }

   // Wait time for all daemon threads start, based on millisecond
   #define PMD_START_WAIT_TIME      ( 60000 )

   INT32 pmdThreadMainEntry( INT32 argc, CHAR** argv )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      UINT32 startTimerCount = 0 ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      po::variables_map vm ;

      pmdSetDBRole( SDB_ROLE_SEADAPTER ) ;

      rc = pmdResolveArguments( argc, argv ) ;
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         ossPrintf( "Failed resolving arguments(error=%d), exit" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      rc = buildDialogPath( dialogPath ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog path(error=%d), exit" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      // dialogPath/sdbseadapter.log
      rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE, SEADPT_LOG_FILE_NAME ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog path failed(error=%d), "
                    "exit" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      sdbEnablePD( dialogPath ) ;
      setPDLevel( (PDLEVEL)( sdbGetSeAdptOptions()->getDiagLevel() ) ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( ( getPDLevel() > PDEVENT ? PDEVENT: getPDLevel() ),
              "Start sdbseadapter[%s]...", verText ) ;

      // Print all configurations in log file.
      {
         string configs ;
         sdbGetSeAdptOptions()->toString( configs ) ;
         PD_LOG( PDEVENT, "All configs:\n%s", configs.c_str() ) ;
      }

      rc = pmdEnableSignalEvent( dialogPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Enable trap failed[ %d ]", rc ) ;

      PMD_REGISTER_CB( sdbGetSeAdapterCB() ) ;

      rc = sdbGetSystemPipeManager()->init( sdbGetSeAdptOptions()->getSvcName(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize pipe manager, rc: %d", rc ) ;

      // system init
      rc = _pmdSystemInit( sdbGetSeAdptOptions()->getConfPath() ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = krcb->init() ;
      PD_RC_CHECK( rc, PDERROR, "Initialize krcb failed[ %d ]", rc ) ;

      // wait until all daemon threads start
      while ( PMD_IS_DB_UP() && startTimerCount < PMD_START_WAIT_TIME &&
              !krcb->isBusinessOK() )
      {
         ossSleepmillis( 100 ) ;
         startTimerCount += 100 ;
      }

      if ( PMD_IS_DB_DOWN() )
      {
         rc = krcb->getShutdownCode() ;
         PD_LOG( PDERROR, "Start failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( startTimerCount >= PMD_START_WAIT_TIME )
      {
         PD_LOG( PDWARNING, "Start warning(timeout)" ) ;
      }

#if defined (_LINUX)
      {
         // Rename the process, adding the service name and role.
         CHAR pmdProcessName[OSS_RENAME_PROCESS_BUFFER_LEN + 1] = {0} ;
         ossSnprintf( pmdProcessName, OSS_RENAME_PROCESS_BUFFER_LEN,
                      "%s(%s) %s", SEADPT_PROCESS_NAME,
                      sdbGetSeAdptOptions()->getSvcName(),
                      SEADPT_ROLE_SHORT_STR ) ;
         ossEnableNameChanges( argc, argv ) ;
         ossRenameProcess( pmdProcessName ) ;
      }
#endif /* _LINUX */

      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs( 1 ) ;
      }
      rc = krcb->getShutdownCode() ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
      krcb->destroy() ;
      if ( krcb->needRestart() )
      {
         pmdGetStartup().restart( TRUE, rc ) ;
      }
      pmdGetStartup().final() ;
      pmdDisableSignalEvent() ;
      PD_LOG( PDEVENT, "Stop program, exit code: %d",
              krcb->getShutdownCode() ) ;
      return SDB_OK == rc ? 0 : 1 ;
   error:
      goto done ;
   }
}

INT32 main( INT32 argc, CHAR** argv )
{
   return seadapter::pmdThreadMainEntry( argc, argv ) ;
}

