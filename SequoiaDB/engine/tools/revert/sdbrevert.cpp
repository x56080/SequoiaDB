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

   Source File Name = sdbrevert.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "revertCommon.hpp"
#include "revertOptions.hpp"
#include "logProcessor.hpp"
#include "lobMetaMgr.hpp"
#include "ossAtomic.hpp"
#include "ossSignal.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossVer.h"
#include "utilCommon.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <pthread.h>

using namespace std ;
using namespace engine ;
using namespace sdbrevert ;

void* handleLogFunc( void *args )
{
   ((logProcessor*)args)->run() ;
   return NULL;
}

ossAtomic32 interrupted( 0 ) ;

#if defined( _LINUX )
void signalHandler( int signal ) {
    if ( SIGINT == signal )
    {
        interrupted.init( 1 ) ;
    }
}
#endif // _LINUX

int main( int argc, char* argv[] )
{
   INT32 rc = SDB_OK ;
   revertOptions options ;
   logFileMgr logfileMgr ;
   lobMetaMgr lobMetaMgr ;
   lobLockMap lobLockMap( SDB_REVERT_LOB_LOCK_MAX_NUM ) ;
   globalInfoMgr globalInfoMgr ;
   vector<pthread_t> threadList ;
   vector<logProcessor*> threadArgList ;
   logProcessor* processor = NULL ;
   CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

#if defined( _LINUX )
   signal( SIGINT, signalHandler ) ;
#endif // _LINUX

   // 1. parse parameters
   rc = options.parse( argc, argv ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( options.hasHelp() )
   {
      options.printHelpInfo() ;
      goto done ;
   }

   if ( options.hasVersion() )
   {
      ossPrintVersion( "sdbrevert" ) ;
      goto done ;
   }

   // 2. enable log
   sdbEnablePD( SDB_REVERT_LOG ) ;
   setPDLevel( PDINFO ) ;

   ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
   PD_LOG( PDEVENT, "Start sdbrevert[%s]...", verText) ;

   // 3. get all log file
   rc = listAllLogFile( options.logPathList(), logfileMgr ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to get all log file, rc= %d", rc ) ;
      cerr << "Failed to get all log file, rc= " << rc << endl ;
      goto error ;
   }

   if ( logfileMgr.empty() )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Archivelog file or replicalog file not found" ) ;
      cerr << "Archivelog file or replicalog file not found" << endl ;
      goto error ;
   }

   for ( INT32 i = 0 ; i < options.jobs() ; ++i )
   {
      pthread_t thread ;

      processor = new logProcessor() ;
      if ( NULL == processor )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new sdbrevert::logProcessor, rc= %d", rc ) ;
         goto error ;
      }

      rc = processor->init( options, logfileMgr, lobMetaMgr, lobLockMap, interrupted, globalInfoMgr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Fialed to init logProcessor, rc= %d", rc ) ;
         cerr << "Fialed to init logProcessor, rc= " << rc << endl ;
         goto error ;
      }

      pthread_create( &thread, NULL, handleLogFunc, processor ) ;
      threadList.push_back( thread ) ;
      threadArgList.push_back( processor ) ;
      processor = NULL ;
   }

   for ( size_t i = 0 ; i < threadList.size() ; ++i )
   {
      pthread_join( threadList[i], NULL ) ;
   }

   if ( interrupted.fetch() > 0 )
   {
      rc = SDB_APP_INTERRUPT ;

      // wait for the child threads to finish executing
      while ( globalInfoMgr.getRunNum() > 0 )
      {
         ossSleepmillis( 100 ) ;
      }
   }
   else
   {
      rc = globalInfoMgr.getGlobalRc() ;
   }

   // print result infomation
   globalInfoMgr.printResultInfo() ;

   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "sdbrevert failed to execute, rc= %d" , rc ) ;
      cerr << "sdbrevert failed to execute, rc= " << rc 
           << ", see " << SDB_REVERT_LOG << " for more details" << endl ;
      goto error ;
   }

done:
   for ( size_t i = 0 ; i < threadArgList.size() ; ++i )
   {
      SAFE_OSS_DELETE( threadArgList[i] ) ;
   }

   PD_LOG ( PDEVENT, "Stop programme, exit code: %d", rc ) ;

   return engine::utilRC2ShellRC (rc ) ;
error:
   goto done ;
}