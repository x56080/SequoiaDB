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

   Source File Name = utilPidFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2019  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilPidFile.hpp"
#include "ossFile.hpp"
#include "ossUtil.hpp"

namespace engine
{
   #define UTIL_PID_BUFFER_SIZE 10

   INT32 createPIDFile( const CHAR *pOutputPath )
   {
      INT32 rc = SDB_OK ;
      CHAR pid[UTIL_PID_BUFFER_SIZE + 1] = { 0 } ;
      ossFile file ;

      rc = file.open( pOutputPath, OSS_REPLACE | OSS_WRITEONLY | OSS_EXCLUSIVE,
                      OSS_DEFAULTFILE | OSS_RO ) ;
      if ( rc )
      {
         goto error ;
      }

      ossSnprintf( pid, UTIL_PID_BUFFER_SIZE + 1, "%d",
                   ossGetCurrentProcessID() ) ;

      rc = file.writeN( pid, ossStrlen( pid ) ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 checkAndCreatePIDFile( const CHAR *pOutputPath, BOOLEAN *pHasCreate )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needCreate = TRUE ;

      /// check file exist or not
      rc = ossAccess( pOutputPath ) ;
      if ( SDB_OK == rc )
      {
         ossFile file ;
         rc = file.open( pOutputPath, OSS_READONLY, OSS_DEFAULTFILE ) ;
         if ( SDB_OK == rc )
         {
            CHAR pidStr[UTIL_PID_BUFFER_SIZE + 1] = { 0 } ;
            INT64 readSize = 0 ;

            rc = file.readN( pidStr, UTIL_PID_BUFFER_SIZE, readSize ) ;
            if ( readSize > 0 )
            {
               OSSPID pidInFile = ossAtoi( pidStr ) ;

               /// check pid wetcher is the same
               if ( pidInFile == ossGetCurrentProcessID() )
               {
                  /// no need create pid file
                  needCreate = FALSE ;
               }
            }
         }
         file.close() ;
      }

      if ( needCreate )
      {
         rc = createPIDFile( pOutputPath ) ;
      }

      if ( pHasCreate && SDB_OK == rc && needCreate )
      {
         *pHasCreate = TRUE ;
      }

      return rc ;
   }

   INT32 removePIDFile( const CHAR *pFilePath )
   {
      return ossFile::deleteFileIfExists( pFilePath ) ;
   }
}