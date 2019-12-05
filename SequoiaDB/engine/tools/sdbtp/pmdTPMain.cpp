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

   Source File Name = pmdTPMain.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for SequoiaDB
   Time Protocol Service.

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
#include "tpCB.hpp"

using namespace std ;

namespace engine
{

   void pmdOnQuit ()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
      iPmdProc::stop( 0 ) ;
   }

   INT32 pmdThreadMainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_TPCB *tpCB = sdbGetTPCB() ;
      tpOptions *options = tpCB->getOptions() ;

      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR pidFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      INT32 delSig[] = { 17, 0 } ; // del SIGCHLD
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      pmdSetDBRole( SDB_ROLE_TP ) ;

      // 1. get root path
      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         cout << "Get current path failed: " << rc << endl ;
         goto error ;
      }

      // 3. create pid file
      rc = utilBuildFullPath( currentPath, SDBTP_LOG_PATH, OSS_MAX_PATHSIZE,
                              dialogPath ) ;
      if ( SDB_OK != rc )
      {
         cout << "Build dialog path failed: " << rc << endl ;
         goto error ;
      }

      // make sure the dir exist
      rc = ossMkdir( dialogPath ) ;
      if ( SDB_OK != rc && SDB_FE != rc )
      {
         cout << "Create dialog dir: " << dialogPath << " failed: "
                   << rc << endl ;
         goto error ;
      }

      rc = utilBuildFullPath( dialogPath, SDBTP_PID_FILE_NAME,
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
      rc = utilBuildFullPath( dialogPath, SDBTP_DIALOG_FILE_NAME,
                              OSS_MAX_PATHSIZE, dialogFile ) ;
      if ( SDB_OK != rc )
      {
         cout << "Build dialog path failed: " << rc << endl ;
         goto error ;
      }
      sdbEnablePD( dialogFile ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start sdbtp[%s]...", verText) ;

      // 4. init param
      options = tpCB->getOptions() ;
      rc = options->initialize( argc, argv, currentPath ) ;
      if ( SDB_OK != rc )
      {
         goto done ;
      }

      setPDLevel( (PDLEVEL)( options->getDiagLevel() ) ) ;
      options->logOptions() ;

      // 6. handlers and init global mem
      rc = pmdEnableSignalEvent( dialogPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 delSig ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to enable trap, rc: %d", rc ) ;

#if defined( _LINUX )
      signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

      // 7. register agent cb
      PMD_REGISTER_CB( tpCB ) ;

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
