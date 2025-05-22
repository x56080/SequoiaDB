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