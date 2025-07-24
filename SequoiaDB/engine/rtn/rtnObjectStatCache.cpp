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

   Source File Name = rtnObjectStatCache.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft
          11/24/2022  ZHY Move to rtn module
   Last Changed =

*******************************************************************************/
#include "rtnObjectStatCache.hpp"
#include "rtnTrace.hpp"
#include "ossLatchGuard.hpp"
#include "dmsStatSUMgr.hpp"
#include <exception>
namespace engine
{
   BOOLEAN findCSNameLower( const _rtnObjectStatCache::MAP_NAME_CL::value_type &value,
                            const CHAR *csName )
   {
      return utilStringView( value.first.data(), value.first.find( '.' ) ) <
             utilStringView( csName );
   }

   BOOLEAN findCSNameUpper( const CHAR *csName,
                            const _rtnObjectStatCache::MAP_NAME_CL::value_type &value )
   {
      return utilStringView( csName ) <
             utilStringView( value.first.data(), value.first.find( '.' ) );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_INIT, "_rtnObjectStatCache::init" )
   void _rtnObjectStatCache::init( std::unique_ptr< rtnObjectStatAgent > &&agent )
   {
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_INIT );
      SDB_ASSERT( nullptr != agent, "can not be nullptr" );
      _agent = std::move( agent );
      PD_TRACE_EXIT( SDB__RTNOBJECTSTATCACHE_INIT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_FINI, "_rtnObjectStatCache::fini" )
   void _rtnObjectStatCache::fini()
   {
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_FINI );
      _agent.reset( nullptr );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _mapNameToCL.clear();
      PD_TRACE_EXIT( SDB__RTNOBJECTSTATCACHE_FINI );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_GETCLSTAT1, "_rtnObjectStatCache::getCLStat" )
   CONST_RTN_CL_STAT_PTR _rtnObjectStatCache::getCLStat( const CHAR *clFullName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_GETCLSTAT1 );
      SDB_ASSERT( clFullName && *clFullName, "collection full name can not be null" );
      CONST_RTN_CL_STAT_PTR clStatPtr = _findWithSharedLock( clFullName );
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE_GETCLSTAT1, rc );
      return clStatPtr;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_GETORUPDATECLSTAT1, "_rtnObjectStatCache::getOrUpdateCLStat" )
   INT32 _rtnObjectStatCache::getOrUpdateCLStat( IExecutor *executor,
                                                 const CHAR *clFullName,
                                                 CONST_RTN_CL_STAT_PTR &clStatPtr )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_GETORUPDATECLSTAT1 );
      clStatPtr.reset();
      MAP_NAME_CL::iterator it;
      if ( !clFullName || !( *clFullName ) || !executor )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      try
      {
         clStatPtr = _findWithSharedLock( clFullName );
         if ( !clStatPtr )
         {
            RTN_CL_STAT_PTR tempPtr;
            rc = _agent->getCollectionStatInfo( executor, clFullName, TRUE, tempPtr );
            PD_RC_CHECK( rc, PDERROR, "failed to get collection[%s] statistics by agent, rc: %d",
                         clFullName, rc );
            clStatPtr = tempPtr;
            ossScopedLock lock( &_mapLatch, EXCLUSIVE );
            _insertOrAssign( clStatPtr );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE_GETORUPDATECLSTAT1, rc );
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE__RELOADCLSTATS, "_rtnObjectStatCache::reloadCLStats" )
   INT32 _rtnObjectStatCache::reloadCLStats( IExecutor *executor, const CHAR *clFullName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE__RELOADCLSTATS );
      removeCLStat( clFullName );
      RTN_CL_STAT_PTR clStatPtr = nullptr;
      if ( !executor || !clFullName || !( *clFullName ) )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _agent->getCollectionStatInfo( executor, clFullName, TRUE, clStatPtr );
      PD_RC_CHECK( rc, PDERROR, "failed to get collection[%s] meta cache rc: %d", clFullName, rc );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         _insertOrAssign( clStatPtr );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE__RELOADCLSTATS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE__RELOADCSSTATS, "_rtnObjectStatCache::reloadCSStats" )
   INT32 _rtnObjectStatCache::reloadCSStats( IExecutor *executor,
                                             const CHAR *csName,
                                             UINT32 batchSize )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE__RELOADCSSTATS );
      removeCLStatInCS( csName );

      if ( !executor || !csName || !( *csName ) || batchSize == 0 )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _reloadStatsByContext( executor, csName, batchSize );
      PD_RC_CHECK( rc, PDERROR, "failed to reload statistics on cs[%s] by cursor, rc: %d", csName,
                   rc );

   done:
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE__RELOADCSSTATS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE__RELOADALLSTATS, "_rtnObjectStatCache::reloadAllStats" )
   INT32 _rtnObjectStatCache::reloadAllStats( IExecutor *executor, UINT32 batchSize )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE__RELOADALLSTATS );
      removeAllStats();

      if ( !executor || batchSize == 0 )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _reloadStatsByContext( executor, nullptr, batchSize );
      PD_RC_CHECK( rc, PDERROR, "failed to reload statistics on node by cursor, rc: %d", rc );
   done:
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE__RELOADALLSTATS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE__RELOADSTATSBYCONTEXT, "_rtnObjectStatCache::_reloadStatsByContext" )
   INT32 _rtnObjectStatCache::_reloadStatsByContext( IExecutor *executor,
                                                     const CHAR *csName,
                                                     UINT32 batchSize )
   {
      SDB_ASSERT( batchSize > 0, "batch size must be greater than 0" );
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE__RELOADSTATSBYCONTEXT );

      std::unique_ptr< _rtnObjectStatAgent::contextBase > ctx = nullptr;
      if ( csName )
      {
         rc = _agent->getCollectionStatInfoOnCS( executor, csName, ctx );
         PD_RC_CHECK( rc, PDERROR, "failed to open cursor when get cs[%s] meta cache, rc: %d",
                      csName, rc );
      }
      else
      {
         rc = _agent->getAllCollectionStatInfo( executor, ctx );
         PD_RC_CHECK( rc, PDERROR, "failed to open cursor when get all meta cache rc: %d", csName,
                      rc );
      }

      while ( TRUE )
      {
         if ( batchSize == 1 )
         {
            RTN_CL_STAT_PTR clStatPtr = nullptr;
            rc = ctx->fetchOne( executor, clStatPtr );
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK;
               break;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fetch collection statistics on cs[%s]", csName );
               goto error;
            }
            try
            {
               ossScopedLock lock( &_mapLatch, EXCLUSIVE );
               _insertOrAssign( clStatPtr );
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDWARNING, "occur exception: %s", e.what() );
               rc = ossException2RC( &e );
               goto error;
            }
         }
         else
         {
            ossPoolVector< RTN_CL_STAT_PTR > clStats;
            rc = ctx->fetchBatch( executor, batchSize, clStats );
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK;
               try
               {
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  for ( ossPoolVector< RTN_CL_STAT_PTR >::const_iterator it = clStats.cbegin();
                        it != clStats.cend(); ++it )
                  {
                     _insertOrAssign( *it );
                  }
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDWARNING, "occur exception: %s", e.what() );
                  rc = ossException2RC( &e );
                  goto error;
               }
               break;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fetch collection statistics on cs[%s]", csName );
               goto error;
            }
            if ( !clStats.empty() )
            {
               try
               {
                  ossScopedLock lock( &_mapLatch, EXCLUSIVE );
                  for ( ossPoolVector< RTN_CL_STAT_PTR >::const_iterator it = clStats.cbegin();
                        it != clStats.cend(); ++it )
                  {
                     _insertOrAssign( *it );
                  }
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDWARNING, "occur exception: %s", e.what() );
                  rc = ossException2RC( &e );
                  goto error;
               }
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNOBJECTSTATCACHE__RELOADSTATSBYCONTEXT, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_REMOVECLSTAT1, "_rtnObjectStatCache::removeCLStat" )
   void _rtnObjectStatCache::removeCLStat( const CHAR *clFullName )
   {
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_REMOVECLSTAT1 );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      try
      {
         _mapNameToCL.erase( clFullName );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__RTNOBJECTSTATCACHE_REMOVECLSTAT1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_REMOVECLSTATINCS1, "_rtnObjectStatCache::removeCLStatInCS" )
   void _rtnObjectStatCache::removeCLStatInCS( const CHAR *csName )
   {
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_REMOVECLSTATINCS1 );
      try
      {
         ossScopedLock lock( &_mapLatch, EXCLUSIVE );
         MAP_NAME_CL::iterator start =
            std::lower_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameLower );
         MAP_NAME_CL::iterator end =
            std::upper_bound( _mapNameToCL.begin(), _mapNameToCL.end(), csName, findCSNameUpper );
         for ( MAP_NAME_CL::iterator it = start; it != end; )
         {
            it = _mapNameToCL.erase( it );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      PD_TRACE_EXIT( SDB__RTNOBJECTSTATCACHE_REMOVECLSTATINCS1 );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNOBJECTSTATCACHE_INVALIDATESTORAGECACHE, "_rtnObjectStatCache::removeAllStats" )
   void _rtnObjectStatCache::removeAllStats()
   {
      PD_TRACE_ENTRY( SDB__RTNOBJECTSTATCACHE_INVALIDATESTORAGECACHE );
      ossScopedLock lock( &_mapLatch, EXCLUSIVE );
      _mapNameToCL.clear();
      PD_TRACE_EXIT( SDB__RTNOBJECTSTATCACHE_INVALIDATESTORAGECACHE );
   }

   CONST_RTN_CL_STAT_PTR _rtnObjectStatCache::_find( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName && ( *clFullName ), "can not be null" );
      CONST_RTN_CL_STAT_PTR ans = nullptr;
      try
      {
         MAP_NAME_CL::const_iterator found = _mapNameToCL.find( clFullName );
         if ( found != _mapNameToCL.end() )
         {
            ans = found->second;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
      }
      return ans;
   }

   CONST_RTN_CL_STAT_PTR _rtnObjectStatCache::_findWithSharedLock( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName && ( *clFullName ), "can not be null" );
      ossScopedLock lock( &_mapLatch, SHARED );
      return _find( clFullName );
   }

   void _rtnObjectStatCache::_insertOrAssign( const CONST_RTN_CL_STAT_PTR &clStatPtr )
   {
      if ( !clStatPtr )
      {
         return;
      }
      try
      {
         MAP_NAME_CL::iterator found = _mapNameToCL.find( clStatPtr->getCLFullName() );
         if ( found != _mapNameToCL.end() )
         {
            _mapNameToCL[ clStatPtr->getCLFullName() ] = clStatPtr;
         }
         else
         {
            _mapNameToCL.emplace( clStatPtr->getCLFullName(), clStatPtr );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "occur exception: %s", e.what() );
      }
   }

   void _rtnObjectStatCache::_remove( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName && ( *clFullName ), "can not be null" );
      try
      {
         _mapNameToCL.erase( clFullName );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "occur exception: %s", e.what() );
      }
   }
} // namespace engine