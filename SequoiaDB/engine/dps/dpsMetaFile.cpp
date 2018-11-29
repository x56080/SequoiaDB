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

   Source File Name = dpsMetaFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2018  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsMetaFile.hpp"
#include "utilStr.hpp"

namespace engine
{
   _dpsMetaFile::_dpsMetaFile()
   {
      ossMemset( _path, 0, sizeof( _path ) ) ;
   }

   _dpsMetaFile::~_dpsMetaFile()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
   }

   INT32 _dpsMetaFile::init( const CHAR *parentDir )
   {
      INT32 rc = SDB_OK ;
      if ( NULL == parentDir )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = utilBuildFullPath( parentDir, DPS_METAFILE_NAME, OSS_MAX_PATHSIZE,
                              _path ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Build full path failed:parentDir=%s,rc=%d",
                 parentDir, rc ) ;
         goto error ;
      }

      if ( SDB_OK == ossAccess( _path ) )
      {
         rc = _restore() ;
         if ( SDB_OK == rc )
         {
            goto done ;
         }

         PD_LOG( PDWARNING, "Restore meta file failed:file=%s,rc=%d",
                 _path, rc ) ;
         PD_LOG( PDWARNING, "Try to delete meta file and recreate it:file=%s",
                 _path ) ;
         ossDelete( _path ) ;
      }

      rc = _initNewFile() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Init new meta file failed:file=%s,rc=%d",
                 _path, rc ) ;
         goto error ;
      }

      PD_LOG( PDINFO, "create dps meta file success:file=%s", _path ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsMetaFile::_restore()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isOpened = FALSE ;
      SINT64 read = 0 ;
      DPS_LSN_OFFSET offset ;

      rc = ossOpen( _path, OSS_READWRITE, OSS_RWXU, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file:file=%s,rc=%d",
                  _path, rc ) ;
      isOpened = TRUE ;

      rc = ossReadN( &_file, sizeof( _header ), (CHAR *)&_header, read ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta header:file=%s,rc=%d",
                  _path, rc ) ;

      // read size must equal header's size.
      PD_CHECK( read == sizeof( _header ), SDB_SYS, error, PDERROR,
                "Failed to read meta header:file=%s,rc=%d", _path, SDB_SYS ) ;

      if ( 0 != ossMemcmp( _header._eyeCatcher, DPS_METAFILE_HEADER_EYECATCHER,
                           DPS_METAFILE_HEADER_EYECATCHER_LEN ) )
      {
         // header's magic number must be DPS_METAFILE_HEADER_EYECATCHER
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Meta header is invalid:rc=%d", rc ) ;
         goto error ;
      }

      // try to read lsn
      rc = readOldestLSNOffset( offset ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read oldest lsn:file=%s,rc=%d",
                  _path, rc ) ;

   done:
      return rc ;
   error:
      if ( isOpened )
      {
         ossClose( _file ) ;
      }
      goto done ;
   }

   INT32 _dpsMetaFile::_initNewFile()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isCreated = FALSE ;
      SINT64 written = 0 ;
      _dpsMetaFileContent content( DPS_INVALID_LSN_OFFSET ) ;

      rc = ossOpen( _path, OSS_CREATEONLY | OSS_READWRITE,
                    OSS_RWXU, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create meta file:file=%s,rc=%d",
                   _path, rc ) ;
      isCreated = TRUE ;

      // increase the file size to the total meta file size
      rc = ossExtendFile( &_file, DPS_METAFILE_HEADER_LEN +
                          DPS_METAFILE_CONTENT_LEN  ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extend meta file:size=%d,rc=%d",
                   DPS_METAFILE_HEADER_LEN + DPS_METAFILE_CONTENT_LEN, rc ) ;

      // write header
      rc = ossSeekAndWriteN( &_file, 0, (const CHAR *)&_header,
                             sizeof( _header ), written ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write header:rc=%d", rc ) ;

      // write content
      rc = ossSeekAndWriteN( &_file, DPS_METAFILE_HEADER_LEN,
                             (const CHAR *)&content,
                             sizeof( content ), written ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write content:rc=%d", rc ) ;

      //sync the file
      rc = ossFsync( &_file ) ;
      PD_RC_CHECK( rc, PDERROR, "Fsync failed:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      if ( isCreated )
      {
         ossClose( _file ) ;
      }
      goto done ;
   }

   INT32 _dpsMetaFile::writeOldestLSNOffset( DPS_LSN_OFFSET offset )
   {
      INT32 rc = SDB_OK ;
      SINT64 written = 0 ;
      _dpsMetaFileContent content( offset ) ;

      SINT64 toWrite = sizeof( content ) ;
      rc = ossSeekAndWriteN( &_file, DPS_METAFILE_HEADER_LEN,
                             ( const CHAR* ) &content, toWrite, written ) ;
      PD_RC_CHECK( rc, PDERROR, "Write content failed:offset=%llu,rc=%d",
                   offset, rc ) ;

      rc = ossFsync( &_file ) ;
      PD_RC_CHECK( rc, PDERROR, "Fsync failed:path=%s,rc=%d",
                   _path, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dpsMetaFile::readOldestLSNOffset( DPS_LSN_OFFSET &offset )
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;
      _dpsMetaFileContent content( DPS_INVALID_LSN_OFFSET ) ;

      SINT64 toRead = sizeof( content ) ;
      rc = ossSeekAndReadN( &_file, DPS_METAFILE_HEADER_LEN, toRead,
                            ( CHAR * ) &content, read ) ;
      PD_RC_CHECK( rc, PDERROR, "Read content failed:rc=%d", rc ) ;

      PD_CHECK( toRead == read, SDB_SYS, error, PDERROR,
                "Read content failed:read=%lld,expect=%lld",
                read, toRead ) ;

      offset = content._oldestLSNOffset ;

   done:
      return rc ;
   error:
      goto done ;
   }
}


