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

   Source File Name = liteCache.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_CACHE_H_
#define VESSEL_LITE_CACHE_H_

#include "vessel/liteCacheDef.h"
#include "vessel/lcPageTagHolder.h"
#include "vessel/vesselOptions.h"
#include "vessel/liteCacheTuple.h"

namespace engine
{
namespace vessel
{

class lcBuckets;
class lcLRUList;
class lcDirtyList;
class lcFreeList;
class diskIOJob;
class diskIOTask;
class requestContext;
class ISession;
class logicalPageSpace;

class liteCache : public SDBObject
{
   public:
      liteCache();
      ~liteCache();

      liteCache(const liteCache &) = delete;
      liteCache &operator=(const liteCache &) = delete;
      
   public: /// for normal requests
      INT32 init(INT32 poolNo,
                 UINT32 pageSize,
                 const liteCacheOptions &o);

      void fini();

      OSS_INLINE BOOLEAN isOpen()const
      {
         return 0 <= _poolNo;
      }
      OSS_INLINE INT32 getPoolNo()const
      {
         return _poolNo;
      }

      /// inc usage cnt and lock
      INT32 allocate(requestContext *request,
                     const GLOBAL_PAGE_ID &id,
                     const liteCacheAllocateOptions &options,
                     liteCacheTuple &tuple);

      INT32 allocateToReset(requestContext *request,
                            const GLOBAL_PAGE_ID &id,
                            liteCacheTuple &tuple);

      /// commit data if wrote something.
      void commit(UINT64 lsn,
                  liteCacheTuple &tuple);

   public:/// only for callback
      INT32 allocateMemPageAndInsertIntoLRU(requestContext *context,
                                            BOOLEAN zeroed,
                                            lcPageTagHolder &holder);

      INT32 tryToUpdateLRU(lcPageTagHolder &holder);

   public:/// only for background threads
      INT32 createIOJobIfNecessary(requestContext *context,
                                   diskIOJob *job);

      /// we should call this func when last flushing task almost done
      /// coz we always begin flushing from tail of lru. too much pending tags
      /// will waste scan steps.
      INT32 batchFlushOrEvictLRU(requestContext *context,
                                 UINT32 scanDepth,
                                 diskIOJob *job,
                                 UINT32 *involvedChunkPageCount=NULL);


      /// if last flushing is done and no more flushing,
      /// reset lru evict begin
      void resetLRUEvictBegin();

      /// it will release exclusive lock of dirty list until hit scanDepth or minLSN.
      INT32 createDirtyListIOJob(requestContext *context,
                                 UINT32 scanDepth,
                                 UINT64 minLSN,
                                 diskIOJob *job);

      INT32 executeIOTask(requestContext *context,
                          diskIOTask *task);

      void updateMinCacheLsn();

   public:
      UINT32 getDirtyListSizeFast()const;

   private:
      INT32 ensureMemPage(requestContext *context, freeListPage &page);

      INT32 fsyncIOTask(requestContext *context,
                        diskIOTask *task);

      void releaseTag();

      void correctOptions(liteCacheOptions &options);

   private:
      INT32 _poolNo = -1;
      lcBuckets *_buckets = NULL;
      lcLRUList *_lru = NULL;
      lcDirtyList *_dl = NULL;
      lcFreeList *_fl = NULL;
      liteCacheOptions::flushOptions _flushOptions;

      //UNIQUE_MUTEX _evictLRULock;
}; /// end of class liteCache
} /// end of namespace vessel
} /// end of namespace engine


#endif