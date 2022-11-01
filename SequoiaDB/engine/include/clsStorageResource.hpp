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

   Source File Name = clsStorageResource.hpp

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
#ifndef CLS_STORAGE_RESOURCE_HPP__
#define CLS_STORAGE_RESOURCE_HPP__

#include "clsStorageResourceAgent.hpp"
#include "utilStringView.hpp"
namespace engine
{
   class _clsStorageResource
   {
   public:
      struct result
      {
         CONST_CLS_INDEX_INFO_SET_PTR indexInfoSetPtr = nullptr;
         CONST_CLS_CL_STAT_PTR clStatPtr = nullptr;
         result() {}
         result( const CONST_CLS_INDEX_INFO_SET_PTR &indexInfoSetPtr,
                 const CONST_CLS_CL_STAT_PTR &clStatPtr )
         : indexInfoSetPtr( indexInfoSetPtr ), clStatPtr( clStatPtr )
         {
         }
         result( const clsCLMetaCache &clCache )
         : result( clCache._indexInfoSetPtr, clCache._clStatPtr )
         {
         }
         void reset()
         {
            indexInfoSetPtr.reset();
         }
      };

   public:
      _clsStorageResource() {}

   public:
      void init( std::unique_ptr< clsStorageResourceAgent > &&agent );
      void fini();

      result getCLMetaCache( const CHAR *clFullName );
      result getCLMetaCache( utilCLUniqueID clUID );
      CONST_CLS_INDEX_INFO_SET_PTR getCLIndexSet( const CHAR *clFullName );
      CONST_CLS_INDEX_INFO_SET_PTR getCLIndexSet( utilCLUniqueID clUID );

      CONST_CLS_CL_STAT_PTR getCLStat( const CHAR *clFullName );
      CONST_CLS_CL_STAT_PTR getCLStat( utilCLUniqueID clUID );

      CONST_CLS_INDEX_STAT_PTR getIndexStat( const CHAR *clFullName, const CHAR *indexName );
      CONST_CLS_INDEX_STAT_PTR getIndexStat( utilCLUniqueID clUID, utilIdxInnerID idxInnerI );

      INT32 getOrUpdateCLStat( IExecutor *executor,
                               const CHAR *clFullName,
                               CONST_CLS_CL_STAT_PTR &clStatPtr );
      INT32 getOrUpdateCLStat( IExecutor *executor,
                               utilCLUniqueID clUID,
                               CONST_CLS_CL_STAT_PTR &clStatPtr );

      INT32 getOrUpdateCLIndexSet( IExecutor *executor,
                                   const CHAR *clFullName,
                                   CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );
      INT32 getOrUpdateCLIndexSet( IExecutor *executor,
                                   utilCLUniqueID clUID,
                                   CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );

      INT32 getOrUpdateCLIndexStats( IExecutor *executor,
                                     const CHAR *clFullName,
                                     CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );
      INT32 getOrUpdateCLIndexStats( IExecutor *executor,
                                     utilCLUniqueID clUID,
                                     CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );

      INT32 getOrUpdateIndexStat( IExecutor *executor,
                                  const CHAR *clFullName,
                                  const CHAR *indexName,
                                  CONST_CLS_INDEX_STAT_PTR &indexStatPtr );
      INT32 getOrUpdateIndexStat( IExecutor *executor,
                                  utilCLUniqueID clUID,
                                  utilIdxInnerID idxInnerID,
                                  CONST_CLS_INDEX_STAT_PTR &indexStatPtr );
      // obj: clsCLStat BSONObj
      INT32 upsertCLStat( IExecutor *executor,
                          const CHAR *clFullName,
                          utilCLUniqueID clUID,
                          const BSONObj &obj );
      // obj: clsIndexStat BSONObj
      INT32 upsertIndexStat( IExecutor *executor,
                             const CHAR *clFullName,
                             utilCLUniqueID clUID,
                             const CHAR *indexName,
                             const BSONObj &obj );

      void invalidateStorageCache();

      void removeCLMetaCache( const CHAR *clFullName );
      void removeCLMetaCache( utilCLUniqueID clUID );
      void removeCSMetaCache( const CHAR *csName );
      void removeCSMetaCache( utilCSUniqueID csUID );

      void removeCLStat( const CHAR *clFullName );
      void removeCLStat( utilCLUniqueID clUID );
      void removeCLStatInCS( const CHAR *csName );
      void removeCLStatInCS( utilCSUniqueID csUID );

      void removeCLIndexStats( const CHAR *clFullName );
      void removeCLIndexStats( utilCLUniqueID clUID );

      void removeCSIndexStats( const CHAR *csName );
      void removeCSIndexStats( utilCSUniqueID csUID );

      void removeIndexStat( const CHAR *clFullName, const CHAR *indexName );
      void removeIndexStat( utilCLUniqueID clUID, utilIdxInnerID idxInnerID );

      void removeAllCLStats();
      void removeAllIndexStats();

   private:
      UINT32 _getLatchPos( const CHAR *name, UINT32 len );
      UINT32 _getLatchPos( utilCLUniqueID clUID );
      INT32 _updateIndexSet( IExecutor *executor,
                             const CHAR *clFullName,
                             BOOLEAN withStat,
                             CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );
      INT32 _updateIndexSet( IExecutor *executor,
                             utilCLUniqueID clUID,
                             BOOLEAN withStat,
                             CONST_CLS_INDEX_INFO_SET_PTR &indexSetPtr );
      void _insert( const clsCLMetaCachePtr &clCataSetPtr );
      void _remove( const CHAR *clFullName );
      void _remove( utilCLUniqueID clUID );
      CLS_INDEX_INFO_SET_PTR _getCLIndexInfoSet( const CHAR *clFullName, BOOLEAN needLockShared );
      CLS_INDEX_INFO_SET_PTR _getCLIndexInfoSet( utilCLUniqueID clUID, BOOLEAN needLockShared );

      clsCLMetaCachePtr _getCLMetaCache( const CHAR *clFullName, BOOLEAN needLockShared );
      clsCLMetaCachePtr _getCLMetaCache( utilCLUniqueID clUID, BOOLEAN needLockShared );

      CONST_CLS_INDEX_STAT_PTR _getIndexStat( const CHAR *clFullName,
                                              const CHAR *indexName,
                                              BOOLEAN needLockShared );
      CONST_CLS_INDEX_STAT_PTR _getIndexStat( utilCLUniqueID clUID,
                                              utilIdxInnerID idxInnerID,
                                              BOOLEAN needLockShared );

   public:
      using MAP_NAME_CL = ossPoolMap< utilStringView, clsCLMetaCachePtr >;
      using MAP_CLUID_CL = ossPoolMap< utilCLUniqueID, clsCLMetaCachePtr >;

   private:
      MAP_NAME_CL _mapNameToCL;
      MAP_CLUID_CL _mapUidToCL;
      ossSpinSLatch _mapLatch;

      static constexpr INT32 LATCH_COUNT = 32;
      ossSpinXLatch _latches[ LATCH_COUNT ];

      std::unique_ptr< clsStorageResourceAgent > _agent;
   };
   using clsStorageResource = _clsStorageResource;
} // namespace engine

#endif