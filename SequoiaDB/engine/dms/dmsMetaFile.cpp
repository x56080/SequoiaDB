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

   Source File Name = dmsMetaFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2024  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsMetaFile.hpp"
#include "utilStr.hpp"
#include "pmdEnv.hpp"

#include "../bson/lib/md5.hpp"
#include "../bson/lib/md5.h"

using namespace std ;

namespace engine
{

   #define DMS_META_DATA_MAX_SZ                 ( 4*1024*1024 + DMS_METAFILE_HEADER_LEN )    // 4MB

   /*
      _dmsMetaFile implement
   */
   _dmsMetaFile::_dmsMetaFile( ossAtomic64 *pTotalCacheMem )
   : _cachedIndexCLNum( 0 )
   {
      _invalidateStatus = FALSE ;
      _errorRC = SDB_OK ;
      ossMemset( _szFileName, 0, sizeof( _szFileName ) ) ;
      _pTotalCacheMem = pTotalCacheMem ;
   }

   _dmsMetaFile::~_dmsMetaFile()
   {
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }

      if ( !_cachedIndexCLNum.compare( 0 ) )
      {
         invalidateAllIndexCache() ;
      }
   }

   void _dmsMetaFile::lock()
   {
      _mtx.get() ;
   }

   void _dmsMetaFile::unlock()
   {
      _mtx.release() ;
   }

   INT32 _dmsMetaFile::init( const CHAR *parentDir, const CHAR *csName )
   {
      INT32 rc = SDB_OK ;
      CHAR pathFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( NULL == parentDir || NULL == csName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( ossStrlen( csName ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// build file name
      ossSnprintf( _szFileName, DMS_META_FILE_NAMESIZE, "%s.1.%s", csName,
                   DMS_METAFILE_NAME_POSIX ) ;
      /// build path name
      rc = utilBuildFullPath( parentDir, _szFileName, OSS_MAX_PATHSIZE, pathFile ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build meta path file(%s,%s) failed, rc: %d", parentDir, csName, rc ) ;
         goto error ;
      }

      try
      {
         _path = pathFile ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when set meta path file: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      /// when the file exist, restore
      if ( SDB_OK == ossAccess( _path.c_str() ) )
      {
         BOOLEAN isValid = FALSE ;
         rc = _restore( isValid ) ;
         if ( rc || !isValid )
         {
            rc = SDB_OK ;
            _invalidateStatus = TRUE ;
            /// need remove the file
            ossDelete( _path.c_str() ) ;
         }
         else
         {
            _invalidateStatus = FALSE ;

            CHAR strTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
            ossTimestamp tm ;

            tm.time = _header._updateTime / 1000 ;
            tm.microtm = ( _header._updateTime % 1000 ) * 1000 ;
            ossTimestampToString( tm, strTime ) ;

            PD_LOG( PDEVENT, "Load collectionspace(%s) meta file succeed("
                             "UpdateTime: %s, "
                             "CollectionNum: %u, "
                             "DataSMECount: %u, "
                             "IdxSMECount: %u, "
                             "LobSMECount: %u)",
                    csName, strTime, _setMBID.size(), _setDataSME.size(),
                    _setIdxSME.size(), _setLobSME.size() ) ;
         }
      }
      else
      {
         _invalidateStatus = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsMetaFile::rename( const CHAR *newCSName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pos = NULL ;
      string newPath ;

      ossScopedLock lock( &_mtx ) ;

      CHAR newFileName[ DMS_META_FILE_NAMESIZE + 1 ] = { 0 } ;
      CHAR pathFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( NULL == newCSName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( ossStrlen( newCSName ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// find the path
      pos = ossStrstr( _path.c_str(), _szFileName ) ;
      if ( !pos || ossStrlen( pos ) != ossStrlen( _szFileName ) )
      {
         PD_LOG( PDERROR, "File full path[%s] is not include su file[%s]",
                 _path.c_str(), _szFileName ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      ossStrncpy( pathFile, _path.c_str(), pos - _path.c_str() ) ;

      /// build file name
      ossSnprintf( newFileName, DMS_META_FILE_NAMESIZE, "%s.1.%s", newCSName,
                   DMS_METAFILE_NAME_POSIX ) ;

      rc = utilCatPath( pathFile, OSS_MAX_PATHSIZE, newFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build new full path name failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         newPath = pathFile ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when set meta path file: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      invalidate( FALSE, FALSE ) ;

      /// then set the new path
      ossStrcpy( _szFileName, newFileName ) ;
      _path = newPath ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsMetaFile::invalidate( BOOLEAN force, BOOLEAN needLock )
   {
      if ( !_invalidateStatus || SDB_OK != _errorRC || force )
      {
         ossScopedLock lock( &_mtx, needLock ) ;

         /// double check
         if ( !_invalidateStatus || SDB_OK != _errorRC || force )
         {
            /// remove the file
            ossDelete( _path.c_str() ) ;

            /// clear
            _clear() ;
            /// reset header
            _header.reset() ;
            _errorRC = SDB_OK ;
            _invalidateStatus = TRUE ;
         }
      }
   }

   void _dmsMetaFile::remove()
   {
      invalidateAllIndexCache() ;

      {
         ossScopedLock lock( &_mtx ) ;

         invalidate( FALSE, FALSE ) ;
         
         /// clear the path
         _path = "" ;
      }
   }

   BOOLEAN _dmsMetaFile::isValid() const
   {
      if ( _header._checkSum != 0 && !_invalidateStatus )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT16 _dmsMetaFile::beginMBID() const
   {
      if ( ! isValid() )
      {
         return 0 ;
      }
      else if ( _setMBID.empty() )
      {
         return DMS_INVALID_MBID ;
      }

      return (*(_setMBID.begin()))._mbID ;
   }

   UINT16 _dmsMetaFile::nextMBID( UINT16 pos ) const
   {
      dmsMBItem tmp( pos ) ;

      if ( pos >= DMS_MME_SLOTS )
      {
         return DMS_INVALID_MBID ;
      }
      else if ( ! isValid() )
      {
         return pos + 1 ;
      }

      ossPoolSet<dmsMBItem>::const_iterator cit = _setMBID.upper_bound( tmp ) ;
      if ( cit != _setMBID.end() )
      {
         return (*(cit))._mbID ;
      }
      return DMS_INVALID_MBID ;
   }

   BOOLEAN _dmsMetaFile::getMBItem( UINT16 mbID, dmsMBItem &mbItem ) const
   {
      dmsMBItem tmp( mbID ) ;
      ossPoolSet<dmsMBItem>::const_iterator cit = _setMBID.find( tmp ) ;
      if ( cit != _setMBID.end() )
      {
         mbItem = *cit ;
         return TRUE ;
      }
      return FALSE ;
   }

   UINT64 _dmsMetaFile::beginSMEItem( UINT32 fileType ) const
   {
      const SET_UINT64 *pSetSME = _getSMEByType( fileType ) ;

      if ( pSetSME->empty() )
      {
         return ~0 ;
      }
      return *( pSetSME->begin() ) ;
   }

   UINT64 _dmsMetaFile::nextSMEItem( UINT32 fileType, UINT64 pos ) const
   {
      const SET_UINT64 *pSetSME = _getSMEByType( fileType ) ;
      SET_UINT64::const_iterator cit = pSetSME->upper_bound( pos ) ;
      if ( cit != pSetSME->end() )
      {
         return *(cit) ;
      }
      return ~0 ;
   }

   const SET_UINT64* _dmsMetaFile::_getSMEByType( UINT32 fileType ) const
   {
      if ( DMS_FILE_DATA == fileType )
      {
         return &_setDataSME ;
      }
      else if ( DMS_FILE_IDX == fileType )
      {
         return &_setIdxSME ;
      }
      else
      {
         SDB_ASSERT( DMS_FILE_LOB == fileType, "File type invalid" ) ;
         return &_setLobSME ;
      }
   }

   INT32 _dmsMetaFile::addMBItem( const dmsMBItem &mbItem )
   {
      INT32 rc = SDB_OK ;

      if ( _errorRC )
      {
         rc = _errorRC ;
         goto error ;
      }
      else if ( !_invalidateStatus )
      {
         SDB_ASSERT( FALSE, "Should make it invalid first" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( mbItem._mbID >= DMS_MME_SLOTS )
      {
         SDB_ASSERT( FALSE, "Invalid mbID" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         if ( ! _setMBID.insert( mbItem ).second )
         {
            /// already exist
            rc = SDB_INVALIDARG ;
            SDB_ASSERT( FALSE, "already exist" ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception when add mbID: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( SDB_OK == _errorRC )
      {
         _errorRC = rc ;
      }
      goto done ;
   }

   INT32 _dmsMetaFile::addSMEItem( UINT32 fileType, UINT64 smeItem )
   {
      INT32 rc = SDB_OK ;
      SET_UINT64 *pSetSME = (SET_UINT64*)_getSMEByType( fileType ) ;

      if ( _errorRC )
      {
         rc = _errorRC ;
         goto error ;
      }
      else if ( !_invalidateStatus )
      {
         SDB_ASSERT( FALSE, "Should make it invalid first" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         if ( ! pSetSME->insert( smeItem ).second )
         {
            /// already exist
            rc = SDB_INVALIDARG ;
            SDB_ASSERT( FALSE, "already exist" ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception when add sme item: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( SDB_OK == _errorRC )
      {
         _errorRC = rc ;
      }
      goto done ;
   }

   BOOLEAN _dmsMetaFile::checkSize() const
   {
      UINT32 buffSize = 0 ;

      buffSize = sizeof( _header ) +
                 _setMBID.size() * sizeof( dmsMBItem ) +
                 _setDataSME.size() * sizeof( UINT64 ) +
                 _setIdxSME.size() * sizeof( UINT64 ) +
                 _setLobSME.size() * sizeof( UINT64 ) +
                 sizeof( UINT32 ) ;

      if ( buffSize > DMS_META_DATA_MAX_SZ )
      {
         return FALSE ;
      }
      return TRUE ;      
   }

   UINT64 _dmsMetaFile::getOldestAccessTick() const
   {
      UINT64 accessTick = 0 ;
      UINT64 itemLastAccessTick = 0 ;

      if ( 0 == _cachedIndexCLNum.peek() )
      {
         goto done ;
      }

      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         const dmsCLIndexCache &item = _arrayIndexCache[ i ] ;
         if ( item._isCached )
         {
            itemLastAccessTick = item._lastAccessTick ;
            if ( 0 == accessTick || accessTick > itemLastAccessTick )
            {
               accessTick = itemLastAccessTick ;
            }
         }
      }

   done:
      return accessTick ;
   }

   void _dmsMetaFile::invalidateAllIndexCache()
   {
      invalidateOutOfDataCache( 0 ) ;
   }

   void _dmsMetaFile::invalidateOutOfDataCache( UINT64 expiredMs )
   {
      UINT32 indexCnt = 0 ;
      UINT32 memSize = 0 ;

      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         dmsCLIndexCache &item = _arrayIndexCache[ i ] ;

         ossScopedLock lock( &item._latch, EXCLUSIVE ) ;

         if ( item._isCached )
         {
            if ( expiredMs > 0 && pmdGetTickSpanTime( item._lastAccessTick ) < expiredMs )
            {
               /// not invalidate
               continue ;
            }

            item._isCached = FALSE ;
            _cachedIndexCLNum.dec() ;

            indexCnt += item._vecIndex.size() ;
            memSize += item._memSize ;
         }
         if ( item._memSize > 0 && _pTotalCacheMem )
         {
            _pTotalCacheMem->sub( item._memSize ) ;
         }

         item._vecIndex.clear() ;
         item._memSize = 0 ;
      }

      if ( ( indexCnt > 0 || memSize > 0 ) && !pmdIsQuitApp() )
      {
         if ( _pTotalCacheMem )
         {
            PD_LOG( PDEVENT, "Clear %s index caches(%u, MemSize: %u) for file(%s) "
                    "succeed. Total index cache mem size: %lld",
                    ( 0 == expiredMs ? "all" : "out-of-date" ),
                    indexCnt, memSize, _szFileName, _pTotalCacheMem->fetch() ) ;
         }
         else
         {
            PD_LOG( PDEVENT, "Clear %s index caches(%u, MemSize: %u) for file(%s) "
                    "succeed",
                    ( 0 == expiredMs ? "all" : "out-of-date" ),
                    indexCnt, memSize, _szFileName ) ;
         }
      }
   }

   void _dmsMetaFile::invalidateIndexCache( UINT16 mbID, const CHAR *csName, const CHAR *clName )
   {
      BOOLEAN hasInvalidate = FALSE ;
      UINT32 indexCnt = 0 ;
      UINT32 memSize = 0 ;

      if ( mbID < DMS_MME_SLOTS )
      {
         dmsCLIndexCache &item = _arrayIndexCache[ mbID ] ;

         ossScopedLock lock( &item._latch, EXCLUSIVE ) ;
         if ( item._isCached )
         {
            item._isCached = FALSE ;
            _cachedIndexCLNum.dec() ;
            hasInvalidate = TRUE ;
         }
         if ( item._memSize > 0 && _pTotalCacheMem )
         {
            _pTotalCacheMem->sub( item._memSize ) ;
         }

         indexCnt = item._vecIndex.size() ;
         memSize = item._memSize ;
         item._vecIndex.clear() ;
         item._memSize = 0 ;
      }

      if ( hasInvalidate && csName && clName && *csName && *clName )
      {
         if ( _pTotalCacheMem )
         {
            PD_LOG( PDINFO, "Clear index cache(%u, MemSize: %u) for collection(%s.%s, MBID:%u) "
                    "succeed. Total index cache mem size: %lld",
                    indexCnt, memSize, csName, clName, mbID, _pTotalCacheMem->fetch() ) ;
         }
         else
         {
            PD_LOG( PDINFO, "Clear index cache(%u, MemSize: %u) for collection(%s.%s, MBID:%u) "
                    "succeed", indexCnt, memSize, csName, clName, mbID ) ;
         }
      }
   }

   INT32 _dmsMetaFile::getIndexCache( UINT16 mbID,
                                      MON_IDX_LIST &indexes,
                                      BOOLEAN &isCacheValid )
   {
      INT32 rc = SDB_OK ;

      if ( mbID >= DMS_MME_SLOTS )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         dmsCLIndexCache &item = _arrayIndexCache[ mbID ] ;

         ossScopedLock lock( &item._latch, SHARED ) ;
         isCacheValid = item._isCached ;

         if ( isCacheValid )
         {
            /// update access tick
            item._lastAccessTick = pmdGetDBTick() ;

            try
            {
               for ( UINT32 i = 0 ; i < item._vecIndex.size() ; ++i )
               {
                  indexes.push_back( item._vecIndex[i] ) ;
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsMetaFile::getIndexCache( UINT16 mbID,
                                      const CHAR *pIndexName,
                                      monIndex &index,
                                      BOOLEAN &isCacheValid )
   {
      INT32 rc = SDB_OK ;

      if ( mbID >= DMS_MME_SLOTS )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         dmsCLIndexCache &item = _arrayIndexCache[ mbID ] ;

         ossScopedLock lock( &item._latch, SHARED ) ;
         isCacheValid = item._isCached ;

         if ( isCacheValid )
         {
            BOOLEAN found = FALSE ;

            /// update access tick
            item._lastAccessTick = pmdGetDBTick() ;

            try
            {
               for ( UINT32 i = 0 ; i < item._vecIndex.size() ; ++i )
               {
                  monIndex &indexItem = item._vecIndex[ i ] ;

                  if ( 0 == ossStrcmp( indexItem.getIndexName(), pIndexName ) )
                  {
                     /// found
                     index = indexItem ;
                     found = TRUE ;
                     rc = SDB_OK ;
                     break ;
                  }
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }

            if ( !found )
            {
               rc = SDB_IXM_NOTEXIST ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsMetaFile::pushIndexCache( UINT16 mbID,
                                       const MON_IDX_LIST &indexes,
                                       const CHAR *csName,
                                       const CHAR *clName )
   {
      INT32 rc = SDB_OK ;
      UINT32 memSize = 0 ;

      /// alloc slot
      rc = _arrayIndexCache.allocSlot( mbID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc slot(%u) failed, rc: %d", mbID, rc ) ;
         goto error ;
      }
      else
      {
         dmsCLIndexCache &item = _arrayIndexCache[ mbID ] ;
         BOOLEAN incNum = FALSE ;

         ossScopedLock lock( &item._latch, EXCLUSIVE ) ;

         if ( !item._isCached )
         {
            incNum = TRUE ;
         }
         if ( item._memSize > 0 && _pTotalCacheMem )
         {
            _pTotalCacheMem->sub( item._memSize ) ;
         }

         /// first clear
         item._isCached = FALSE ;
         item._vecIndex.clear() ;
         item._memSize = 0 ;

         /// then push indexes
         try
         {
            for ( UINT32 i = 0 ; i < indexes.size() ; ++i )
            {
               const monIndex &indexItem = indexes[ i ] ;
               item._vecIndex.push_back( indexItem ) ;
               item._memSize += ( sizeof( monIndex ) + indexItem._indexDef.objsize() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            item._vecIndex.clear() ;
            item._memSize = 0 ;
            goto error ;
         }

         /// set cached
         item._isCached = TRUE ;
         item._lastAccessTick = pmdGetDBTick() ;
         memSize = item._memSize ;

         if ( item._memSize > 0 && _pTotalCacheMem )
         {
            _pTotalCacheMem->add( item._memSize ) ;
         }
         if ( incNum )
         {
            _cachedIndexCLNum.inc() ;
         }
      }

      if ( csName && clName && *csName && *clName )
      {
         if ( _pTotalCacheMem )
         {
            PD_LOG( PDINFO, "Cached indexes(%u, MemSize: %u) for collection(%s.%s, MBID:%u) "
                    "succeed. Total index cache mem size: %lld",
                    indexes.size(), memSize, csName, clName, mbID, _pTotalCacheMem->fetch() ) ;
         }
         else
         {
            PD_LOG( PDINFO, "Cached indexes(%u, MemSize: %u) for collection(%s.%s, MBID:%u) "
                    "succeed", indexes.size(), memSize, csName, clName, mbID ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsMetaFile::writeDone( const CHAR *csName )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      UINT32 buffSize = 0 ;
      INT64 written = 0 ;

      if ( SDB_OK != _errorRC )
      {
         SDB_ASSERT( FALSE, "Has some error, can't write meta file" ) ;
         rc = _errorRC ;
         goto error ;
      }
      else if ( !_invalidateStatus || _path.empty() )
      {
         /// nothing need write
         goto done ;
      }

      /// calc
      _header._checkSum = 0 ;
      _header._updateTime = ossGetCurrentMilliseconds() ;
      _header._collectionNum = _setMBID.size() ;
      _header._dataSMEItemNum = _setDataSME.size() ;
      _header._idxSMEItemNum = _setIdxSME.size() ;
      _header._lobSMEItemNum = _setLobSME.size() ;

      buffSize = sizeof( _header ) +
                 _header._collectionNum * sizeof( dmsMBItem ) +
                 _header._dataSMEItemNum * sizeof( UINT64 ) +
                 _header._idxSMEItemNum * sizeof( UINT64 ) +
                 _header._lobSMEItemNum * sizeof( UINT64 ) +
                 sizeof( UINT32 ) ;

      if ( buffSize > DMS_META_DATA_MAX_SZ )
      {
         /// not write meta
         goto done ;
      }

      /// alloc buffer
      pBuff = ( CHAR* )SDB_OSS_MALLOC( buffSize ) ;
      if ( !pBuff )
      {
         PD_LOG( PDERROR, "Allocate memory(%llu) failed", buffSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      /// fill data
      rc = _fillData( pBuff, buffSize ) ;
      if ( rc )
      {
         goto error ;
      }

      /// create file
      rc = ossOpen( _path.c_str(), OSS_CREATEONLY | OSS_READWRITE,
                    OSS_DEFAULTFILE, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create meta file(%s), rc: %d",
                   _path.c_str(), rc ) ;

      /// write data
      rc = ossSeekAndWriteN( &_file, 0, pBuff, buffSize, written ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write data to meta file(%s), rc: %d",
                   _path.c_str(), rc ) ;
      /// check write size
      if ( written != (INT64)buffSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to write data to meta file(%s), rc: %d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      //sync the file
      rc = ossFsync( &_file ) ;
      PD_RC_CHECK( rc, PDERROR, "Fsync failed file(%s), rc: %d", _path.c_str(), rc ) ;

      /// change status
      _invalidateStatus = FALSE ;

      if ( csName && *csName )
      {
         PD_LOG( PDEVENT, "Save collectionspace(%s) meta file succeed("
                          "CollectionNum: %u, "
                          "DataSMECount: %u, "
                          "IdxSMECount: %u, "
                          "LobSMECount: %u)",
                 csName, _header._collectionNum, _header._dataSMEItemNum,
                 _header._idxSMEItemNum, _header._lobSMEItemNum ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Save collectionspace meta file(%s) succeed("
                          "CollectionNum: %u, "
                          "DataSMECount: %u, "
                          "IdxSMECount: %u, "
                          "LobSMECount: %u)",
                 _path.c_str(), _header._collectionNum, _header._dataSMEItemNum,
                 _header._idxSMEItemNum, _header._lobSMEItemNum ) ;
      }

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
      return rc ;
   error:
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
         ossDelete( _path.c_str() ) ;
      }
      goto done ;
   }

   INT32 _dmsMetaFile::_restore( BOOLEAN &isValid )
   {
      INT32 rc = SDB_OK ;
      SINT64 read = 0 ;
      CHAR *pBuff = NULL ;
      INT64 buffSize = 0 ;
      INT64 fileSize = 0 ;

      isValid = FALSE ;

      rc = ossOpen( _path.c_str(), OSS_READWRITE, OSS_RWXU, _file ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open meta file(%s), rc:%d",
                  _path.c_str(), rc ) ;

      /// get file size
      rc = ossGetFileSize( &_file, &fileSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get meta file(%s) size failed, rc: %d",
                 _path.c_str(), rc ) ;
         goto error ;
      }

      if ( fileSize > DMS_META_DATA_MAX_SZ )
      {
         /// file size overflow
         rc = SDB_SYS ;
         goto error ;
      }

      pBuff = ( CHAR* )SDB_OSS_MALLOC( fileSize ) ;
      if ( !pBuff )
      {
         PD_LOG( PDERROR, "Allocate memory(%llu) failed", fileSize ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = ossReadN( &_file, fileSize, pBuff, read ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read meta file(%s) data, rc: %d",
                  _path.c_str(), rc ) ;

      // read size must equal file size.
      PD_CHECK( read == fileSize, SDB_SYS, error, PDERROR,
                "Failed to read meta file(%s) data, rc: %d",
                _path.c_str(), SDB_SYS ) ;

      ossMemcpy( (void*)&_header, pBuff, sizeof( _header ) ) ;

      if ( 0 != ossMemcmp( _header._eyeCatcher, DMS_METAFILE_HEADER_EYECATCHER,
                           DMS_METAFILE_HEADER_EYECATCHER_LEN ) )
      {
         // header's magic number must be DMS_METAFILE_HEADER_EYECATCHER
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Meta header is invalid, rc: %d", rc ) ;
         goto error ;
      }

      if ( 0 == _header._checkSum )
      {
         /// invalid
         goto done ;
      }

      buffSize = sizeof( _header ) +
                 _header._collectionNum * sizeof( dmsMBItem ) +
                 _header._dataSMEItemNum * sizeof( UINT64 ) +
                 _header._idxSMEItemNum * sizeof( UINT64 ) +
                 _header._lobSMEItemNum * sizeof( UINT64 ) +
                 sizeof( UINT32 ) ;

      if ( fileSize != buffSize )
      {
         PD_LOG( PDERROR, "Meta file(%s) size(%lld) is not the same with %lld",
                 _path.c_str(), fileSize, buffSize ) ;
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      /// parse data
      rc = _parseData( pBuff, buffSize ) ;
      if ( rc )
      {
         goto error ;
      }

      isValid = TRUE ;

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      if ( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _dmsMetaFile::_clear()
   {
      _setMBID.clear() ;
      _setDataSME.clear() ;
      _setIdxSME.clear() ;
      _setLobSME.clear() ;
   }

   INT32 _dmsMetaFile::_parseData( const CHAR *pData, UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 offset = 0 ;
      dmsMetaFileHeader *pHeader = NULL ;
      dmsMBItem *pMBID = NULL ;
      UINT64 *pSME = NULL ;
      UINT32 *pCheckSum = NULL ;
      UINT32 checkSum = 0 ;

      /// header
      pHeader = ( dmsMetaFileHeader* )( pData + offset ) ;
      offset += sizeof( dmsMetaFileHeader ) ;

      try
      {
         pMBID = (dmsMBItem*)( pData + offset ) ;
         for ( UINT32 i = 0 ; i < _header._collectionNum ; ++i )
         {
            /// check size
            if ( offset >= buffSize )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            if ( pMBID[i]._mbID >= DMS_MME_SLOTS )
            {
               /// invalid mbID
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }

            if ( ! _setMBID.insert( pMBID[i] ).second )
            {
               /// already exist
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }

            offset += sizeof( dmsMBItem ) ;
         }

         pSME = (UINT64*)( pData + offset ) ;
         for ( UINT32 i = 0 ; i < _header._dataSMEItemNum ; ++i )
         {
            /// check size
            if ( offset >= buffSize )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            if ( ! _setDataSME.insert( pSME[i] ).second )
            {
               /// already exist
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }

            offset += sizeof( UINT64 ) ;
         }

         pSME = (UINT64*)( pData + offset ) ;
         for ( UINT32 i = 0 ; i < _header._idxSMEItemNum ; ++i )
         {
            /// check size
            if ( offset >= buffSize )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            if ( ! _setIdxSME.insert( pSME[i] ).second )
            {
               /// already exist
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }

            offset += sizeof( UINT64 ) ;
         }

         pSME = (UINT64*)( pData + offset ) ;
         for ( UINT32 i = 0 ; i < _header._lobSMEItemNum ; ++i )
         {
            /// check size
            if ( offset >= buffSize )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            if ( ! _setLobSME.insert( pSME[i] ).second )
            {
               /// already exist
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }

            offset += sizeof( UINT64 ) ;
         }

         if ( offset + sizeof( UINT32 ) != buffSize )
         {
            SDB_ASSERT( FALSE, "Invalid buffSize" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pCheckSum = (UINT32*)( pData + offset ) ;

         /// reset checksum, and calc checksum
         pHeader->_checkSum = 0 ;
         checkSum = _calcCheckSum( pData, offset ) ;

         if ( *pCheckSum != _header._checkSum )
         {
            /// checksum is not the same
            rc = SDB_CORRUPTED_RECORD ;
            PD_LOG( PDWARNING, "The tail checksum(%u) is not the same with header(%u) in "
                    "meta file(%s)", *pCheckSum, _header._checkSum, _path.c_str() ) ;
            goto error ;
         }
         else if ( checkSum != _header._checkSum )
         {
            /// checksum is not invalid
            rc = SDB_CORRUPTED_RECORD ;
            PD_LOG( PDWARNING, "The header checksum(%u) is not the same with calc(%u) in "
                    "meta file(%s)", _header._checkSum, checkSum, _path.c_str() ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when parse meta file(%s) data: %s",
                 _path.c_str(), e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _clear() ;
      goto done ;
   }

   INT32 _dmsMetaFile::_fillData( CHAR *pData, UINT32 buffSize )
   {
      INT32 rc = SDB_OK ;
      dmsMetaFileHeader *pHeader = NULL ;
      dmsMBItem *pMBID = NULL ;
      UINT64 *pSME = NULL ;
      UINT32 *pCheckSum = NULL ;
      UINT32 offset = 0 ;
      UINT32 index = 0 ;

      SET_UINT64::iterator itSme ;

      /// header
      pHeader = ( dmsMetaFileHeader* )( pData + offset ) ;
      ossMemcpy( pHeader, (void*)&_header, sizeof( _header ) ) ;

      /// MBID
      offset += sizeof( _header ) ;
      pMBID = ( dmsMBItem* )( pData + offset ) ;

      index = 0 ;
      ossPoolSet<dmsMBItem>::iterator it = _setMBID.begin() ;
      while( it != _setMBID.end() )
      {
         /// check size
         if ( offset >= buffSize )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         pMBID[ index ] = *it ;
         ++it ;
         ++index ;
         offset += sizeof( dmsMBItem ) ;
      }

      /// data sme
      pSME = ( UINT64* )( pData + offset ) ;
      itSme = _setDataSME.begin() ;
      index = 0 ;
      while ( itSme != _setDataSME.end() )
      {
         /// check size
         if ( offset >= buffSize )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         pSME[ index ] = *itSme ;
         ++itSme ;
         ++index ;
         offset += sizeof( UINT64 ) ;
      }

      /// index sme
      pSME = ( UINT64* )( pData + offset ) ;
      itSme = _setIdxSME.begin() ;
      index = 0 ;
      while ( itSme != _setIdxSME.end() )
      {
         /// check size
         if ( offset >= buffSize )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         pSME[ index ] = *itSme ;
         ++itSme ;
         ++index ;
         offset += sizeof( UINT64 ) ;
      }

      /// lob sme
      pSME = ( UINT64* )( pData + offset ) ;
      itSme = _setLobSME.begin() ;
      index = 0 ;
      while ( itSme != _setLobSME.end() )
      {
         /// check size
         if ( offset >= buffSize )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         pSME[ index ] = *itSme ;
         ++itSme ;
         ++index ;
         offset += sizeof( UINT64 ) ;
      }

      if ( offset + sizeof( UINT32 ) != buffSize )
      {
         SDB_ASSERT( FALSE, "Invalid buffSize" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      pCheckSum = (UINT32*)( pData + offset ) ;

      /// reset checksum, and calc checksum
      pHeader->_checkSum = 0 ;
      *pCheckSum = _calcCheckSum( pData, offset ) ;
      pHeader->_checkSum = *pCheckSum ;
      _header._checkSum = *pCheckSum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 _dmsMetaFile::_calcCheckSum( const CHAR *pData, UINT32 len )
   {
      md5::md5digest digest ;
      md5::md5( pData, len, digest ) ;

      UINT32 hashValue = 0 ;
      UINT32 i = 0 ;
      while ( i++ < 4 )
      {
         hashValue |= ( (UINT32)digest[i] << ( 32 - 8 * i ) ) ;
      }

      return hashValue ;
   }

   /*
      Global function
   */
   ossAtomic64* dmsGetTotalIndexMemSize()
   {
      static ossAtomic64 s_totalIndexMemSize( 0 ) ;
      return &s_totalIndexMemSize ;
   }

}

