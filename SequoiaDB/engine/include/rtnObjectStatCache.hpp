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