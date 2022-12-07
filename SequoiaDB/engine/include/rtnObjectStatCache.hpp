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

   Source File Name = rtnObjectStatCache.hpp

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
#ifndef RTN_OBJECT_STAT_CACHE_HPP__
#define RTN_OBJECT_STAT_CACHE_HPP__

#include "rtnObjectStatAgent.hpp"
#include "utilStringView.hpp"
namespace engine
{
   class _rtnObjectStatCache
   {
   private:
      static constexpr UINT32 DEFAULT_BATCH_SIZE = 5;
   public:
      _rtnObjectStatCache() = default;

   public:
      void init( std::unique_ptr< rtnObjectStatAgent > &&agent );
      void fini();

      CONST_RTN_CL_STAT_PTR getCLStat( const CHAR *clFullName );

      CONST_RTN_INDEX_STAT_PTR getIndexStat( const CHAR *clFullName, const CHAR *indexName );

      INT32 getOrUpdateCLStat( IExecutor *executor,
                               const CHAR *clFullName,
                               CONST_RTN_CL_STAT_PTR &clStatPtr );

      // Firstly remove collection and index statistics on cl, then fetch new.
      INT32 reloadCLStats( IExecutor *executor, const CHAR *clFullName);

      // Firstly remove collection and index statistics on cs, then fetch new.
      INT32 reloadCSStats( IExecutor *executor,
                           const CHAR *csName,
                           UINT32 batchSize = DEFAULT_BATCH_SIZE );

      // Firstly remove all collection and index statistics, then fetch new.
      INT32 reloadAllStats( IExecutor *executor, UINT32 batchSize = DEFAULT_BATCH_SIZE );

      void removeCLStat( const CHAR *clFullName );

      void removeCLStatInCS( const CHAR *csName );

      void removeAllStats();

   private:
      CONST_RTN_CL_STAT_PTR _find( const CHAR *clFullName );
      CONST_RTN_CL_STAT_PTR _findWithSharedLock( const CHAR* clFullName);
      void _insertOrAssign( const CONST_RTN_CL_STAT_PTR &clStatPtr );
      void _remove( const CHAR *clFullName );

      // Firstly remove collection and index statistics in a CS, then fetch new.
      // If csName == nullptr, reload whole node
      INT32 _reloadStatsByContext( IExecutor *executor, const CHAR *csName, UINT32 batchSize );

   public:
      using MAP_NAME_CL = ossPoolMap< utilStringView, CONST_RTN_CL_STAT_PTR >;

   private:
      MAP_NAME_CL _mapNameToCL;
      ossSpinSLatch _mapLatch;

      std::unique_ptr< rtnObjectStatAgent > _agent = nullptr;
   };
   using rtnObjectStatCache = _rtnObjectStatCache;
} // namespace engine

#endif