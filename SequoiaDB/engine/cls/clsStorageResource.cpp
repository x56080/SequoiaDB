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
#include "clsIndexInfo.hpp"
#include "dmsEventHandler.hpp"
#include "ixm.hpp"
#include "msgDef.h"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "sdbInterface.hpp"
#include "utilStringView.hpp"
#include "utilUniqueID.hpp"
#include "clsTrace.hpp"
#include <memory>
#include <string>
namespace engine
{
// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_INIT, "_clsStorageResource::init" )
void _clsStorageResource::init(
   std::unique_ptr< clsStorageResourceAgent > &&agent )
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
INT32 _clsStorageResource::getCLMetaCache( const CHAR *clFullName, result &res )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE1 );
   res.reset();
   ossScopedLock lock( &_mapLatch, SHARED );
   clsIndexInfoSetPtr indexSetPtr = nullptr;
   if ( !clFullName || !( *clFullName ) )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   indexSetPtr = _getCLIndexSetWithoutLock( clFullName );
   if ( !indexSetPtr )
   {
      rc = SDB_CAT_NO_MATCH_CATALOG;
      PD_LOG( PDERROR, "failed to get index cache set, rc:%d", rc );
      goto error;
   }
   res = result( indexSetPtr );

done:
   PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE1, rc );
   return rc;
error:
   res.reset();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2, "_clsStorageResource::getCLMetaCache" )
INT32 _clsStorageResource::getCLMetaCache( utilCLUniqueID cluid, result &res )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2 );
   ossScopedLock lock( &_mapLatch, SHARED );
   clsIndexInfoSetPtr indexSetPtr;
   if ( !UTIL_IS_VALID_CLUNIQUEID( cluid ) )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   indexSetPtr = _getCLIndexSetWithoutLock( cluid );
   if ( !indexSetPtr )
   {
      rc = SDB_CAT_NO_MATCH_CATALOG;
      PD_LOG( PDERROR, "failed to get index cache set, rc:%d", rc );
      goto error;
   }
   res = result( indexSetPtr );

done:
   PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLMETACACHE2, rc );
   return rc;
error:
   res.reset();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1, "_clsStorageResource::getCLIndexSet" )
INT32 _clsStorageResource::getCLIndexSet( const CHAR *clFullName,
                                          clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1 );
   indexSetPtr.reset();
   ossScopedLock lock( &_mapLatch, SHARED );
   if ( !clFullName || !( *clFullName ) )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   indexSetPtr = _getCLIndexSetWithoutLock( clFullName );
   if ( !indexSetPtr )
   {
      rc = SDB_CAT_NO_MATCH_CATALOG;
      PD_LOG( PDERROR, "failed to get index cache set, rc:%d", rc );
      goto error;
   }

done:
   PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET1, rc );
   return rc;
error:
   indexSetPtr.reset();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2, "_clsStorageResource::getCLIndexSet" )
INT32 _clsStorageResource::getCLIndexSet( utilCLUniqueID cluid,
                                          clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2 );
   indexSetPtr.reset();
   ossScopedLock lock( &_mapLatch, SHARED );
   if ( !UTIL_IS_VALID_CLUNIQUEID( cluid ) )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   indexSetPtr = _getCLIndexSetWithoutLock( cluid );
   if ( !indexSetPtr )
   {
      rc = SDB_CAT_NO_MATCH_CATALOG;
      PD_LOG( PDERROR, "failed to get index cache set, rc:%d", rc );
      goto error;
   }

done:
   PD_TRACE_EXITRC( SDB__CLSSTORAGERESOURCE_GETCLINDEXSET2, rc );
   return rc;
