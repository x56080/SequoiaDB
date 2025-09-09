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

   Source File Name = clsLocalValidation.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsLocalValidation.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

#define CLS_DISK_DETECT_FILE_NAME     ".SEQUOIADB_DISK_DETECT_FILE"
#define CLS_DISK_DETECT_FILE_CONTENT  'N'
#define CLS_DISK_DETECT_FILE_CONTENT_SIZE OSS_FILE_DIRECT_IO_ALIGNMENT

namespace engine
{
   clsDiskWriteCostTime curDiskWriteCostTime ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLS_TRACE_EMPTY_FUNC, "clsTarceEmptyFunction" )
   static INT32 clsTarceEmptyFunction()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CLS_TRACE_EMPTY_FUNC ) ;
      PD_TRACE_EXITRC( SDB_CLS_TRACE_EMPTY_FUNC, rc ) ;
      return rc ;
   }

   _clsDiskDetector::_clsDiskDetector()
   {
      _isMonitoredRole = TRUE ;
      _lastTick = 0 ;
      _hasInit = FALSE ;
      _content = NULL ;
   }

   _clsDiskDetector::~_clsDiskDetector()
   {
      if ( NULL != _content )
      {
         SDB_OSS_ORIGINAL_FREE( _content ) ;
         _content = NULL ;
      }
   }

   INT32 _clsDiskDetector::detect()
   {
      INT32  rc = SDB_OK ;
      BOOLEAN needUpdateLastTick = FALSE ;
      BOOLEAN first = TRUE ;
      clsDiskWriteCostTime tmpTime ;

      if ( !_hasInit )
      {
         rc = init() ;
         if ( rc )
         {
            goto error ;
         }
         _hasInit = TRUE ;
      }

      if ( !_isNeedToDetect() )
      {
         goto done ;
      }

      needUpdateLastTick = TRUE ;

      curDiskWriteCostTime.lastSpentTime = curDiskWriteCostTime.curSpentTime ;
      curDiskWriteCostTime.finishAllDiskWrite = FALSE ;

      for( ossPoolSet<ossPoolString>::iterator it = _filePathsSet.begin() ;
           it != _filePathsSet.end() ; ++it )
      {
         rc = _tryToWriteFile( it->c_str(), curDiskWriteCostTime ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( first )
         {
            // write the first file
            tmpTime = curDiskWriteCostTime ;
            first = FALSE ;
         }
         else
         {
            // get the max spentTime
            if ( tmpTime.curSpentTime > curDiskWriteCostTime.curSpentTime )
            {
               curDiskWriteCostTime = tmpTime ;
            }
            else
            {
               tmpTime = curDiskWriteCostTime ;
            }
         }
      }

   done:
      if ( needUpdateLastTick )
      {
         _lastTick = pmdGetDBTick() ;
      }
      curDiskWriteCostTime.finishAllDiskWrite = TRUE ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsDiskDetector::_isNeedToDetect()
   {
      if ( _isMonitoredRole &&
           pmdGetTickSpanTime( _lastTick ) >= OSS_ONE_SEC * pmdGetOptionCB()->detectDisk() &&
           OSS_BIT_TEST( pmdGetOptionCB()->ftMask(), PMD_FT_MASK_DISK_FAULT ) )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _clsDiskDetector::init()
   {
      INT32 rc = SDB_OK ;
      SDB_ROLE role = utilGetRoleEnum( pmdGetOptionCB()->dbroleStr() ) ;

      if ( SDB_ROLE_COORD == role || SDB_ROLE_MAX == role )
      {
         _isMonitoredRole = FALSE ;
         _hasInit = TRUE ;
         goto done ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getDbPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init db path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getIndexPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init index path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getLobPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init lob path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getLobMetaPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init lobm path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getReplLogPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init replicalog path, rc: %d", rc ) ;
         goto error ;
      }

      _content = ( CHAR * )ossAlignedAlloc( OSS_FILE_DIRECT_IO_ALIGNMENT,
                                            CLS_DISK_DETECT_FILE_CONTENT_SIZE ) ;
      if ( NULL != _content )
      {
         ossMemset( _content, CLS_DISK_DETECT_FILE_CONTENT, CLS_DISK_DETECT_FILE_CONTENT_SIZE ) ;
      }
      else
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc file content, rc: %d", rc ) ;
         goto error ;
      }

      _lastTick = pmdGetDBTick() ;

      _hasInit = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDiskDetector::_addFilePath( const CHAR* pFilePath )
   {
      SDB_ASSERT( pFilePath, "pFilePath can't be null" ) ;

      INT32 rc = SDB_OK ;
      CHAR tmpFilePath[ OSS_MAX_PATHSIZE +  1 ] = { 0 } ;

      // eg: /opt/sequoiadb/databases/20000/ --->
      //     /opt/sequoiadb/databases/20000/.SEQUOIADB_DISK_DETECT_FILE
      rc = utilBuildFullPath( pFilePath, CLS_DISK_DETECT_FILE_NAME,
                              OSS_MAX_PATHSIZE, tmpFilePath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build disk detect file path, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         _filePathsSet.insert( tmpFilePath ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when inserting file path:"
                 " %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDiskDetector::_tryToWriteFile( const CHAR* pFilePath, clsDiskWriteCostTime &time )
   {
      SDB_ASSERT( pFilePath, "pFilePath can't be null" ) ;

      UINT32 rc = SDB_OK ;
      OSSFILE file ;

      time.beginTime = pmdGetDBTick() ;
      time.endTime = 0 ;
      time.curSpentTime = 0 ;
      time.returnCode = rc ;

      rc = ossOpen( pFilePath, OSS_REPLACE | OSS_WRITEONLY | OSS_DIRECTIO,
                    OSS_WU | OSS_RU, file ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", pFilePath, rc ) ;
         goto error ;
      }

      clsTarceEmptyFunction() ;

      rc = ossWriteN( &file, _content, CLS_DISK_DETECT_FILE_CONTENT_SIZE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write file[%s], rc: %d", pFilePath, rc ) ;
         goto error ;
      }

   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
         ossDelete( pFilePath ) ;
      }
      time.returnCode = rc ;
      time.endTime = pmdGetDBTick() ;
      time.curSpentTime = pmdGetTickSpanTime( time.beginTime ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsLocalValidation::run()
   {
      INT32 rc = SDB_OK ;

      pmdUpdateValidationTick() ;

      rc = _diskDetector.detect() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Unexpected error, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

