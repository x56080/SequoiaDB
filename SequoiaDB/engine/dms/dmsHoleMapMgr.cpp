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

   Source File Name = dmsHoleMapMgr.cpp

   Descriptive Name =

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/01/2023  Tangtao Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsHoleMapMgr.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{
   #define DMS_HOLEMAP_DEFAULT_SIZE       (64*1024)
   #define DMS_HOLEMAP_BIT2UNIT_SQUARE    3
   #define DMS_HOLEMAP_WHITOUTLOB_SIZE    3

   _dmsHoleMapMgr::_dmsHoleMapMgr( const CHAR *pSuFileName,
                                   dmsStorageInfo *pInfo )
   {
      SDB_ASSERT( pSuFileName, "SU file name can't be NULL" ) ;

      _pStorageInfo  = pInfo ;
      _dmsHeader     = NULL ;
      _needRebuild   = FALSE ;
      _maxHoleNum    = 0 ;
      _maxLobHoleNum = 0 ;
      ossStrncpy( _suFileName, pSuFileName, DMS_SU_FILENAME_SZ ) ;
      _suFileName[ DMS_SU_FILENAME_SZ ] = 0 ;
      ossMemset( _fullPathName, 0, sizeof(_fullPathName) ) ;
   }

   _dmsHoleMapMgr::~_dmsHoleMapMgr()
   {
      ossPoolMap<INT32,dmsHME*>::iterator it ;
      for ( it = _HME.begin() ; it != _HME.end() ; it++ )
      {
         SDB_OSS_DEL it->second ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_OPEN, "_dmsHoleMapMgr::open" )
   INT32 _dmsHoleMapMgr::open( const CHAR *pPath )
   {
      INT32 rc    = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_OPEN ) ;
      SDB_ASSERT( pPath, "path can't be NULL" ) ;
      if ( NULL == _pStorageInfo )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = utilBuildFullPath( pPath, _suFileName, OSS_MAX_PATHSIZE,
                              _fullPathName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s; %s", pPath,
                  _suFileName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _pStorageInfo->_hadShrinkSpace )
      {
         rc = _openHoleMap( FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to open hole map file %s, rc: %d",
                    _suFileName ,rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsHoleMapMgr::getSuFileName () const
   {
      return _suFileName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_OPENHOLEMAP, "_dmsHoleMapMgr::_openHoleMap" )
   INT32 _dmsHoleMapMgr::_openHoleMap( BOOLEAN createNew )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_OPENHOLEMAP ) ;
      UINT64 fileSize         = 0 ;
      UINT64 rightSize        = 0 ;
      UINT64 rightSizeMin     = 0 ;
      UINT32 mode = OSS_READWRITE|OSS_EXCLUSIVE ;
      UINT32 dataHoleMapSize  = 0 ;
      UINT32 idxHoleMapSize   = 0 ;
      UINT32 lobHoleMapSize   = 0 ;

      if ( NULL == _pStorageInfo )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( createNew )
      {
         mode |= OSS_REPLACE ;
      }

      if ( _opened )
      {
         goto done ;
      }

      PD_LOG ( PDDEBUG, "Open hole map file %s", _fullPathName ) ;

      // open the file, create one if not exist
      rc = ossMmapFile::open ( _fullPathName, mode, OSS_RU|OSS_WU|OSS_RG ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc && !createNew )
         {
            _needRebuild = TRUE ;
            mode |= OSS_CREATEONLY ;
            // open the file, create one if not exist
            rc = ossMmapFile::open ( _fullPathName, mode, OSS_RU|OSS_WU|OSS_RG ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to recreate hole map file: %s, "
                         "rc: %d", _fullPathName, rc ) ;

            PD_LOG ( PDWARNING, "The hole map is missing, try to recreate "
                     "hole map file[%s], mode:0x%08x", _fullPathName, mode ) ;
            createNew = TRUE ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to open %s, rc=%d", _fullPathName, rc ) ;
            goto error ;
         }
      }
      if ( createNew )
      {
         PD_LOG( PDEVENT, "Create hole map file[%s] succeed, "
                 "mode: 0x%08x", _fullPathName, mode ) ;
      }

      // ensure that the file size is as expected
      {
         UINT32 pageSize         = 0 ;
         UINT32 lobPageSize      = 0 ;
         BOOLEAN hadCreateLob    = 0 ;

         pageSize       = _pStorageInfo->_pageSize ;
         lobPageSize    = _pStorageInfo->_lobdPageSize ;
         hadCreateLob   = _pStorageInfo->_createLobs ;

         _maxHoleNum    = ( (UINT64)pageSize * DMS_MAX_PG ) / DMS_FILEHOLE_BLOCK_SZ ;
         _maxLobHoleNum = ( (UINT64)lobPageSize * DMS_MAX_PG ) / DMS_FILEHOLE_BLOCK_SZ ;

         dataHoleMapSize = _maxHoleNum >> DMS_HOLEMAP_BIT2UNIT_SQUARE ;
         idxHoleMapSize = _maxHoleNum >> DMS_HOLEMAP_BIT2UNIT_SQUARE ;
         lobHoleMapSize = _maxLobHoleNum >> DMS_HOLEMAP_BIT2UNIT_SQUARE ;
         if ( dataHoleMapSize < DMS_HOLEMAP_DEFAULT_SIZE )
         {
            dataHoleMapSize = DMS_HOLEMAP_DEFAULT_SIZE ;
         }
         if ( idxHoleMapSize < DMS_HOLEMAP_DEFAULT_SIZE )
         {
            idxHoleMapSize = DMS_HOLEMAP_DEFAULT_SIZE ;
         }
         if ( lobHoleMapSize < DMS_HOLEMAP_DEFAULT_SIZE )
         {
            lobHoleMapSize = DMS_HOLEMAP_DEFAULT_SIZE ;
         }

         rightSizeMin = sizeof( _dmsStorageUnitHeader ) + dataHoleMapSize +
                        idxHoleMapSize ;
         rightSize = rightSizeMin + ( hadCreateLob ? lobHoleMapSize : 0 ) ;

         // we get the file size to make sure it's what we need
         rc = ossMmapFile::size ( fileSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }

         if ( fileSize != 0 &&
              fileSize != rightSizeMin &&
              fileSize != rightSize )
         {
            PD_LOG( PDWARNING, "File[%s] size[%llu] is invalid, need rebuild.",
                    _suFileName, fileSize ) ;
            rc = ossTruncateFile( &_file, 0 ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Truncate file[%s] to size[%llu] failed, rc: %d",
                       _suFileName, 0, rc ) ;
               goto error ;
            }
            fileSize = 0 ;
         }

         // is it a brand new file
         if ( 0 == fileSize )
         {
            // if it's a brand new file but we don't ask for creating new
            if ( !createNew )
            {
               _needRebuild = TRUE ;
               PD_LOG ( PDWARNING, "Hole map file[%s] is empty, need to initialize.",
                        _suFileName ) ;
            }
            rc = _initializeHMMgr( rightSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Initialize hole map unit failed, rc: %d", rc ) ;
               goto error ;
            }
            // then we get the size again to make sure it's what we need
            rc = ossMmapFile::size ( fileSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to get file size: %s, rc: %d",
                        _suFileName, rc ) ;
               goto error ;
            }
         }
      }

      // mmap
      {
         UINT64 curOffset        = 0 ;
         BOOLEAN hadCreateLob    = 0 ;
         CHAR* tmpPtr            = NULL ;

         hadCreateLob = _pStorageInfo->_createLobs ;

         // map header, 64K
         rc = map ( DMS_HEADER_OFFSET, DMS_HEADER_SZ, (void**)&_dmsHeader ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Map file[%s] header failed, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
         curOffset += DMS_HEADER_SZ ;

         // map dataSu hole map
         rc = map( curOffset, dataHoleMapSize, (void**)&tmpPtr ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Map file[%s] dataSu hole map failed, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
         curOffset += dataHoleMapSize ;
         try
         {
            _suPageSize[DMS_FILE_DATA] = _pStorageInfo->_pageSize ;
            _HME[DMS_FILE_DATA] = SDB_OSS_NEW dmsHME( tmpPtr, _maxHoleNum ) ;
            if ( !_HME[DMS_FILE_DATA] )
            {
               PD_LOG( PDERROR, "Unable to allocate memory for holeMapExtent" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }

         // map indexSu hole map
         rc = map( curOffset, idxHoleMapSize, (void**)&tmpPtr ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Map file[%s] indexSu hole map failed, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
         curOffset += idxHoleMapSize ;
         try
         {
            _suPageSize[DMS_FILE_IDX] = _pStorageInfo->_pageSize ;
            _HME[DMS_FILE_IDX] = SDB_OSS_NEW dmsHME( tmpPtr, _maxHoleNum ) ;
            if ( !_HME[DMS_FILE_IDX] )
            {
               PD_LOG( PDERROR, "Unable to allocate memory for holeMapExtent" ) ;
               rc = SDB_OOM ;
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }

         if ( hadCreateLob && ( curOffset < fileSize ) )
         {
            // map lobSu hole map
            rc = map( curOffset, lobHoleMapSize, (void**)&tmpPtr ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Map file[%s] lobSu hole map failed, rc: %d",
                        _suFileName, rc ) ;
               goto error ;
            }
            curOffset += lobHoleMapSize ;
            try
            {
               _suPageSize[DMS_FILE_LOB] = _pStorageInfo->_lobdPageSize ;
               _HME[DMS_FILE_LOB] = SDB_OSS_NEW dmsHME( tmpPtr, _maxLobHoleNum ) ;
               if ( !_HME[DMS_FILE_LOB] )
               {
                  PD_LOG( PDERROR, "Unable to allocate memory for holeMapExtent" ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
            }
            catch ( std::exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_OPENHOLEMAP, rc ) ;
      return rc ;
   error:
      ossMmapFile::close () ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_REFRESHHOLEMAP, "_dmsHoleMapMgr::_refreshHoleMap" )
   INT32 _dmsHoleMapMgr::_refreshHoleMap()
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_REFRESHHOLEMAP ) ;
      UINT32 lobPageSize      = 0 ;
      UINT64 curOffset        = 0 ;
      UINT64 fileSize         = 0 ;
      UINT32 lobHoleMapSize   = 0 ;
      CHAR* tmpPtr            = NULL ;

      lobPageSize    = _pStorageInfo->_lobdPageSize ;
      _maxLobHoleNum = ( (UINT64)lobPageSize * DMS_MAX_PG ) / DMS_FILEHOLE_BLOCK_SZ ;
      lobHoleMapSize = _maxLobHoleNum >> DMS_HOLEMAP_BIT2UNIT_SQUARE ;
      if ( lobHoleMapSize < DMS_HOLEMAP_DEFAULT_SIZE )
      {
         lobHoleMapSize = DMS_HOLEMAP_DEFAULT_SIZE ;
      }

      if ( DMS_HOLEMAP_WHITOUTLOB_SIZE < segmentSize() )
      {
         goto done ;
      }
      else if ( DMS_HOLEMAP_WHITOUTLOB_SIZE > segmentSize() )
      {
         rc = SDB_INVALIDSIZE ;
         PD_LOG ( PDERROR, "Hole map manager has %d map which is less than expected, "
                  "rc: %d", segmentSize() - 1, rc ) ;
         goto error ;
      }

      rc = ossGetFileSize ( &_file, (INT64 *)&fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get file size, rc = %d", rc ) ;
      curOffset = fileSize ;

      // extend file size
      rc = ossExtend( &_file, fileSize, lobHoleMapSize, FALSE ) ;
      if ( rc )
      {
         INT32 rc1 = SDB_OK ;
         PD_LOG ( PDWARNING, "Failed to extend hole map for %llu "
                  "bytes, rc: %d", lobHoleMapSize, rc ) ;

         // truncate the file when it's failed to extend file
         rc1 = ossTruncateFile ( &_file, fileSize ) ;
         if ( rc1 )
         {
            // if we increased the file size but got error, and we are not able
            // to decrease it, something wrong.
            PD_LOG ( PDSEVERE, "Failed to revert the increase of segment, "
                     "rc: %d", rc1 ) ;
         }

         // we need to manage how to truncate the file to original size here
         goto error ;
      }

      // map lobSu hole map
      rc = map( curOffset, lobHoleMapSize, (void**)&tmpPtr ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "cs[%s] failed to map lobSu hole map, rc: %d",
                  _suFileName, rc ) ;
         goto error ;
      }
      curOffset += lobHoleMapSize ;
      try
      {
         _suPageSize[DMS_FILE_LOB] = lobPageSize ;
         _HME[DMS_FILE_LOB] = SDB_OSS_NEW dmsHME( tmpPtr, _maxLobHoleNum ) ;
         if ( !_HME[DMS_FILE_LOB] )
         {
            PD_LOG( PDERROR, "Unable to allocate memory for holeMapExtent" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_REFRESHHOLEMAP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dmsHoleMapMgr::flushHME( BOOLEAN sync )
   {
      _ossMmapFile::flushAll( sync ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_RENAMESTORAGE, "_dmsHoleMapMgr::renameStorage" )
   INT32 _dmsHoleMapMgr::renameStorage( const CHAR *csName,
                                        const CHAR *suFileName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_RENAMESTORAGE ) ;
      CHAR tmpPathFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      ossStrcpy( tmpPathFile, _fullPathName ) ;

      CHAR *pos = ossStrstr( tmpPathFile, _suFileName ) ;
      if ( !pos )
      {
         PD_LOG( PDERROR, "File full path[%s] is not include su file[%s]",
                 _fullPathName, _suFileName ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      *pos = '\0' ;
      utilCatPath( tmpPathFile, OSS_MAX_PATHSIZE, suFileName ) ;

#ifdef _WINDOWS
      /// modify the header
      ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
      _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
      flushHME( TRUE ) ;

      {
         /// close
         closeStorage() ;

         /// rename
         rc = ossRenamePath( _fullPathName, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPathName, tmpPathFile, rc ) ;
            goto error ;
         }

         /// open
         *pos = '\0' ;
         ossStrncpy( _suFileName, suFileName, DMS_SU_FILENAME_SZ ) ;
         _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;
         ossStrncpy( _pStorageInfo->_suName, csName, DMS_SU_NAME_SZ ) ;
         _pStorageInfo->_suName[ DMS_SU_NAME_SZ ] = 0 ;
         rc = open( tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Open storage file failed, rc: %d", rc ) ;
            goto error ;
         }
      }
#else

      if ( _pStorageInfo->_hadShrinkSpace )
      {
         if ( !_dmsHeader || !_opened )
         {
            _openHoleMap( FALSE ) ;
         }

         /// rename filename
         rc = ossRenamePath( _fullPathName, tmpPathFile ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rename file[%s] to %s failed, rc: %d",
                    _fullPathName, tmpPathFile, rc ) ;
            goto error ;
         }
      }

      ossStrncpy( _suFileName, suFileName, DMS_SU_FILENAME_SZ ) ;
      _suFileName[ DMS_SU_FILENAME_SZ ] = '\0' ;

      ossStrcpy( _fullPathName, tmpPathFile ) ;

      if ( _opened && _dmsHeader )
      {
         /// modify the header
         ossStrncpy( _dmsHeader->_name, csName, DMS_SU_NAME_SZ ) ;
         _dmsHeader->_name[ DMS_SU_NAME_SZ ] = 0 ;
         flushAll( TRUE ) ;
      }

#endif // _WINDOWS

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_RENAMESTORAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dmsHoleMapMgr::closeStorage ()
   {
      if ( ossMmapFile::_opened )
      {
         flushAll( TRUE ) ;

         ossMmapFile::close() ;

         _dmsHeader = NULL ;

         ossPoolMap<INT32,dmsHME*>::iterator it ;
         for ( it = _HME.begin() ; it != _HME.end() ; it++ )
         {
            SDB_OSS_DEL it->second ;
         }
         _HME.clear() ;
         _suPageSize.clear() ;
      }
   }

   INT32 _dmsHoleMapMgr::removeStorage ()
   {
      INT32 rc = SDB_OK ;

      if ( _fullPathName[0] == 0 )
      {
         goto done ;
      }

      // close
      closeStorage() ;

      if ( !_pStorageInfo->_hadShrinkSpace )
      {
         goto done ;
      }

      rc = ossDelete( _fullPathName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove storeage unit file: %s, "
                   "rc: %d", _fullPathName, rc ) ;

      PD_LOG( PDEVENT, "Remove storage unit file[%s] succeed", _fullPathName ) ;
      _fullPathName[ 0 ] = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsHoleMapMgr::_initializeHMMgr ( UINT64 size )
   {
      INT32   rc        = SDB_OK ;
      _dmsHeader        = NULL ;
      INT64   hadWrite  = 0 ;

      // allocate file size
      rc = ossExtend( &_file, 0, size, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Extend file[%s] to size[%llu] failed, rc: %d",
                  _suFileName, size, rc ) ;
         goto error ;
      }

      // move to beginning of the file
      rc = ossSeek ( &_file, 0, OSS_SEEK_SET ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to seek to beginning of the file, rc: %d",
                  rc ) ;
         goto error ;
      }

      // allocate buffer for dmsHeader
      _dmsHeader = SDB_OSS_NEW dmsStorageUnitHeader ;
      if ( !_dmsHeader )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsHeader" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_HEADER_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // initialize a new header with empty size
      _initHeader ( _dmsHeader ) ;

      // write the buffer into file
      rc = ossWrite ( &_file, (const CHAR *)_dmsHeader, DMS_HEADER_SZ, &hadWrite ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng HMMgr init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsHeader ;
      _dmsHeader = NULL ;

   done :
      return rc ;
   error :
      if ( _dmsHeader )
      {
         SDB_OSS_DEL _dmsHeader ;
         _dmsHeader = NULL ;
      }
      goto done ;
   }

   void _dmsHoleMapMgr::_initHeader( dmsStorageUnitHeader * pHeader )
   {
      ossStrncpy ( pHeader->_eyeCatcher, DMS_HMM_EYECATCHER,
                   DMS_HEADER_EYECATCHER_LEN ) ;
      pHeader->_version          = 0 ;
      pHeader->_pageSize         = 0 ;
      pHeader->_lobdPageSize     = 0 ;
      pHeader->_segmentSize      = 0 ;
      pHeader->_storageUnitSize  = 0 ;
      ossStrncpy ( pHeader->_name, _pStorageInfo->_suName, DMS_SU_NAME_SZ ) ;
      pHeader->_sequence         = 0 ;
      pHeader->_numMB            = 0 ;
      pHeader->_MBHWM            = 0 ;
      pHeader->_pageNum          = 0 ;
      pHeader->_secretValue      = 0 ;
      pHeader->_createLobs       = 0 ;
      pHeader->_commitFlag       = 0 ;
      pHeader->_commitLsn        = ~0 ;
      pHeader->_commitTime       = 0 ;
      pHeader->_csUniqueID       = 0 ;
      pHeader->_idxInnerHWM      = 0 ;
      pHeader->_hasHoleMap       = 0 ;
   }

   UINT32 _dmsHoleMapMgr::_getSuPageSize( INT32 type )
   {
      return _suPageSize[ type ] ;
   }

   INT32 _dmsHoleMapMgr::_getHME( INT32 type, dmsHME **pHME  )
   {
      INT32 rc = SDB_OK ;
      ossScopedTryLock _lock( &_HMMgrMutex, EXCLUSIVE ) ;

      if ( !_opened )
      {
         rc = _openHoleMap( TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to open su[%s]'s hole map. rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
      }

      if ( 0 == _HME.count( type ) &&
           DMS_FILE_LOB == type )
      {
         rc = _refreshHoleMap() ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to append su[%s]'s lob hole map, rc: %d",
                     _suFileName, rc ) ;
            goto error ;
         }
      }

      if ( 0 == _HME.count( type ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to get HME type[%d], rc: %d",
                  type, rc ) ;
         goto error ;
      }

      *pHME = _HME[ type ] ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_RESETHOLEMAPMASK, "_dmsHoleMapMgr::resetHoleMapMask" )
   INT32 _dmsHoleMapMgr::resetHoleMapMask( INT32 type )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_RESETHOLEMAPMASK ) ;
      dmsHME *pHME = NULL ;

      rc = _getHME( type, &pHME ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get HME, rc: %d", rc ) ;
         goto error ;
      }

      pHME->resetBitmap() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_RESETHOLEMAPMASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_CLEARHOLEMAPMASK, "_dmsHoleMapMgr::clearHoleMapMask" )
   INT32 _dmsHoleMapMgr::clearHoleMapMask( INT32 type, dmsExtentID &foundPage,
                                           INT32 &numPages, INT64 *pOffset,
                                           INT64 *pLenght )
   {
      INT32 rc             = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_CLEARHOLEMAPMASK ) ;
      UINT32 blockStart    = 0 ;
      UINT32 blockEnd      = 0 ;
      INT64 offset         = 0 ;
      INT64 length         = 0 ;
      INT64 tmpLen         = 0 ;
      UINT32 holeID        = 0 ;
      UINT32 pageSize      = 0 ;
      BOOLEAN hadClearMask = FALSE ;
      dmsHME *pHME         = NULL ;

      rc = _getHME( type, &pHME ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get HME, rc: %d", rc ) ;
         goto error ;
      }
      pageSize = _getSuPageSize( type ) ;

      // clear the fileHole mask, need to ensure every hole blocks
      // which contain giving page had allocate space
      //
      //              hole blocks
      // --|----------|----------|----------|----
      //
      //       start               end
      //         |------------------|
      //
      // extend to the boundary
      // start                             end
      //   |--------------------------------|
      offset = (INT64)foundPage * pageSize ;
      length = (INT64)numPages * pageSize ;
      blockStart = offset / DMS_FILEHOLE_BLOCK_SZ ;
      tmpLen = offset % DMS_FILEHOLE_BLOCK_SZ ;
      if ( tmpLen )
      {
         length += tmpLen ;
      }

      blockEnd = blockStart + ( length / DMS_FILEHOLE_BLOCK_SZ ) ;
      if ( length % DMS_FILEHOLE_BLOCK_SZ )
      {
         blockEnd += 1 ;
      }

      for ( holeID = blockStart ; holeID < blockEnd ; holeID++ )
      {
         if ( pHME->testBit( holeID ) )
         {
            pHME->clearBit( holeID ) ;
            hadClearMask = TRUE ;
         }
      }

      // adjust page info to actual operation value
      foundPage = (UINT64)blockStart * DMS_FILEHOLE_BLOCK_SZ / pageSize ;
      numPages = (UINT64)( blockEnd - blockStart ) * DMS_FILEHOLE_BLOCK_SZ / pageSize ;

      if ( pOffset && pLenght && hadClearMask )
      {
         *pOffset = (INT64)blockStart * DMS_FILEHOLE_BLOCK_SZ ;
         *pLenght = (INT64)( blockEnd - blockStart ) * DMS_FILEHOLE_BLOCK_SZ ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_CLEARHOLEMAPMASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK1, "_dmsHoleMapMgr::setHoleMapMask" )
   INT32 _dmsHoleMapMgr::setHoleMapMask( INT32 type, dmsExtentID &foundPage,
                                         INT32 &numPages, INT64 *pOffset,
                                         INT64 *pLenght )
   {
      INT32 rc             = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK1 ) ;
      UINT32 blockStart    = 0 ;
      UINT32 blockEnd      = 0 ;
      INT64 offset         = 0 ;
      INT64 length         = 0 ;
      INT64 tmpLen         = 0 ;
      UINT32 holeID        = 0 ;
      UINT32 pageSize      = 0 ;
      BOOLEAN hadSetMask   = FALSE ;
      dmsHME *pHME         = NULL ;

      rc = _getHME( type, &pHME ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get HME, rc: %d", rc ) ;
         goto error ;
      }
      pageSize = _getSuPageSize( type ) ;

      // set the fileHole mask, make sure all the page in hole blocks
      // need to punch_hole, otherwise do not punch hole
      //
      //              hole blocks
      // --|----------|----------|----------|----
      //
      //       start               end
      //         |------------------|
      //
      // adjust to the boundary
      //           start         end
      //              |----------|
      offset = (INT64)foundPage * pageSize ;
      length = (INT64)numPages * pageSize ;
      blockStart = offset / DMS_FILEHOLE_BLOCK_SZ ;
      tmpLen = offset % DMS_FILEHOLE_BLOCK_SZ ;
      if ( tmpLen )
      {
         length -= ( DMS_FILEHOLE_BLOCK_SZ - tmpLen ) ;
         if ( length < 0 )
         {
            length = 0 ;
         }
         blockStart += 1 ;
      }

      blockEnd = blockStart + ( length / DMS_FILEHOLE_BLOCK_SZ ) ;

      for ( holeID = blockStart ; holeID < blockEnd ; holeID++ )
      {
         if ( !pHME->testBit( holeID ) )
         {
            hadSetMask = TRUE ;
            pHME->setBit( holeID ) ;
         }
      }
      if ( hadSetMask )
      {
         flushHME( TRUE ) ;
      }

      // adjust page info to actual operation value
      foundPage = (UINT64)blockStart * DMS_FILEHOLE_BLOCK_SZ / pageSize ;
      numPages = (UINT64)( blockEnd - blockStart ) * DMS_FILEHOLE_BLOCK_SZ / pageSize ;

      if ( pOffset && pLenght )
      {
         *pOffset = (INT64)blockStart * DMS_FILEHOLE_BLOCK_SZ ;
         *pLenght = (INT64)( blockEnd - blockStart ) * DMS_FILEHOLE_BLOCK_SZ ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK2, "_dmsHoleMapMgr::setHoleMapMask" )
   INT32 _dmsHoleMapMgr::setHoleMapMask( INT32 type,
                                         const ossPoolVector<_dmsSMESpaceNode> &freePages,
                                         ossPoolVector<_dmsFileSpaceNode> &offsetVec )
   {
      INT32 rc             = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK2 ) ;
      UINT32 blockStart    = 0 ;
      UINT32 blockEnd      = 0 ;
      INT64 offset         = 0 ;
      INT64 length         = 0 ;
      INT64 tmpLen         = 0 ;
      UINT32 holeID        = 0 ;
      UINT32 pageSize      = 0 ;
      BOOLEAN hadSetMask   = FALSE ;
      dmsHME *pHME         = NULL ;

      rc = _getHME( type, &pHME ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get HME, rc: %d", rc ) ;
         goto error ;
      }
      pageSize = _getSuPageSize( type ) ;

      for ( UINT32 idx = 0 ; idx < freePages.size() ; idx++ )
      {
         // set the fileHole mask, make sure all the page in hole blocks
         // need to punch_hole, otherwise do not punch hole
         //
         //              hole blocks
         // --|----------|----------|----------|----
         //
         //       start               end
         //         |------------------|
         //
         // adjust to the boundary
         //           start         end
         //              |----------|
         offset = (INT64)( freePages[idx].start ) * pageSize ;
         length = (INT64)( freePages[idx].length ) * pageSize ;
         blockStart = offset / DMS_FILEHOLE_BLOCK_SZ ;
         tmpLen = offset % DMS_FILEHOLE_BLOCK_SZ ;
         if ( tmpLen )
         {
            length -= ( DMS_FILEHOLE_BLOCK_SZ - tmpLen ) ;
            if ( length < 0 )
            {
               length = 0 ;
            }
            blockStart += 1 ;
         }

         blockEnd = blockStart + ( length / DMS_FILEHOLE_BLOCK_SZ ) ;

         for ( holeID = blockStart ; holeID < blockEnd ; holeID++ )
         {
            if ( !pHME->testBit( holeID ) )
            {
               hadSetMask = TRUE ;
               pHME->setBit( holeID ) ;
            }
         }

         offset = (INT64)blockStart * DMS_FILEHOLE_BLOCK_SZ ;
         length = (INT64)( blockEnd - blockStart ) * DMS_FILEHOLE_BLOCK_SZ ;
         try
         {
            offsetVec.push_back( _dmsFileSpaceNode( offset, length ) ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }
      if ( hadSetMask )
      {
         flushHME( TRUE ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSHOLEMAPMGR_SETHOLEMAPMASK2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}