error:
   indexSetPtr.reset();
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK1, "_clsStorageResource::_getCLIndexSetWithoutLock" )
clsIndexInfoSetPtr _clsStorageResource::_getCLIndexSetWithoutLock(
   const CHAR *clFullName )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK1 );
   SDB_ASSERT( clFullName && *clFullName, "can not be null" );
   MAP_NAME_CL::const_iterator it = _mapNameToCL.find( clFullName );
   if ( it == _mapNameToCL.cend() )
   {
      return nullptr;
   }
   else
   {
      return it->second->_getIndexSet();
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK2, "_clsStorageResource::_getCLIndexSetWithoutLock" )
clsIndexInfoSetPtr _clsStorageResource::_getCLIndexSetWithoutLock(
   utilCLUniqueID cluid )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK2 );
   SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( cluid ),
               "cl unique id must be valid" );
   MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( cluid );
   if ( it == _mapUidToCL.cend() )
   {
      return nullptr;
   }
   else
   {
      return it->second->_getIndexSet();
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__GETCLINDEXSETWITHOUTLOCK2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET1, "_clsStorageResource::getOrUpdateCLIndexSet" )
INT32 _clsStorageResource::getOrUpdateCLIndexSet(
   IExecutor *executor,
   const CHAR *clFullName,
   clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET1 );
   indexSetPtr.reset();
   if ( nullptr == clFullName || nullptr == executor )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   _mapLatch.get_shared();
   indexSetPtr = _getCLIndexSetWithoutLock( clFullName );
   _mapLatch.release_shared();
   if ( !indexSetPtr )
   {
      ossScopedLock lockCL(
         _latches + _getLatchPos( clFullName, ossStrlen( clFullName ) ) );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      indexSetPtr = _getCLIndexSetWithoutLock( clFullName );
      if ( !indexSetPtr )
      {
         rc = _updateIndexSet( executor, clFullName, indexSetPtr );
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
INT32 _clsStorageResource::getOrUpdateCLIndexSet(
   IExecutor *executor, utilCLUniqueID cluid, clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_GETORUPDATECLINDEXSET2 );
   indexSetPtr.reset();
   if ( !UTIL_IS_VALID_CLUNIQUEID( cluid ) || nullptr == executor )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   _mapLatch.get_shared();
   indexSetPtr = _getCLIndexSetWithoutLock( cluid );
   _mapLatch.release_shared();
   if ( !indexSetPtr )
   {
      ossScopedLock lockCL( _latches + _getLatchPos( cluid ) );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      indexSetPtr = _getCLIndexSetWithoutLock( cluid );
      if ( !indexSetPtr )
      {
         rc = _updateIndexSet( executor, cluid, indexSetPtr );
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
   return ossHash( name, len ) % LATCH_COUNT;
}
UINT32 _clsStorageResource::_getLatchPos( utilCLUniqueID cluid )
{
   return ossHash( (const CHAR *)&cluid, UINT32( sizeof( cluid ) ) ) %
          LATCH_COUNT;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET1, "_clsStorageResource::_updateIndexSet" )
INT32 _clsStorageResource::_updateIndexSet( IExecutor *executor,
                                            const CHAR *clFullName,
                                            clsIndexInfoSetPtr &indexSetPtr )
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
   rc = _agent->getCLMetaCache( executor, clFullName, tempPtr );
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
      goto error;
   }
   indexSetPtr = tempPtr->_getIndexSet();
   it = _mapNameToCL.find( tempPtr->_getCLFullName() );
   if ( it == _mapNameToCL.cend() )
   {
      _insert( tempPtr );
   }
   else
   {
      it->second->_indexSetPtr = std::move( tempPtr->_indexSetPtr );
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
                                            utilCLUniqueID cluid,
                                            clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__UPDATECLINDEXSET2 );
   indexSetPtr.reset();
   ossPoolVector< BSONObj > indexes;
   clsCLMetaCachePtr tempPtr = nullptr;
   MAP_NAME_CL::const_iterator it;
   if ( !UTIL_IS_VALID_CLUNIQUEID( cluid ) || nullptr == executor )
   {
      rc = SDB_INVALIDARG;
      goto error;
   }
   SDB_ASSERT( _agent, "can not be nullptr" );
   rc = _agent->getCLMetaCache( executor, cluid, tempPtr );
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to update index set from dms, rc:%d", rc );
      goto error;
   }
   indexSetPtr = tempPtr->_getIndexSet();
   it = _mapNameToCL.find( tempPtr->_getCLFullName() );
   if ( it == _mapNameToCL.cend() )
   {
      _insert( tempPtr );
   }
   else
   {
      it->second->_indexSetPtr = std::move( tempPtr->_indexSetPtr );
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
void _clsStorageResource::removeCLMetaCache( utilCLUniqueID cluid )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE2 );
   SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( cluid ),
               "must be valid cl unique id" );
   ossScopedLock lock( &_mapLatch, EXCLUSIVE );
   _remove( cluid );
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECLMETACACHE2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1, "_clsStorageResource::removeCSMetaCache" )
void _clsStorageResource::removeCSMetaCache( const CHAR *csName )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1 );
   SDB_ASSERT( nullptr != csName, "can not be nullptr" );
   ossScopedLock lock( &_mapLatch, EXCLUSIVE );
   for ( MAP_NAME_CL::const_iterator it = _mapNameToCL.cbegin();
         it != _mapNameToCL.cend(); )
   {
      utilStringView csNameSv( it->second->_name.data(),
                               it->second->_name.find( '.' ) );
      if ( csNameSv == csName )
      {
         utilCLUniqueID cluid = it->second->_cluid;
         if ( UTIL_IS_VALID_CLUNIQUEID( cluid ) )
         {
            _mapUidToCL.erase( cluid );
         }
         _mapNameToCL.erase( it );
      }
      ++it;
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2, "_clsStorageResource::removeCSMetaCache" )
void _clsStorageResource::removeCSMetaCache( utilCSUniqueID csuid )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2 );
   SDB_ASSERT( UTIL_IS_VALID_CSUNIQUEID( csuid ),
               "must be valid cs unique id" );
   ossScopedLock lock( &_mapLatch, EXCLUSIVE );
   for ( MAP_CLUID_CL::const_iterator it = _mapUidToCL.cbegin();
         it != _mapUidToCL.cend(); )
   {
      utilCSUniqueID itemCsUid = utilGetCSUniqueID( it->second->_cluid );
      if ( csuid == itemCsUid )
      {
         utilStringView clFullName( it->second->_name );
         _mapNameToCL.erase( clFullName );
         _mapUidToCL.erase( it );
      }
      ++it;
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE_REMOVECSMETACACHE2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__INSERT, "_clsStorageResource::_insert" )
void _clsStorageResource::_insert( const clsCLMetaCachePtr &clCataSetPtr )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__INSERT );
   SDB_ASSERT( nullptr != clCataSetPtr, "can not be nullptr" );
   _mapNameToCL.emplace( clCataSetPtr->_name, clCataSetPtr );
   if ( UTIL_IS_VALID_CLUNIQUEID( clCataSetPtr->_cluid ) )
   {
      _mapUidToCL.emplace( clCataSetPtr->_cluid, clCataSetPtr );
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
      utilCLUniqueID cluid = it->second->_cluid;
      if ( UTIL_IS_VALID_CLUNIQUEID( cluid ) )
      {
         _mapUidToCL.erase( cluid );
      }
      _mapNameToCL.erase( it );
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__REMOVE1 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSTORAGERESOURCE__REMOVE2, "_clsStorageResource::_remove" )
void _clsStorageResource::_remove( utilCLUniqueID cluid )
{
   PD_TRACE_ENTRY( SDB__CLSSTORAGERESOURCE__REMOVE2 );
   SDB_ASSERT( UTIL_IS_VALID_CLUNIQUEID( cluid ),
               "must be valid cl unique id" );
   MAP_CLUID_CL::const_iterator it = _mapUidToCL.find( cluid );
   if ( it != _mapUidToCL.end() )
   {
      utilStringView clFullName( it->second->_name );
      _mapNameToCL.erase( clFullName );
      _mapUidToCL.erase( it );
   }
   PD_TRACE_EXIT( SDB__CLSSTORAGERESOURCE__REMOVE2 );
}
} // namespace engine