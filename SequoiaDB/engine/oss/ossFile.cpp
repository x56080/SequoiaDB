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

   Source File Name = ossFile.cpp

   Descriptive Name = File operation methods

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/12/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossFile.hpp"
#include "pd.hpp"

namespace engine
{

   ossFile::ossFile()
   {
   }

   ossFile::~ossFile()
   {
      if ( isOpened() )
      {
         close() ;
      }
   }

   INT32 ossFile::open( const string& filePath, UINT32 mode, UINT32 permission )
   {
      INT32 rc = SDB_OK ;

      rc = ossOpen( filePath.c_str(), mode, permission, _file ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _path = filePath ;

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 ossFile::close()
   {
      if ( _file.isOpened() )
      {
         _path = "" ;
         return ossClose( _file ) ;
      }
      else
      {
         return SDB_OK ;
      }
   }

   BOOLEAN ossFile::isOpened() const
   {
      return _file.isOpened() ;
   }
   
   INT32 ossFile::seek( INT64 offset, OSS_SEEK whence, INT64* position )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeek( &_file, offset, whence, position ) ;
   }
   
   INT32 ossFile::sync()
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossFsync( &_file ) ;
   }

   INT32 ossFile::extend( const INT64 incrementSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossExtendFile( &_file, incrementSize ) ;
   }

   INT32 ossFile::truncate( const INT64 fileLen )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossTruncateFile( &_file, fileLen ) ;
   }
   
   INT32 ossFile::getSize( INT64& size )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossGetFileSize( &_file, &size ) ;
   }
   
   const string& ossFile::getPath() const
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return _path ;
   }
      
   INT32 ossFile::read( CHAR* buffer, INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossRead( &_file, buffer, bufferLen, &readSize ) ;
   }
   
   INT32 ossFile::readN( CHAR* buffer, INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossReadN( &_file, bufferLen, buffer, readSize ) ;
   }
   
   INT32 ossFile::seekAndRead( INT64 offset, CHAR* buffer,
                               INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndRead( &_file, offset, buffer, bufferLen, &readSize ) ;
   }
   
   INT32 ossFile::seekAndReadN( INT64 offset, CHAR* buffer,
                                INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndReadN( &_file, offset, bufferLen, buffer, readSize ) ;
   }
      
   INT32 ossFile::write( const CHAR* buffer, INT64 bufferLen, INT64& writeSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossWrite( &_file, buffer, bufferLen, &writeSize ) ;
   }
   
   INT32 ossFile::writeN( const CHAR* buffer, INT64 bufferLen )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossWriteN( &_file, buffer, bufferLen ) ;
   }
   
   INT32 ossFile::seekAndWrite( INT64 offset, const CHAR* buffer,
                                INT64 bufferLen, INT64& writeSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndWrite( &_file, offset, buffer, bufferLen, &writeSize ) ;
   }
   
   INT32 ossFile::seekAndWriteN( INT64 offset, const CHAR* buffer, 
                                 INT64 bufferLen )
   {
      INT64 writeSize = 0 ;
      INT32 rc = SDB_OK ;
      
      SDB_ASSERT( isOpened(), "file should be opened" ) ;

      rc = ossSeekAndWriteN( &_file, offset, buffer, bufferLen, writeSize ) ;
      if ( SDB_OK == rc )
      {
         SDB_ASSERT( writeSize == bufferLen, "writeSize != bufferLen" ) ;
      }

      return rc ;
   }

   INT32 ossFile::exists( const string& filePath, BOOLEAN& exist )
   {
      INT32 rc = SDB_OK ;

      rc = ossAccess( filePath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            exist = FALSE ;
            rc = SDB_OK ;
            goto done ;
         }

         goto error ;
      }

      exist = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

