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

   Source File Name = utilCipherFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCipherFile.hpp"

namespace engine
{

   _utilCipherFile::~_utilCipherFile()
   {
      if ( NULL != _fileContent )
      {
         SDB_OSS_FREE( _fileContent ) ;
      }
   }

   INT32 _utilCipherFile::initFile( const string &fileName, 
                                    cipherRole role )
   {
      INT32 rc = SDB_OK ;

      string usr, cipherText ;
      UINT32 iMode = ( ( RRole == role ) ? OSS_READONLY : ( OSS_CREATE | OSS_READWRITE ) ) ;

      rc = _file.open( fileName, iMode, OSS_RU | OSS_WU ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "cipher file option [open] err [%d] ", ossGetLastError()) ;
         goto error;
      }

      return rc ;

   error:
      goto error ;
   }

   INT32 _utilCipherFile::readFromFile( const CHAR **fileContent,
                                        INT64 *contentLen )
   {
      INT32 rc = SDB_OK ;

      INT64 fileSize = 0 ;

      rc = _file.getFileSize( fileSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "cipher file option [getFileSize] err [%d] ", ossGetLastError()) ;
         goto error;
      }

      if ( 0 == fileSize )
      {
         PD_LOG ( PDDEBUG, "cipher file is empty ") ;
         goto done;
      }

      _fileContent = ( CHAR* )SDB_OSS_MALLOC( fileSize * sizeof( CHAR ) ) ;
      if ( NULL == _fileContent )
      {
         PD_LOG ( PDERROR, "cipher file option [SDB_OSS_MALLOC] err [%d] ", 
                  ossGetLastError() ) ;
         rc = SDB_OOM ; 
         goto error ;
      }

      rc = _file.read( _fileContent, fileSize, *contentLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "cipher file option [read] err [%d] ", ossGetLastError() ) ;
         SDB_OSS_FREE( _fileContent ) ;
         goto error ;
      }

   *fileContent = _fileContent ;

   done:
      return rc ;
   error:
      goto error ;
   }

   INT32 _utilCipherFile::writeToFile( const std::string& fileContent )
   {
      INT32  rc = SDB_OK ;

      INT64  wCnt = 0 ;
      UINT64 len = fileContent.length() ;

      rc = _file.seek( 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "cipher file option [seek] err [%d] ", ossGetLastError() ) ;
         goto error ;
      }

      rc = _file.truncate( 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "cipher file option [truncate] err [%d] ", ossGetLastError() ) ;
         goto error ;
      }

      rc = _file.write( fileContent.c_str(), len, wCnt ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}