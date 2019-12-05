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

   Source File Name = tpMetaStore.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpMetaStore.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"
#include "ossIO.hpp"

namespace engine
{

   #define TP_META_FILE_EYECATCHER        "TP_METAH"
   #define TP_META_FILE_EYECATCHER_LEN    ( 8 )
   #define TP_META_FILE_CONTENT_LEN       ( 4 * 1024 )

   #define TP_META_FILE_VERSION           ( TP_VERSION )

   #define TP_META_FILE_PADDING_LEN       ( TP_META_FILE_CONTENT_LEN - \
                                            TP_META_FILE_EYECATCHER_LEN - \
                                            sizeof( UINT32 ) - \
                                            sizeof( DPS_LSN_OFFSET ) - \
                                            sizeof( DPS_LSN_VER ) - \
                                            sizeof( UINT64 ) )

   /*
      _tpMetaFileContent define
    */
   struct _tpMetaFileContent
   {
      CHAR           _eyeCatcher[ TP_META_FILE_EYECATCHER_LEN ] ;
      UINT32         _metaVersion ;
      DPS_LSN_OFFSET _time ;
      DPS_LSN_VER    _version ;
      UINT64         _flushTime ;
      CHAR           _padding[ TP_META_FILE_PADDING_LEN ] ;

      _tpMetaFileContent()
      {
         ossMemcpy( _eyeCatcher, TP_META_FILE_EYECATCHER,
                    TP_META_FILE_EYECATCHER_LEN ) ;
         _metaVersion = TP_META_FILE_VERSION ;
         _time = DPS_INVALID_LSN_OFFSET ;
         _version = DPS_INVALID_LSN_VERSION ;
         _flushTime = 0 ;
         ossMemset( _padding, 0, sizeof( _padding ) ) ;
      }
   } ;

   typedef struct _tpMetaFileContent tpMetaFileContent ;

   /*
      _tpMetaStore implement
    */
   _tpMetaStore::_tpMetaStore()
   : _time( 0LL ),
     _version( 0 ),
     _flushTime( 0LL )
   {
      _metaFileName[ 0 ] = '\0' ;
   }

   _tpMetaStore::~_tpMetaStore()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETASTORE_INIT, "_tpMetaStore::initialize" )
   INT32 _tpMetaStore::initialize( const CHAR *configPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETASTORE_INIT ) ;

      PD_CHECK( NULL != configPath, SDB_INVALIDARG, error, PDERROR,
                "Config path is empty" ) ;

      rc = utilBuildFullPath( configPath, SDBTP_META_FILE_NAME,
                              OSS_MAX_PATHSIZE, _metaFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build meta file path from config "
                   "path %s, rc: %d", configPath, rc ) ;

      rc = _readMeta() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Failed to read meta from meta file [%s], "
                 "need remove, rc: %d", _metaFileName, rc ) ;
         rc = ossDelete( _metaFileName ) ;
         if ( SDB_OK != rc && SDB_FNE != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to remove error meta file [%s], "
                         "rc: %d", rc ) ;
         }
         rc = SDB_FNE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETASTORE_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETASTORE_SAVE, "_tpMetaStore::save" )
   INT32 _tpMetaStore::save()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETASTORE_SAVE ) ;

      _flushTime = ossGetCurrentMicroseconds() ;

      rc = _writeMeta() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETASTORE_SAVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETASTORE__READMETA, "_tpMetaStore::_readMeta" )
   INT32 _tpMetaStore::_readMeta()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETASTORE__READMETA ) ;

      _OSS_FILE file ;
      BOOLEAN opened = TRUE ;
      INT64 readSize = 0 ;

      tpMetaFileContent content ;

      rc = ossOpen( _metaFileName, OSS_READWRITE, OSS_DEFAULTFILE, file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

      rc = ossReadN( &file, TP_META_FILE_CONTENT_LEN, (CHAR *)( &content ),
                     readSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta content from meta "
                   "file [%s], rc: %d", _metaFileName, rc ) ;

      PD_CHECK( readSize == TP_META_FILE_CONTENT_LEN, SDB_SYS, error, PDERROR,
                "Failed to read meta content from meta file [%s], "
                "length is mismatched, given [%lld], read [%lld]",
                _metaFileName, TP_META_FILE_CONTENT_LEN, readSize ) ;

      PD_CHECK( 0 == ossMemcmp( content._eyeCatcher,
                                TP_META_FILE_EYECATCHER,
                                TP_META_FILE_EYECATCHER_LEN ),
                SDB_SYS, error, PDERROR, "Failed to read meta content from "
                "meta file [%s], eye catcher is invalid", _metaFileName ) ;

      _time = content._time ;
      _version = content._version ;
      _flushTime = content._flushTime ;

   done:
      if ( opened )
      {
         ossClose( file ) ;
      }
      PD_TRACE_EXITRC( SDB__TPMETASTORE__READMETA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETASTORE__WRITEMETA, "_tpMetaStore::_writeMeta" )
   INT32 _tpMetaStore::_writeMeta()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETASTORE__WRITEMETA ) ;

      _OSS_FILE file ;
      BOOLEAN opened = TRUE ;

      tpMetaFileContent content ;

      content._time = _time ;
      content._version = _version ;
      content._flushTime = _flushTime ;

      rc = ossOpen( _metaFileName, OSS_CREATE | OSS_READWRITE, OSS_DEFAULTFILE,
                    file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

      rc = ossWriteN( &file, (CHAR *)( &content ), TP_META_FILE_CONTENT_LEN ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write meta content to meta "
                   "file [%s], rc: %d", _metaFileName, rc ) ;

      rc = ossFsync( &file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

   done:
      if ( opened )
      {
         ossClose( file ) ;
      }
      PD_TRACE_EXITRC( SDB__TPMETASTORE__WRITEMETA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
