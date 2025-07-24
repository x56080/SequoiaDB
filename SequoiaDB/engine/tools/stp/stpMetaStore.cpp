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

   Source File Name = stpMetaStore.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpMetaStore.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"
#include "ossIO.hpp"

namespace engine
{

   // eye-catcher of meta file
   #define STP_META_FILE_EYECATCHER       "STPMETAH"
   // length of eye-catcher of meta file
   #define STP_META_FILE_EYECATCHER_LEN   ( 8 )
   // length of content of meta file
   #define STP_META_FILE_CONTENT_LEN      ( 4 * 1024 )

   // use version of STP as version of meta file
   #define STP_META_FILE_VERSION          ( STP_VERSION )

   // padding length of meta file
   // | eye-catcher | version | offset ( time ) | version | flush time |
   #define STP_META_FILE_PADDING_LEN      ( STP_META_FILE_CONTENT_LEN - \
                                            STP_META_FILE_EYECATCHER_LEN - \
                                            sizeof( UINT32 ) - \
                                            sizeof( UINT64 ) - \
                                            sizeof( UINT32 ) - \
                                            sizeof( UINT64 ) )

   /*
      _stpMetaFileContent define
    */
   // _stpMetaFileContent is content of meta file
   struct _stpMetaFileContent
   {
      // eye catcher
      CHAR           _eyeCatcher[ STP_META_FILE_EYECATCHER_LEN ] ;
      // version of meta LSN
      // NOTE: when structure of meta LSN changed, meta version should be
      //       increased
      UINT32         _metaVersion ;
      // time of meta LSN ( as offset )
      UINT64         _time ;
      // version of meta LSN
      UINT32         _version ;
      // flush time of meta file
      UINT64         _flushTime ;
      // padding
      CHAR           _padding[ STP_META_FILE_PADDING_LEN ] ;

      // default constructor
      _stpMetaFileContent()
      {
         ossMemcpy( _eyeCatcher, STP_META_FILE_EYECATCHER,
                    STP_META_FILE_EYECATCHER_LEN ) ;
         _metaVersion = STP_META_FILE_VERSION ;
         _time = DPS_INVALID_LSN_OFFSET ;
         _version = DPS_INVALID_LSN_VERSION ;
         _flushTime = 0 ;
         ossMemset( _padding, 0, sizeof( _padding ) ) ;
      }
   } ;

   typedef struct _stpMetaFileContent stpMetaFileContent ;

   /*
      _stpMetaStore implement
    */
   _stpMetaStore::_stpMetaStore()
   : _metaLSN(),
     _flushTime( 0LL )
   {
      _metaFileName[ 0 ] = '\0' ;
   }

   _stpMetaStore::~_stpMetaStore()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETASTORE_INIT, "_stpMetaStore::initialize" )
   INT32 _stpMetaStore::initialize( const CHAR *configPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETASTORE_INIT ) ;

      // check config path
      PD_CHECK( NULL != configPath, SDB_INVALIDARG, error, PDERROR,
                "Config path is empty" ) ;

      // build file name of meta file
      rc = utilBuildFullPath( configPath, STP_META_FILE_NAME,
                              OSS_MAX_PATHSIZE, _metaFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build meta file path from config "
                   "path %s, rc: %d", configPath, rc ) ;

      // read meta file
      rc = _readMeta() ;
      if ( SDB_OK != rc )
      {
         // read failed, delete the malformed file
         PD_LOG( PDWARNING, "Failed to read meta from meta file [%s], "
                 "need remove, rc: %d", _metaFileName, rc ) ;
         rc = ossDelete( _metaFileName ) ;
         if ( SDB_OK != rc && SDB_FNE != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to remove error meta file [%s], "
                         "rc: %d", rc ) ;
         }

         // report file not exits
         rc = SDB_FNE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETASTORE_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETASTORE_SAVE, "_stpMetaStore::save" )
   INT32 _stpMetaStore::save()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETASTORE_SAVE ) ;

      // set flush time
      _flushTime = ossGetCurrentMicroseconds() ;

      // write meta to meta file
      rc = _writeMeta() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETASTORE_SAVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETASTORE__READMETA, "_stpMetaStore::_readMeta" )
   INT32 _stpMetaStore::_readMeta()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETASTORE__READMETA ) ;

      _OSS_FILE file ;
      BOOLEAN opened = TRUE ;
      INT64 readSize = 0 ;

      stpMetaFileContent content ;

      // open meta file
      rc = ossOpen( _metaFileName, OSS_READWRITE, OSS_DEFAULTFILE, file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

      // read meta content
      rc = ossReadN( &file, STP_META_FILE_CONTENT_LEN, (CHAR *)( &content ),
                     readSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta content from meta "
                   "file [%s], rc: %d", _metaFileName, rc ) ;

      // check length of meta content
      PD_CHECK( readSize == STP_META_FILE_CONTENT_LEN, SDB_SYS, error, PDERROR,
                "Failed to read meta content from meta file [%s], "
                "length is mismatched, given [%lld], read [%lld]",
                _metaFileName, STP_META_FILE_CONTENT_LEN, readSize ) ;

      // check eye-catcher of meta content
      PD_CHECK( 0 == ossMemcmp( content._eyeCatcher,
                                STP_META_FILE_EYECATCHER,
                                STP_META_FILE_EYECATCHER_LEN ),
                SDB_SYS, error, PDERROR, "Failed to read meta content from "
                "meta file [%s], eye catcher is invalid", _metaFileName ) ;

      // get fields of meta content
      _metaLSN.set( content._time, content._version ) ;
      _flushTime = content._flushTime ;

   done:
      // close meta file
      if ( opened )
      {
         ossClose( file ) ;
      }
      PD_TRACE_EXITRC( SDB__STPMETASTORE__READMETA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETASTORE__WRITEMETA, "_stpMetaStore::_writeMeta" )
   INT32 _stpMetaStore::_writeMeta()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETASTORE__WRITEMETA ) ;

      _OSS_FILE file ;
      BOOLEAN opened = TRUE ;

      stpMetaFileContent content ;

      // copy fields to meta content
      content._time = (UINT64)( _metaLSN.offset ) ;
      content._version = (UINT32)( _metaLSN.version ) ;
      content._flushTime = _flushTime ;

      // open meta file
      rc = ossOpen( _metaFileName, OSS_CREATE | OSS_READWRITE, OSS_DEFAULTFILE,
                    file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

      // write meta content to meta file
      rc = ossWriteN( &file, (CHAR *)( &content ), STP_META_FILE_CONTENT_LEN ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write meta content to meta "
                   "file [%s], rc: %d", _metaFileName, rc ) ;

      // flush meta file
      rc = ossFsync( &file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync meta file [%s], rc: %d",
                   _metaFileName, rc ) ;

   done:
      // close meta file
      if ( opened )
      {
         ossClose( file ) ;
      }
      PD_TRACE_EXITRC( SDB__STPMETASTORE__WRITEMETA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
