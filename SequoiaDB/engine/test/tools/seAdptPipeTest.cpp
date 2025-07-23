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

   Source File Name = seAdptPipeTest.cpp

   Descriptive Name = Test seadapter pipe

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for pipe
   manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          25/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilNodeOpr.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "ossPath.hpp"
#include "pmdDef.hpp"
#include "gtest/gtest.h"
#include "../bson/util/builder.h"
#include <iostream>
#if defined( _LINUX )
#include <dirent.h>
#endif //_LINUX

using namespace bson ;

#define TEST_SE_PROC_NAME_PREFIX            "sdbseadapter("

BOOLEAN isDigitalStr( const CHAR *pStr )
{
   if ( NULL == pStr ) return FALSE ;
   while ( *pStr )
   {
      CHAR c = *pStr++ ;
      if ( c < '0' || c > '9' ) return FALSE ;
   }
   return TRUE ;
}

#if defined( _LINUX )
TEST( seAdapter, pipe_read_and_write )
{
   INT32 rc = SDB_OK ;
   DIR *pDir                  = NULL ;
   struct dirent *pDirent     = NULL ;
   CHAR *pSvcBegin            = NULL ;
   CHAR *pSvcEnd              = NULL ;
   CHAR *seAdptSvcName        = NULL ;
   BOOLEAN isOpen = FALSE ;
   OSSPID seAdptPID = OSS_INVALID_PID ;
   BOOLEAN hasSeAdptProcess = FALSE ;

   pDir = opendir( PROC_PATH ) ;
   ASSERT_TRUE( pDir != NULL ) ;
   isOpen = TRUE ;

   while( (pDirent = readdir( pDir )) != NULL )
   {
      UINT64 seAdptStartTime = 0 ;
      CHAR seAdptModeStr[ 16 + 1 ] = { 0 } ;
      CHAR seAdptDataSvcName [ 32 + 1 ] = { 0 } ;
      CHAR tmpTime[ 21 + 1 ] = { 0 } ;
      struct tm otm ;
      time_t tt ;

      if ( !isDigitalStr( pDirent->d_name ) )
      {
         continue ;
      }
      CHAR pathName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      ossSnprintf( pathName, OSS_MAX_PATHSIZE, PROC_CMDLINE_PATH_FORMAT,
                   pDirent->d_name ) ;
      FILE *fp = NULL ;
      fp = fopen( pathName, "r" ) ;
      if ( !fp )
      {
         continue ;
      }
      CHAR commandLine[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR *pTmp = fgets ( commandLine, OSS_MAX_PATHSIZE, fp ) ;
      fclose(fp) ;
      if ( NULL == pTmp )
      {
         continue ;
      }

      // analysize node info
      pSvcBegin = NULL ;
      pSvcEnd = NULL ;

      // 1. svcname
      pSvcBegin = ossStrstr( commandLine, TEST_SE_PROC_NAME_PREFIX ) ;
      if ( !pSvcBegin )
      {
         continue ;
      }

      printf( "command line: %s\n", commandLine ) ;

      pSvcEnd = ossStrchr( pSvcBegin + 1, ')' ) ;
      if ( !pSvcEnd || pSvcEnd - pSvcBegin <= 1 ||
           ossStrlen( pSvcEnd ) > 3  )
      {
         continue ;
      }
      *pSvcEnd = 0 ;
      seAdptSvcName = pSvcBegin + ossStrlen( TEST_SE_PROC_NAME_PREFIX ) ;

      // 2. PID
      seAdptPID = ossAtoi( pDirent->d_name ) ;

      // 3. test read and write
      // 3.1 get seadapter's startTime
      rc = engine::utilWriteReadPipe( seAdptSvcName, seAdptPID,
                                      ENGINE_NPIPE_MSG_STARTTIME,
                                      sizeof( ENGINE_NPIPE_MSG_STARTTIME ),
                                      (CHAR*)&seAdptStartTime,
                                      sizeof( UINT64 ), TRUE ) ;
      if ( rc )
      {
         printf( "Failed to get startTime from pipe, rc: %d\n", rc ) ;
         ASSERT_TRUE( SDB_OK == rc ) ;
      }

      if ( 0 == seAdptStartTime )
      {
         printf( "Invalid start time: 0\n" ) ;
         ASSERT_TRUE( FALSE ) ;
      }

      ASSERT_TRUE( seAdptStartTime != 0 ) ;

      tt = seAdptStartTime ;
      localtime_r( &tt, &otm ) ;
      ossSnprintf( tmpTime, sizeof( tmpTime ) - 1,
                   "%04d-%02d-%02d-%02d.%02d.%02d",
                   otm.tm_year+1900,
                   otm.tm_mon+1,
                   otm.tm_mday,
                   otm.tm_hour,
                   otm.tm_min,
                   otm.tm_sec ) ;

      // 3.2 get seadapter's mode
      rc = engine::utilWriteReadPipe( seAdptSvcName, seAdptPID,
                                      ENGINE_NPIPE_MSG_MODE,
                                      sizeof( ENGINE_NPIPE_MSG_MODE ),
                                      (CHAR *)seAdptModeStr,
                                      16,
                                      FALSE ) ;
      if ( rc )
      {
         printf( "Failed to get mode str from pipe, rc: %d\n", rc ) ;
         ASSERT_TRUE( SDB_OK == rc ) ;
      }

      if ( 0 != ossStrcmp( seAdptModeStr, "unregister" ) &&
           0 != ossStrcmp( seAdptModeStr, "read-only") &&
           0 != ossStrcmp( seAdptModeStr, "read-write") )
      {
         printf( "Invalid mode str: %s\n", seAdptModeStr ) ;
         ASSERT_TRUE( FALSE ) ;
      }

      // 3.3 get seadapter's data svcname seAdptDataSvcName
      rc = engine::utilWriteReadPipe( seAdptSvcName, seAdptPID,
                                      ENGINE_NPIPE_MSG_DATASVCNAME,
                                      sizeof( ENGINE_NPIPE_MSG_DATASVCNAME ),
                                      (CHAR *)seAdptDataSvcName,
                                      32,
                                      FALSE ) ;
      if ( rc )
      {
         printf( "Failed to get data svcname from pipe, rc: %d\n", rc ) ;
         ASSERT_TRUE( SDB_OK == rc ) ;
      }

      if ( 0 == ossStrcmp( seAdptDataSvcName, "" ) )
      {
         printf( "Invalid data svcname: \"0\"\n" ) ;
         ASSERT_TRUE( FALSE ) ;
      }

      printf( "seadapter svcname: %s, PID: %d, startTime: %s, mode: %s, dataSvcName: %s\n",
              seAdptSvcName, seAdptPID, tmpTime, seAdptModeStr, seAdptDataSvcName ) ;

      hasSeAdptProcess = TRUE ;
   }

   if ( !hasSeAdptProcess )
   {
      printf( "The system has no seadapter process\n" ) ;
      ASSERT_TRUE( hasSeAdptProcess ) ;
   }

   if ( isOpen )
   {
      closedir( pDir ) ;
   }
}
#endif