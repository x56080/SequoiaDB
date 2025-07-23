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

   Source File Name = ossEnv.cpp

   Descriptive Name = oss Environment Info

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/12/2022  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossEnv.hpp"
#include "utilStr.hpp"

namespace engine
{
   #define OSS_PATH_DETECT_SYS_PAGESIZE  (4096)    // 4K
   #define OSS_PATH_DETECT_FILE_NAME     ".SEQUOIADB_PATH_DETECT_FILE"

   INT32 _ossPathDetector::testPunchHole( const ossPoolSet<ossPoolString> &pathSet )
   {
      INT32 rc    = SDB_OK ;

      ossPoolSet<ossPoolString>::iterator it ;
      for ( it = pathSet.begin() ; it != pathSet.end() ; ++it )
      {
         rc = _tryToPunchHole( it->c_str() ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _ossPathDetector::_tryToPunchHole( const CHAR* pFilePath )
   {
      SDB_ASSERT( pFilePath, "pFilePath can't be null" ) ;
      INT32 rc = SDB_OK ;
      OSSFILE file ;

      CHAR tmpFilePath[ OSS_MAX_PATHSIZE +  1 ] = { 0 } ;

      // eg: /opt/sequoiadb/databases/20000/ --->
      //     /opt/sequoiadb/databases/20000/.SEQUOIADB_PATH_DETECT_FILE
      rc = utilBuildFullPath( pFilePath, OSS_PATH_DETECT_FILE_NAME,
                              OSS_MAX_PATHSIZE, tmpFilePath ) ;

      rc = ossOpen( tmpFilePath, OSS_REPLACE | OSS_WRITEONLY, OSS_WU | OSS_RU,
                    file ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", tmpFilePath, rc ) ;
         goto error ;
      }

      rc = ossFallocate( &file, OSS_FALLOC_FL_ALLOC_SPACE, 0, OSS_PATH_DETECT_SYS_PAGESIZE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to allocate space for file[%s],"
                 " rc: %d", tmpFilePath, rc ) ;
         goto error ;
      }

      rc = ossFallocate( &file, OSS_FALLOC_FL_PUNCH_HOLE | OSS_FALLOC_FL_KEEP_SIZE,
                         0, OSS_PATH_DETECT_SYS_PAGESIZE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to punch hole at file[%s],"
                 " rc: %d", tmpFilePath, rc ) ;
         goto error ;
      }

   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
         ossDelete( tmpFilePath ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   _ossEnvInfo::_ossEnvInfo()
   {
      _supportPunchHoleMode = FALSE ;
   }

   _ossEnvInfo::~_ossEnvInfo()
   {
   }

   INT32 _ossEnvInfo::init( const CHAR *dataPath, const CHAR *idxPath,
                            const CHAR *lobmPath, const CHAR *lobdPath )
   {
      INT32 rc    = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      ossPathDetector detector ;

      try
      {
         _filePathsSet.insert( dataPath ) ;
         _filePathsSet.insert( idxPath ) ;
         _filePathsSet.insert( lobmPath ) ;
         _filePathsSet.insert( lobdPath ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when inserting file path:"
                 " %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      rcTmp = detector.testPunchHole( _filePathsSet ) ;
      if ( rcTmp )
      {
         PD_LOG( PDWARNING, "OSS environment do not support punch hole at file,"
                 " rc: %d", rc ) ;
         _supportPunchHoleMode = FALSE ;
      }
      else
      {
         _supportPunchHoleMode = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   ossEnvInfo* getOssEnvInfo()
   {
      static ossEnvInfo ossEnv ;
      return &ossEnv ;
   }

   BOOLEAN ossEnvCanPunchHole()
   {
      return getOssEnvInfo()->_supportPunchHoleMode ;
   }
}

