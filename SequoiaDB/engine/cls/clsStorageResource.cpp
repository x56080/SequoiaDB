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

   Source File Name = clsStorageResource.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsStorageResource.hpp"
#include "clsTrace.hpp"
#include "ossLatchGuard.hpp"
#include <exception>
namespace engine
{
   BOOLEAN findCSNameLower( const _clsStorageResource::MAP_NAME_CL::value_type &value,
                            const CHAR *csName )
   {
      return utilStringView( value.first.data(), value.first.find( '.' ) ) <
             utilStringView( csName );
   }

   BOOLEAN findCSNameUpper( const CHAR *csName,
                            const _clsStorageResource::MAP_NAME_CL::value_type &value )
   {
      return utilStringView( csName ) <
             utilStringView( value.first.data(), value.first.find( '.' ) );
   }

   BOOLEAN findCSUidLower( const _clsStorageResource::MAP_CLUID_CL::value_type &value,
                           utilCSUniqueID csUID )
   {
      return utilGetCSUniqueID( value.first ) < csUID;
   }

   BOOLEAN findCSUidUpper( utilCSUniqueID csUID,
                           const _clsStorageResource::MAP_CLUID_CL::value_type &value )
   {
      return csUID < utilGetCSUniqueID( value.first );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_INIT, "_clsStorageResource::init" )
   void _clsStorageResource::init( std::unique_ptr< clsStorageResourceAgent > &&agent )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_INIT );
      SDB_ASSERT( nullptr != agent, "can not be nullptr" );
      _agent = std::move( agent );
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_INIT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_FINI, "_clsStorageResource::fini" )
   void _clsStorageResource::fini()
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_FINI );
      _agent.reset( nullptr );
      _mapNameToCL.clear();
      _mapUidToCL.clear();
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_FINI );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE1, "_clsStorageResource::getCLMetaCache" )
   _clsStorageResource::result _clsStorageResource::getCLMetaCache( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE1 );
      if ( !clFullName || !( *clFullName ) )
      {
         SDB_ASSERT( FALSE, "collection full name can not be null" );
         return result();
      }
      clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, TRUE );
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE1 );
      return result( *clCachePtr );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2, "_clsStorageResource::getCLMetaCache" )
   _clsStorageResource::result _clsStorageResource::getCLMetaCache( utilCLUniqueID clUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2 );
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) )
      {
         SDB_ASSERT( FALSE, "collection unique id must be valid" );
         return result();
      }
      clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clUID, TRUE );
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2 );
      return result( *clCachePtr );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1, "_clsStorageResource::getCLIndexSet" )
   CONST_CLS_INDEX_INFO_SET_PTR _clsStorageResource::getCLIndexSet( const CHAR *clFullName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1 );
      if ( !clFullName || !( *clFullName ) )
      {
         SDB_ASSERT( FALSE, "collection full name can not be null" );
         return nullptr;
      }
      CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr = _getCLIndexInfoSet( clFullName, TRUE );
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1, rc );
      return indexSetPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2, "_clsStorageResource::getCLIndexSet" )
   CONST_CLS_INDEX_INFO_SET_PTR _clsStorageResource::getCLIndexSet( utilCLUniqueID clUID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2 );
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) )
      {
         SDB_ASSERT( FALSE, "collection unique id must be valid" );
         return nullptr;
      }
      CONST_CLS_INDEX_INFO_SET_PTR indexSetPtr = _getCLIndexInfoSet( clUID, TRUE );
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2, rc );
      return indexSetPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET1, "_clsStorageResource::_getCLIndexInfoSet" )
   CLS_INDEX_INFO_SET_PTR _clsStorageResource::_getCLIndexInfoSet( const CHAR *clFullName,
                                                                   BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET1 );
      SDB_ASSERT( clFullName && *clFullName, "can not be null" );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_NAME_CL::const_iterator it = _mapNameToCL.find( clFullName );
      if ( it == _mapNameToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         return it->second->_getIndexInfoSet();
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET1 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET2, "_clsStorageResource::_getCLIndexInfoSet" )
   CLS_INDEX_INFO_SET_PTR _clsStorageResource::_getCLIndexInfoSet( utilCLUniqueID clUID,
                                                                   BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET2 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "cl unique id must be valid" );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( clUID );
      if ( it == _mapUidToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         return it->second->_getIndexInfoSet();
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLINDEXINFOSET2 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE1, "_clsStorageResource::_getCLMetaCache" )
   clsCLMetaCachePtr _clsStorageResource::_getCLMetaCache( const CHAR *clFullName,
                                                           BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE1 );
      SDB_ASSERT( clFullName && *clFullName, "can not be null" );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_NAME_CL::const_iterator it = _mapNameToCL.find( clFullName );
      if ( it == _mapNameToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         return it->second;
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE1 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE2, "_clsStorageResource::_getCLMetaCache" )
   clsCLMetaCachePtr _clsStorageResource::_getCLMetaCache( utilCLUniqueID clUID,
                                                           BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE2 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "cl unique id must be valid" );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( clUID );
      if ( it == _mapUidToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         return it->second;
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLMETACACHE2 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT1, "_clsStorageResource::_getIndexStat" )
   CONST_CLS_INDEX_STAT_PTR _clsStorageResource::_getIndexStat( const CHAR *clFullName,
                                                                const CHAR *indexName,
                                                                BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT1 );
      SDB_ASSERT( clFullName && *clFullName && indexName && *indexName, "can not be null" );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_NAME_CL::const_iterator it = _mapNameToCL.find( clFullName );
      if ( it == _mapNameToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         if ( it->second->_getIndexInfoSet() )
         {
            CLS_INDEX_INFO_PTR infoPtr = it->second->_getIndexInfoSet()->get( indexName );
            if ( infoPtr )
            {
               return infoPtr->getStat();
            }
            else
            {
               return nullptr;
            }
         }
         else
         {
            return nullptr;
         }
      }

      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT1 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT2, "_clsStorageResource::_getIndexStat" )
   CONST_CLS_INDEX_STAT_PTR _clsStorageResource::_getIndexStat( utilCLUniqueID clUID,
                                                                utilIdxInnerID idxInnerID,
                                                                BOOLEAN needLockShared )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT2 );
      ossSLatchGuard guard( &_mapLatch, SHARED, FALSE );
      if ( needLockShared )
      {
         guard.lock();
      }
      MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( clUID );
      if ( it == _mapUidToCL.cend() )
      {
         return nullptr;
      }
      else
      {
         if ( it->second->_getIndexInfoSet() )
         {
            CLS_INDEX_INFO_PTR infoPtr = it->second->_getIndexInfoSet()->get( idxInnerID );
            if ( infoPtr )
            {
               return infoPtr->getStat();
            }
            else
            {
               return nullptr;
            }
         }
         else
         {
            return nullptr;
         }
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETINDEXSTAT2 );
      return nullptr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET1, "_clsStorageResource::getOrUpdateCLIndexSet" )
   INT32 _clsStorageResource::getOrUpdateCLIndexSet( IExecutor *executor,
                                                     const CHAR *clFullName,
                                                     CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET1 );
      indexSetPtr.reset();
      if ( nullptr == clFullName || nullptr == executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      indexSetPtr = _getCLIndexInfoSet( clFullName, TRUE );
      if ( !indexSetPtr )
      {
         ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
         indexSetPtr = _getCLIndexInfoSet( clFullName, TRUE );
         if ( !indexSetPtr )
         {
            rc = _updateIndexSet( executor, clFullName, FALSE, indexSetPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
               goto error;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET1, rc );
      return rc;
   error:
      indexSetPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET2, "_clsStorageResource::getOrUpdateCLIndexSet" )
   INT32 _clsStorageResource::getOrUpdateCLIndexSet( IExecutor *executor,
                                                     utilCLUniqueID clUID,
                                                     CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET2 );
      indexSetPtr.reset();
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || nullptr == executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      indexSetPtr = _getCLIndexInfoSet( clUID, TRUE );
      if ( !indexSetPtr )
      {
         ossScopedLock lockCL( _latches + _getLatchPos( clUID ) );
         indexSetPtr = _getCLIndexInfoSet( clUID, TRUE );
         if ( !indexSetPtr )
         {
            rc = _updateIndexSet( executor, clUID, FALSE, indexSetPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
               goto error;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET2, rc );
      return rc;
   error:
      indexSetPtr.reset();
      goto done;
   }

   UINT32 _clsStorageResource::_getLatchPos( const CHAR *name, UINT32 len )
   {
      static_assert( LATCH_COUNT, "must be a power of 2" );
      return ossHash( name, len ) & ( LATCH_COUNT - 1 );
   }
   UINT32 _clsStorageResource::_getLatchPos( utilCLUniqueID clUID )
   {
      static_assert( LATCH_COUNT, "must be a power of 2" );
      return ossHash( (const CHAR *)&clUID, UINT32( sizeof( clUID ) ) ) & ( LATCH_COUNT - 1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET1, "_clsStorageResource::_updateIndexSet" )
   INT32 _clsStorageResource::_updateIndexSet( IExecutor *executor,
                                               const CHAR *clFullName,
                                               BOOLEAN withStat,
                                               CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET1 );
      indexSetPtr.reset();
      ossPoolVector< BSONObj > indexes;
      clsCLMetaCachePtr tempPtr = nullptr;
      MAP_NAME_CL::const_iterator it;
      if ( nullptr == clFullName || nullptr == executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      SDB_ASSERT( _agent, "can not be nullptr" );
      rc = _agent->getCLMetaCache( executor, clFullName, FALSE, TRUE, withStat, tempPtr );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to update index set in collection[%s], rc:%d", clFullName, rc );
         goto error;
      }
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         indexSetPtr = tempPtr->_getIndexInfoSet();
         it = _mapNameToCL.find( tempPtr->_getCLFullName() );
         if ( it == _mapNameToCL.cend() )
         {
            _insert( tempPtr );
         }
         else
         {
            it->second->_indexInfoSetPtr = std::move( tempPtr->_indexInfoSetPtr );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET1, rc );
      return rc;
   error:
      indexSetPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET2, "_clsStorageResource::_updateIndexSet" )
   INT32 _clsStorageResource::_updateIndexSet( IExecutor *executor,
                                               utilCLUniqueID clUID,
                                               BOOLEAN withStat,
                                               CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET2 );
      indexSetPtr.reset();
      ossPoolVector< BSONObj > indexes;
      clsCLMetaCachePtr tempPtr = nullptr;
      MAP_NAME_CL::const_iterator it;
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      SDB_ASSERT( _agent, "can not be nullptr" );
      rc = _agent->getCLMetaCache( executor, clUID, FALSE, TRUE, withStat, tempPtr );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
         goto error;
      }
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         indexSetPtr = tempPtr->_getIndexInfoSet();
         it = _mapNameToCL.find( tempPtr->_getCLFullName() );
         if ( it == _mapNameToCL.cend() )
         {
            _insert( tempPtr );
         }
         else
         {
            it->second->_indexInfoSetPtr = std::move( tempPtr->_indexInfoSetPtr );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET2, rc );
      return rc;
   error:
      indexSetPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_INVALIDATESTORAGECACHE, "_clsStorageResource::invalidateStorageCache" )
   void _clsStorageResource::invalidateStorageCache()
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_INVALIDATESTORAGECACHE );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _mapNameToCL.clear();
      _mapUidToCL.clear();
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_INVALIDATESTORAGECACHE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE1, "_clsStorageResource::removeCLMetaCache" )
   void _clsStorageResource::removeCLMetaCache( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE1 );
      SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _remove( clFullName );
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE2, "_clsStorageResource::removeCLMetaCache" )
   void _clsStorageResource::removeCLMetaCache( utilCLUniqueID clUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE2 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "must be valid cl unique id" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _remove( clUID );
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1, "_clsStorageResource::removeCSMetaCache" )
   void _clsStorageResource::removeCSMetaCache( const CHAR *csName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1 );
      SDB_ASSERT( nullptr != csName, "can not be nullptr" );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_NAME_CL::iterator start =
            std::lower_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameLower );
         MAP_NAME_CL::iterator end =
            std::upper_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameUpper );
         for ( MAP_NAME_CL::iterator it = start; it != end; )
         {
            utilCLUniqueID clUID = it->second->_clUID;
            _mapUidToCL.erase( clUID );
            it = _mapNameToCL.erase( it );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }

      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2, "_clsStorageResource::removeCSMetaCache" )
   void _clsStorageResource::removeCSMetaCache( utilCSUniqueID csUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2 );
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "must be valid cs unique id" );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_CLUID_CL::iterator start =
            std::lower_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidLower );
         MAP_CLUID_CL::iterator end =
            std::upper_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidUpper );
         PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2 );
         for ( MAP_CLUID_CL::iterator it = start; it != end; )
         {
            utilStringView clFullName = it->second->_name;
            _mapNameToCL.erase( clFullName );
            it = _mapUidToCL.erase( it );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLSTAT1, "_clsStorageResource::getCLStat" )
   CONST_CLS_CL_STAT_PTR _clsStorageResource::getCLStat( const CHAR *clFullName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLSTAT1 );
      if ( !clFullName || !( *clFullName ) )
      {
         SDB_ASSERT( FALSE, "collection full name can not be null" );
         return nullptr;
      }
      clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, TRUE );
      if ( !clCachePtr )
      {
         return nullptr;
      }
      CONST_CLS_CL_STAT_PTR clStatPtr = clCachePtr->_getCLStat();
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLSTAT1, rc );
      return clStatPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLSTAT2, "_clsStorageResource::getCLStat" )
   CONST_CLS_CL_STAT_PTR _clsStorageResource::getCLStat( utilCLUniqueID clUID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLSTAT2 );
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) )
      {
         SDB_ASSERT( FALSE, "collection unique id must be valid" );
         return nullptr;
      }
      clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clUID, TRUE );
      if ( !clCachePtr )
      {
         return nullptr;
      }
      CONST_CLS_CL_STAT_PTR clStatPtr = clCachePtr->_getCLStat();

      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLSTAT2, rc );
      return clStatPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT1, "_clsStorageResource::getIndexStat" )
   CONST_CLS_INDEX_STAT_PTR _clsStorageResource::getIndexStat( const CHAR *clFullName,
                                                               const CHAR *indexName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT1 );
      if ( !clFullName || !( *clFullName ) || !indexName || !( *indexName ) )
      {
         SDB_ASSERT( FALSE, "collection full name and index name can not be null" );
         return nullptr;
      }
      CONST_CLS_INDEX_STAT_PTR indexStatPtr = _getIndexStat( clFullName, indexName, TRUE );
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT1, rc );
      return indexStatPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT2, "_clsStorageResource::getIndexStat" )
   CONST_CLS_INDEX_STAT_PTR _clsStorageResource::getIndexStat( utilCLUniqueID clUID,
                                                               utilIdxInnerID idxInnerID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT2 );
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !utilCheckIdxInnerID( idxInnerID ) )
      {
         SDB_ASSERT( FALSE, "collection full name and index name can not be null" );
         return nullptr;
      }
      CONST_CLS_INDEX_STAT_PTR indexStatPtr = _getIndexStat( clUID, idxInnerID, TRUE );
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETINDEXSTAT2, rc );
      return indexStatPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT1, "_clsStorageResource::getOrUpdateCLStat" )
   INT32 _clsStorageResource::getOrUpdateCLStat( IExecutor *executor,
                                                 const CHAR *clFullName,
                                                 CONST_CLS_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT1 );
      clStatPtr.reset();
      MAP_NAME_CL::iterator it;
      if ( !clFullName || !( *clFullName ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, TRUE );
         if ( !clCachePtr )
         {
            ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
            clCachePtr = _getCLMetaCache( clFullName, TRUE );
            if ( !clCachePtr )
            {
               rc = _agent->getCLMetaCache( executor, clFullName, TRUE, FALSE, FALSE, clCachePtr );
               PD_RC_CHECK( rc, PDERROR, "failed to get collection meta cache, rc:%d", rc );
               clStatPtr = clCachePtr->_getCLStat();
               ossScopedLock lock( &_mapLatch, EXCLUSIVE );
               it = _mapNameToCL.find( clFullName );
               if ( it == _mapNameToCL.cend() )
               {
                  _insert( clCachePtr );
               }
               else
               {
                  it->second->_clStatPtr = std::move( clCachePtr->_clStatPtr );
               }
            }
            else
            {
               clStatPtr = clCachePtr->_getCLStat();
            }
         }
         else
         {
            clStatPtr = clCachePtr->_getCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT1, rc );
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT2, "_clsStorageResource::getOrUpdateCLStat" )
   INT32 _clsStorageResource::getOrUpdateCLStat( IExecutor *executor,
                                                 utilCLUniqueID clUID,
                                                 CONST_CLS_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT2 );
      clStatPtr = CLS_DEFAULT_CL_STAT;
      clsCLMetaCachePtr clCachePtr = nullptr;
      MAP_CLUID_CL::iterator it;
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clUID, TRUE );
         if ( !clCachePtr )
         {
            ossScopedLock lockCL( _latches + _getLatchPos( clUID ) );
            clCachePtr = _getCLMetaCache( clUID, TRUE );
            if ( !clCachePtr )
            {
               rc = _agent->getCLMetaCache( executor, clUID, TRUE, FALSE, FALSE, clCachePtr );
               PD_RC_CHECK( rc, PDERROR, "failed to get collection meta cache, rc:%d", rc );
               clStatPtr = clCachePtr->_getCLStat();
               ossScopedLock lock( &_mapLatch, EXCLUSIVE );
               it = _mapUidToCL.find( clUID );
               if ( it == _mapUidToCL.cend() )
               {
                  _insert( clCachePtr );
               }
               else
               {
                  it->second->_clStatPtr = std::move( clCachePtr->_clStatPtr );
               }
            }
            else
            {
               clStatPtr = clCachePtr->_getCLStat();
            }
         }
         else
         {
            clStatPtr = clCachePtr->_getCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLSTAT2, rc );
      return rc;
   error:
      clStatPtr = CLS_DEFAULT_CL_STAT;
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS1, "_clsStorageResource::getOrUpdateCLIndexStats" )
   INT32 _clsStorageResource::getOrUpdateCLIndexStats( IExecutor *executor,
                                                       const CHAR *clFullName,
                                                       CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS1 );
      indexSetPtr.reset();
      if ( nullptr == clFullName || nullptr == executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      indexSetPtr = _getCLIndexInfoSet( clFullName, TRUE );
      if ( !indexSetPtr )
      {
         ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
         indexSetPtr = _getCLIndexInfoSet( clFullName, TRUE );
         if ( !indexSetPtr )
         {
            rc = _updateIndexSet( executor, clFullName, TRUE, indexSetPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
               goto error;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS1, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS2, "_clsStorageResource::getOrUpdateCLIndexStats" )
   INT32 _clsStorageResource::getOrUpdateCLIndexStats( IExecutor *executor,
                                                       utilCLUniqueID clUID,
                                                       CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS2 );
      indexSetPtr.reset();
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || nullptr == executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      indexSetPtr = _getCLIndexInfoSet( clUID, TRUE );
      if ( !indexSetPtr )
      {
         ossScopedLock lockCL( _latches + _getLatchPos( clUID ) );
         indexSetPtr = _getCLIndexInfoSet( clUID, TRUE );
         if ( !indexSetPtr )
         {
            rc = _updateIndexSet( executor, clUID, TRUE, indexSetPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
               goto error;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSTATS2, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS1, "_clsStorageResource::getOrUpdateIndexStat" )
   INT32 _clsStorageResource::getOrUpdateIndexStat( IExecutor *executor,
                                                    const CHAR *clFullName,
                                                    const CHAR *indexName,
                                                    CONST_CLS_INDEX_STAT_PTR &indexStatPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS1 );
      indexStatPtr.reset();
      MAP_NAME_CL::iterator it;
      if ( !clFullName || !( *clFullName ) || !indexName || !( *indexName ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         indexStatPtr = _getIndexStat( clFullName, indexName, TRUE );
         if ( !indexStatPtr )
         {
            ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
            indexStatPtr = _getIndexStat( clFullName, indexName, TRUE );
            if ( !indexStatPtr )
            {
               CLS_INDEX_INFO_PTR infoPtr = nullptr;
               rc = _agent->getIndexInfo( executor, clFullName, indexName, TRUE, infoPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to fetch index info[%s]", indexName );
               indexStatPtr = infoPtr->getStat();
               clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, TRUE );
               if ( !clCachePtr )
               {
                  rc = _agent->getCLMetaCache( executor, clFullName, FALSE, FALSE, FALSE,
                                               clCachePtr );
                  PD_RC_CHECK( rc, PDERROR, "failed to fetch cl[%s] meta cache", clFullName );
                  CLS_INDEX_INFO_SET_PTR infoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
                  PD_CHECK( infoSetPtr, SDB_OOM, error, PDWARNING,
                            "failed to allocate memory for index info set" );
                  infoSetPtr->getVec().push_back( std::move( infoPtr ) );
                  clCachePtr->_indexInfoSetPtr = std::move( infoSetPtr );
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  _insert( clCachePtr );
               }
               else
               {
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  CLS_INDEX_INFO_SET_PTR infoSetPtr = clCachePtr->_getIndexInfoSet();
                  if ( !infoSetPtr )
                  {
                     infoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
                     PD_CHECK( infoSetPtr, SDB_OOM, error, PDWARNING,
                               "failed to allocate memory for index info set" );
                     infoSetPtr->getVec().push_back( std::move( infoPtr ) );
                     clCachePtr->_indexInfoSetPtr = std::move( infoSetPtr );
                  }
                  else
                  {
                     CLS_INDEX_INFO_SET_PTR newInfoSetPtr =
                        makeSharedPtrFromPool< clsIndexInfoSet >( *infoSetPtr );
                     PD_CHECK( newInfoSetPtr, SDB_OOM, error, PDWARNING,
                               "failed to allocate memory for index info set" );
                     ossPoolVector< CLS_INDEX_INFO_PTR > &vec = newInfoSetPtr->getVec();
                     ossPoolVector< CLS_INDEX_INFO_PTR >::iterator found =
                        std::find_if( vec.begin(), vec.end(),
                                      [ &, indexName ]( const CLS_INDEX_INFO_PTR &ptr ) -> BOOLEAN {
                                         return utilStringView( ptr->getIndexName() ) ==
                                                utilStringView( indexName );
                                      } );
                     *( found ) = infoPtr;
                     clCachePtr->_indexInfoSetPtr = newInfoSetPtr;
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS1, rc );
      return rc;
   error:
      indexStatPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS2, "_clsStorageResource::getOrUpdateIndexStat" )
   INT32 _clsStorageResource::getOrUpdateIndexStat( IExecutor *executor,
                                                    utilCLUniqueID clUID,
                                                    utilIdxInnerID idxInnerID,
                                                    CONST_CLS_INDEX_STAT_PTR &indexStatPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS2 );
      indexStatPtr.reset();
      MAP_NAME_CL::iterator it;
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         indexStatPtr = _getIndexStat( clUID, idxInnerID, TRUE );
         if ( !indexStatPtr )
         {
            ossScopedLock lockCL( _latches + _getLatchPos( clUID ) );
            indexStatPtr = _getIndexStat( clUID, idxInnerID, TRUE );
            if ( !indexStatPtr )
            {
               CLS_INDEX_INFO_PTR infoPtr = nullptr;
               rc = _agent->getIndexInfo( executor, clUID, idxInnerID, TRUE, infoPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to fetch index info[%s]", idxInnerID );
               indexStatPtr = infoPtr->getStat();
               clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clUID, TRUE );
               if ( !clCachePtr )
               {
                  rc = _agent->getCLMetaCache( executor, clUID, FALSE, TRUE, FALSE, clCachePtr );
                  PD_RC_CHECK( rc, PDERROR, "failed to fetch cl[%s] meta cache", clUID );
                  CLS_INDEX_INFO_SET_PTR infoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
                  PD_CHECK( infoSetPtr, SDB_OOM, error, PDWARNING,
                            "failed to allocate memory for index info set" );
                  infoSetPtr->getVec().push_back( std::move( infoPtr ) );
                  clCachePtr->_indexInfoSetPtr = std::move( infoSetPtr );
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  _insert( clCachePtr );
               }
               else
               {
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  CLS_INDEX_INFO_SET_PTR infoSetPtr = clCachePtr->_getIndexInfoSet();
                  if ( !infoSetPtr )
                  {
                     infoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
                     PD_CHECK( infoSetPtr, SDB_OOM, error, PDWARNING,
                               "failed to allocate memory for index info set" );
                     infoSetPtr->getVec().push_back( std::move( infoPtr ) );
                     clCachePtr->_indexInfoSetPtr = std::move( infoSetPtr );
                  }
                  else
                  {
                     CLS_INDEX_INFO_SET_PTR newInfoSetPtr =
                        makeSharedPtrFromPool< clsIndexInfoSet >( *infoSetPtr );
                     PD_CHECK( newInfoSetPtr, SDB_OOM, error, PDWARNING,
                               "failed to allocate memory for index info set" );
                     ossPoolVector< CLS_INDEX_INFO_PTR > &vec = newInfoSetPtr->getVec();
                     ossPoolVector< CLS_INDEX_INFO_PTR >::iterator found = std::find_if(
                        vec.begin(), vec.end(),
                        [ &, idxInnerID ]( const CLS_INDEX_INFO_PTR &ptr ) -> BOOLEAN {
                           return ptr->getIdxInnerID() == idxInnerID;
                        } );
                     *( found ) = infoPtr;
                     clCachePtr->_indexInfoSetPtr = newInfoSetPtr;
                  }
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETORUPDATEINDEXSTATS2, rc );
      return rc;
   error:
      indexStatPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_UPSERTCLSTAT, "_clsStorageResource::upsertCLStat" )
   INT32 _clsStorageResource::upsertCLStat( IExecutor *executor,
                                            const CHAR *clFullName,
                                            utilCLUniqueID clUID,
                                            const BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_UPSERTCLSTAT );
      if ( !clFullName || !( *clFullName ) || !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         CLS_CL_STAT_PTR clStatPtr = nullptr;
         rc = clsCLStat::buildFromBson( obj, clStatPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build collection[%s] statistics", clFullName );
         ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, FALSE );
         if ( clCachePtr )
         {
            if ( clCachePtr->_clUID != clUID )
            {
               PD_LOG( PDERROR, "cl full name and unique id do not match" );
               rc = SDB_INVALIDARG;
            }
            clCachePtr->_clStatPtr = clStatPtr;
         }
         else
         {
            clCachePtr = makeSharedPtrFromPool< clsCLMetaCache >();
            PD_CHECK( clCachePtr, SDB_OOM, error, PDWARNING,
                      "failed to allocate memory for collection[%s] meta cache", clFullName );
            rc = clCachePtr->init( clFullName, clUID, clStatPtr, nullptr );
            PD_RC_CHECK( rc, PDERROR, "failed to initialize collection[%s] meta cache",
                         clFullName );
            _insert( clCachePtr );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_UPSERTCLSTAT, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_UPSERTINDEXSTAT, "_clsStorageResource::upsertIndexStat" )
   INT32 _clsStorageResource::upsertIndexStat( IExecutor *executor,
                                               const CHAR *clFullName,
                                               utilCLUniqueID clUID,
                                               const CHAR *indexName,
                                               const BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_UPSERTINDEXSTAT );
      if ( !clFullName || !( *clFullName ) || !indexName || !( *indexName ) ||
           !UTIL_IS_VALID_CLUNIQUEID( clUID ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         CLS_INDEX_STAT_PTR indexStatPtr = nullptr;
         rc = clsIndexStat::buildFromBson( obj, indexStatPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to build index[%s] statistics in collection[%s]",
                      indexName, clFullName );
         BOOLEAN needInsert = FALSE;
         ossScopedLock lockCL( _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
         clsCLMetaCachePtr clCachePtr = _getCLMetaCache( clFullName, TRUE );
         if ( !clCachePtr )
         {
            clCachePtr = makeSharedPtrFromPool< clsCLMetaCache >();
            PD_CHECK( clCachePtr, SDB_OOM, error, PDWARNING,
                      "failed to allocate memory for collection[%s] meta cache", clFullName );
            rc = clCachePtr->init( clFullName, clUID );
            PD_RC_CHECK( rc, PDERROR, "failed to initialize collection[%s] meta cache",
                         clFullName );
            needInsert = TRUE;
         }

         CLS_INDEX_INFO_SET_PTR newInfoSetPtr = nullptr;
         CLS_INDEX_INFO_SET_PTR infoSetPtr = clCachePtr->_getIndexInfoSet();
         if ( infoSetPtr )
         {
            newInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >( *infoSetPtr );
            PD_CHECK( newInfoSetPtr, SDB_OOM, error, PDWARNING,
                      "failed to allocate memory for index info set" );
            auto isEqualNameOrID = [ &,
                                     indexName ]( const CLS_INDEX_INFO_PTR &infoPtr ) -> BOOLEAN {
               return utilStringView( infoPtr->getIndexName() ) == utilStringView( indexName );
            };
            ossPoolVector< CLS_INDEX_INFO_PTR >::iterator found = std::find_if(
               newInfoSetPtr->getVec().begin(), newInfoSetPtr->getVec().end(), isEqualNameOrID );
            CLS_INDEX_INFO_PTR newInfoPtr = nullptr;
            if ( found != newInfoSetPtr->getVec().end() )
            {
               newInfoPtr = makeSharedPtrFromPool< clsIndexInfo >( **found );
               PD_CHECK( newInfoPtr, SDB_OOM, error, PDWARNING,
                         "failed to allocate memory for index info" );
            }
            else
            {
               rc = _agent->getIndexInfo( executor, clFullName, indexName, FALSE, newInfoPtr );
               PD_RC_CHECK( rc, PDERROR, "failed to get index[%s.%s] info", clFullName, indexName );
            }

            newInfoPtr->setStat( indexStatPtr );
            *found = newInfoPtr;
            ossScopedLock lock( &_mapLatch, EXCLUSIVE );
            clCachePtr->_indexInfoSetPtr = newInfoSetPtr;
            if ( needInsert )
            {
               _insert( clCachePtr );
            }
         }
         else
         {
            CLS_INDEX_INFO_PTR infoPtr = nullptr;
            newInfoSetPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
            PD_CHECK( newInfoSetPtr, SDB_OOM, error, PDWARNING,
                      "failed to allocate memory for index info" );
            rc = _agent->getIndexInfo( executor, clFullName, indexName, FALSE, infoPtr );
            PD_RC_CHECK( rc, PDERROR, "failed to fetch index info" );
            infoPtr->setStat( indexStatPtr );
            newInfoSetPtr->getVec().push_back( std::move( infoPtr ) );
            ossScopedLock lock( &_mapLatch, EXCLUSIVE );
            clCachePtr->_indexInfoSetPtr = newInfoSetPtr;
            if ( needInsert )
            {
               _insert( clCachePtr );
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_UPSERTINDEXSTAT, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT1, "_clsStorageResource::removeCLStat" )
   void _clsStorageResource::removeCLStat( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT1 );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_NAME_CL::iterator found = _mapNameToCL.find( clFullName );
         if ( found != _mapNameToCL.end() )
         {
            found->second->resetCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT2, "_clsStorageResource::removeCLStat" )
   void _clsStorageResource::removeCLStat( utilCLUniqueID clUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT1 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "cl unique id must be valid" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_CLUID_CL::iterator found = _mapUidToCL.find( clUID );
         if ( found != _mapUidToCL.end() )
         {
            found->second->resetCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLSTAT1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS1, "_clsStorageResource::removeCLStatInCS" )
   void _clsStorageResource::removeCLStatInCS( const CHAR *csName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS1 );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_NAME_CL::iterator start =
            std::lower_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameLower );
         MAP_NAME_CL::iterator end =
            std::upper_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameUpper );
         for ( MAP_NAME_CL::iterator it = start; it != end; )
         {
            it->second->resetCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS2, "_clsStorageResource::removeCLStatInCS" )
   void _clsStorageResource::removeCLStatInCS( utilCSUniqueID csUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS2 );
      SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csUID ), "cl unique id must be valid" );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_CLUID_CL::iterator start =
            std::lower_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidLower );
         MAP_CLUID_CL::iterator end =
            std::upper_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidUpper );
         for ( MAP_CLUID_CL::iterator it = start; it != end; )
         {
            it->second->resetCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLSTATINCS2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS1, "_clsStorageResource::removeCLIndexStats" )
   void _clsStorageResource::removeCLIndexStats( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS1 );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_NAME_CL::iterator found = _mapNameToCL.find( clFullName );
         if ( found != _mapNameToCL.end() )
         {
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = found->second->_indexInfoSetPtr->getVec();
            for ( ossPoolVector< CLS_INDEX_INFO_PTR >::iterator it = vec.begin(); it != vec.end();
                  ++it )
            {
               ( *it )->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS2, "_clsStorageResource::removeCLIndexStats" )
   void _clsStorageResource::removeCLIndexStats( utilCLUniqueID clUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS2 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "cl unique id must be valid" );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_CLUID_CL::iterator found = _mapUidToCL.find( clUID );
         if ( found != _mapUidToCL.end() )
         {
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = found->second->_indexInfoSetPtr->getVec();
            for ( ossPoolVector< CLS_INDEX_INFO_PTR >::iterator it = vec.begin(); it != vec.end();
                  ++it )
            {
               ( *it )->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLINDEXSTATS2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS1, "_clsStorageResource::removeCSIndexStats" )
   void _clsStorageResource::removeCSIndexStats( const CHAR *csName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS1 );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_NAME_CL::iterator start =
            std::lower_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameLower );
         MAP_NAME_CL::iterator end =
            std::upper_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameUpper );
         for ( MAP_NAME_CL::iterator it = start; it != end; )
         {
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = it->second->_indexInfoSetPtr->getVec();
            for ( ossPoolVector< CLS_INDEX_INFO_PTR >::iterator itInfo = vec.begin();
                  itInfo != vec.end(); ++itInfo )
            {
               ( *itInfo )->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS2, "_clsStorageResource::removeCSIndexStats" )
   void _clsStorageResource::removeCSIndexStats( utilCSUniqueID csUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS2 );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_CLUID_CL::iterator start =
            std::lower_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidLower );
         MAP_CLUID_CL::iterator end =
            std::upper_bound( _mapUidToCL.begin(), _mapUidToCL.end(), csUID, findCSUidUpper );
         for ( MAP_CLUID_CL::iterator it = start; it != end; )
         {
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = it->second->_indexInfoSetPtr->getVec();
            for ( ossPoolVector< CLS_INDEX_INFO_PTR >::iterator itInfo = vec.begin();
                  itInfo != vec.end(); ++itInfo )
            {
               ( *itInfo )->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSINDEXSTATS2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT1, "_clsStorageResource::removeIndexStat" )
   void _clsStorageResource::removeIndexStat( const CHAR *clFullName, const CHAR *indexName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT1 );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_NAME_CL::iterator found = _mapNameToCL.find( clFullName );
         if ( found != _mapNameToCL.end() )
         {
            CLS_INDEX_INFO_PTR infoPtr = found->second->_indexInfoSetPtr->get( indexName );
            if ( !infoPtr )
            {
               infoPtr->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT2, "_clsStorageResource::removeIndexStat" )
   void _clsStorageResource::removeIndexStat( utilCLUniqueID clUID, utilIdxInnerID idxInnerID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT2 );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         MAP_CLUID_CL::iterator found = _mapUidToCL.find( clUID );
         if ( found != _mapUidToCL.end() )
         {
            CLS_INDEX_INFO_PTR infoPtr = found->second->_indexInfoSetPtr->get( idxInnerID );
            if ( !infoPtr )
            {
               infoPtr->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVEINDEXSTAT2 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVEALLCLSTATS, "_clsStorageResource::removeAllCLStats" )
   void _clsStorageResource::removeAllCLStats()
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVEALLCLSTATS );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         for ( MAP_NAME_CL::iterator it = _mapNameToCL.begin(); it != _mapNameToCL.end(); ++it )
         {

            it->second->resetCLStat();
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVEALLCLSTATS );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVEALLINDEXSTATS, "_clsStorageResource::removeAllIndexStats" )
   void _clsStorageResource::removeAllIndexStats()
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVEALLINDEXSTATS );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {

         for ( MAP_NAME_CL::iterator it = _mapNameToCL.begin(); it != _mapNameToCL.end(); ++it )
         {
            ossPoolVector< CLS_INDEX_INFO_PTR > &vec = it->second->_indexInfoSetPtr->getVec();
            for ( ossPoolVector< CLS_INDEX_INFO_PTR >::iterator itInfo = vec.begin();
                  itInfo != vec.end(); ++itInfo )
            {
               ( *itInfo )->resetStat();
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVEALLINDEXSTATS );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__INSERT, "_clsStorageResource::_insert" )
   void _clsStorageResource::_insert( const clsCLMetaCachePtr &clCataSetPtr )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__INSERT );
      SDB_ASSERT( nullptr != clCataSetPtr, "can not be nullptr" );
      _mapNameToCL.emplace( clCataSetPtr->_name, clCataSetPtr );
      if ( UTIL_IS_VALID_CLUNIQUEID( clCataSetPtr->_clUID ) )
      {
         _mapUidToCL.emplace( clCataSetPtr->_clUID, clCataSetPtr );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__INSERT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__REMOVE1, "_clsStorageResource::_remove" )
   void _clsStorageResource::_remove( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__REMOVE1 );
      SDB_ASSERT( nullptr != clFullName, "can not be nullptr" );
      MAP_NAME_CL::const_iterator it = _mapNameToCL.find( clFullName );
      if ( it != _mapNameToCL.end() )
      {
         utilCLUniqueID clUID = it->second->_clUID;
         if ( UTIL_IS_VALID_CLUNIQUEID( clUID ) )
         {
            _mapUidToCL.erase( clUID );
         }
         _mapNameToCL.erase( it );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__REMOVE1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__REMOVE2, "_clsStorageResource::_remove" )
   void _clsStorageResource::_remove( utilCLUniqueID clUID )
   {
      PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__REMOVE2 );
      SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( clUID ), "must be valid cl unique id" );
      MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( clUID );
      if ( it != _mapUidToCL.end() )
      {
         utilStringView clFullName( it->second->_name );
         _mapNameToCL.erase( clFullName );
         _mapUidToCL.erase( it );
      }
      PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__REMOVE2 );
   }
} // namespace engine