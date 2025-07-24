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

   Source File Name = lobChunkBufferPool.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lobChunkBufferPool.h"
#include "ossLikely.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilSharedPtrMaker.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileCluster.h"

namespace engine
{
namespace vessel
{
   static std::atomic_ullong _DUMMY_LSN = {0};

   lobChunkBufferPool::lobChunkBufferPool()
   {

   }

   lobChunkBufferPool::~lobChunkBufferPool()
   {
      fini();
   }

   INT32 lobChunkBufferPool::init(const lobcBufferPoolOptions &o)
   {
      INT32 rc = SDB_OK;

      _o = o;
      _o.correctIfNecessary();
      rc = _env.init(o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init poo env:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lobChunkBufferPool::fini()
   {
      detachWatcher();
      _env.fini();
      _watcherEnv.fini();
      _o = lobcBufferPoolOptions();
   }

   INT32 lobChunkBufferPool::read(const globalLobChunkKey &key,
                                  const lobcExtentChain &chain,
                                  UINT32 offset,
                                  UINT32 size,
                                  CHAR *buf)
   {
      INT32 rc = SDB_OK;
      sharedLobChunkBuffer buffer;
      _accessingContext context;
      storageFileCluster *fcluster = nullptr;
      /// consider about bind dms when init pool?
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");

      if (OSS_UNLIKELY(!key.isValid() ||
                       !chain.isValidAccessing(offset, size) ||
                       nullptr == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(chain.getPageSize() <= _env.getMemPool()->getBlockSize(),
                 "invalid page size");
      fcluster = tc->getEnv()->dms.getLobdFileCluster(key.getSpaceId());
      if (OSS_UNLIKELY(nullptr == fcluster))
      {
         PD_LOG(PDERROR, "failed to get lobd file cluster if sid[%d]",
                context.key->getSpaceId());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(fcluster->getCoreArgs().pageSize !=
                            chain.getPageSize()))
      {
         PD_LOG(PDERROR, "different page size found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (_findBufferToRead(key, buffer))
      {
         SDB_ASSERT(buffer->getPageSize() == chain.getPageSize(), "must be same");
      }

      context.resetToRead(&key, &chain, offset, size, buf, buffer.get());

      rc = _read(context, fcluster);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read lob chunk:%d", rc);
         goto error;
      }
   done:
      _endToRead(buffer.get());
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::write(const globalLobChunkKey &key,
                                   const lobcExtentChain &chain,
                                   UINT32 offset,
                                   const slice &data,
                                   const writeOptions &o)
   {
      INT32 rc = SDB_OK;
      sharedLobChunkBuffer buffer;
      _accessingContext context;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      storageFileCluster *fcluster = nullptr;
               
      if (OSS_UNLIKELY(!key.isValid() ||
                       !data.isValid() ||
                       !chain.isValidAccessing(offset, data.getSize())))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(chain.getPageSize() <= _env.getMemPool()->getBlockSize(),
                 "invalid page size");

      fcluster = tc->getEnv()->dms.getLobdFileCluster(key.getSpaceId());
      if (OSS_UNLIKELY(nullptr == fcluster))
      {
         PD_LOG(PDERROR, "failed to get lobd file cluster if sid[%d]",
                context.key->getSpaceId());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (OSS_UNLIKELY(fcluster->getCoreArgs().pageSize != chain.getPageSize()))
      {
         PD_LOG(PDERROR, "different page size found");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < chain.getChainSize(); ++i)
      {
         const lextentDescriptor &desc = chain.getChainItem(i);
         if (fcluster->isOutOfSpace(desc.pid) ||
             (1 < desc.pcnt && fcluster->isOutOfSpace(desc.pcnt + desc.pid - 1)))
         {
            PD_LOG(PDERROR, "extent [%d, %d] out of file space", desc.pid, desc.pcnt);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
      }

      rc = _ensureBufferToWrite(key, chain.getPageSize(), buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write, rc:%d", rc);
         goto error;
      }

      context.resetToWrite(&key, &chain, offset,
                           data.getSize(), data.data(), buffer.get());
      rc = _write(context, o, fcluster);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write lob chunk:%d", rc);
         goto error;
      }

      if (o.originalChunkSize != chain.getChunkSize())
      {
         buffer->setMetaDataToCommit();
      }

      _env.getDirtyList().insert(buffer);
   done:
      if (buffer)
      {
         buffer->ctl().decRefCnt(LOBC_BUFFER_CTL_FLAGS::BUSY);
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::remove(const globalLobChunkKey &key)
   {
      INT32 rc = SDB_OK;
      sharedLobChunkBuffer buffer;
      _accessingContext context;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      storageFileCluster *fcluster = nullptr;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fcluster = tc->getEnv()->dms.getLobdFileCluster(key.getSpaceId());
      if (OSS_UNLIKELY(nullptr == fcluster))
      {
         PD_LOG(PDERROR, "failed to get lobd file cluster if sid[%d]",
                context.key->getSpaceId());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _getBufferToRemove(key, fcluster->getCoreArgs().pageSize, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write, rc:%d", rc);
         goto error;
      }

      buffer->setLSN(_DUMMY_LSN.fetch_add(1, std::memory_order_relaxed));
      buffer->setMetaDataToCommit();
      _env.getDirtyList().insert(buffer);
   done:
      if (buffer)
      {
         buffer->ctl().decRefCnt(LOBC_BUFFER_CTL_FLAGS::BUSY);
      }
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::truncate(const globalLobChunkKey &key,
                                      const lobcExtentChain &chain)
   {
      INT32 rc = SDB_OK;
      sharedLobChunkBuffer buffer;

      if (OSS_UNLIKELY(!key.isValid() || !chain.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _ensureBufferToWrite(key, chain.getPageSize(), buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure buffer to write, rc:%d", rc);
         goto error;
      }

      /// it is unnecessary to truncate dirty page buffers here.
      /// new writing request will overwrite pages.
      /// however, truncate dirty buffers will reduce disk io. 
      buffer->setMetaDataToCommit();
      buffer->setLSN(_DUMMY_LSN.fetch_add(1, std::memory_order_relaxed));
      _env.getDirtyList().insert(buffer);

   done:
      if (buffer)
      {
         buffer->ctl().decRefCnt(LOBC_BUFFER_CTL_FLAGS::BUSY);
      }
      return rc;
   error:
      goto done;
   }

   void lobChunkBufferPool::discard(SPACE_ID sid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      
      _discardBuffersInDirtyList(sid, INVALID_CL_MB_ID);
      _discardBuffersInBuckets(sid, INVALID_CL_MB_ID);

      return;
   }

   void lobChunkBufferPool::discard(SPACE_ID sid, CL_MB_ID mbid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      
      _discardBuffersInDirtyList(sid, mbid);
      _discardBuffersInBuckets(sid, mbid);
      return;
   }

   void lobChunkBufferPool::_discardBuffersInDirtyList(SPACE_ID sid,
                                                       CL_MB_ID mbid)
   {
      dirtyLobcBufferList &dl = _env.getDirtyList();
      SHARED_LOBC_BUFFER_LIST l;
      dl.discard(sid, mbid, l);
      while (!l.empty())
      {
         atomicBufferCtlBlock &ctl = l.front()->ctl();
         ctl.resetFlags();
         if(OSS_LIKELY(ctl.setRecyclingFromNormal()))
         {
            l.front()->getBufferCtx().clear();
            BOOLEAN r = ctl.setDiscardedFromRecycling();
            SDB_ASSERT(r, "should not be failed");
            l.pop_front();
         }
         else
         {
            SDB_ASSERT(FALSE, "should not be failed");
         }
      }
   }

   void lobChunkBufferPool::_discardBuffersInBuckets(SPACE_ID sid,
                                                    CL_MB_ID mbid)
   {
      SHARED_LOBC_BUFFER_LIST l;
      for (UINT32 bucketId = 0; bucketId < _o.buckets; ++bucketId)
      {
         SHARED_LOBC_BUFFER_LIST &entry = _env.getBucketEntry(bucketId);
         std::mutex &mutex = _env.getEntryMutex(bucketId);
         std::unique_lock<std::mutex> guard(mutex);
         SHARED_LOBC_BUFFER_LIST::iterator itr = entry.begin();
         while (itr != entry.end())
         {
            atomicBufferCtlBlock &ctl = (*itr)->ctl();
            bufferControlBlock ctlSnapshot = ctl.load();
            if (ctlSnapshot.isDiscarded())
            {
               itr = entry.erase(itr);
            }
            else if ((*itr)->getKey().getSpaceId() == sid && 
                     (INVALID_CL_MB_ID == mbid || (*itr)->getKey().getMbId() == mbid))
            {
               l.splice(l.end(), entry, itr++);
            }
            else
            {
               ++itr;
            }
         }
      }
      
      while (!l.empty())
      {
         SHARED_LOBC_BUFFER_LIST::iterator itr = l.begin();
         while (itr != l.end())
         {
            atomicBufferCtlBlock &ctl = (*itr)->ctl();
            bufferControlBlock ctlSnapshot = ctl.load();
            if (ctlSnapshot.isDiscarded())
            {
               itr = l.erase(itr);
            }
            else
            {
               ++itr;
            }
         }
         if (l.empty())
         {
            break;
         }
         else
         {
            /// wait buffers flush done
            ossSleepmillis(10);
         }
      }

      return;
   }

   BOOLEAN lobChunkBufferPool::_findBufferToRead(const globalLobChunkKey &key,
                                                 sharedLobChunkBuffer &out)
   {
      SDB_ASSERT(key.isValid(), "can not be invalid");
      BOOLEAN r = FALSE;
      UINT32 hash = key.hash();
      UINT32 bucketId = 0;
      SHARED_LOBC_BUFFER_LIST &entry = _env.searchBucketEntry(hash, bucketId);
      std::mutex &mutex = _env.getEntryMutex(bucketId);

      std::unique_lock<std::mutex> guard(mutex);

      SHARED_LOBC_BUFFER_LIST::iterator itr = entry.begin();
      while (itr != entry.end())
      { 
         sharedLobChunkBuffer &buffer = (*itr);
         bufferControlBlock block = buffer->ctl().load();

         if (block.isDiscarded())
         {
            itr = entry.erase(itr);
            continue;
         }
         else if (block.isNormal() &&
                  key == buffer->getKey() &&
                  buffer->ctl().incRefCntIfNormal())
         {
            out = *itr;
            r = TRUE;
            break;
         }

         ++itr;
         continue;
      }
      
      return r;
   }

   INT32 lobChunkBufferPool::_ensureBufferToWrite(const globalLobChunkKey &key,
                                                  UINT32 pageSize,
                                                  sharedLobChunkBuffer &out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      UINT32 hash = key.hash();
      UINT32 bucketId = 0;
      SHARED_LOBC_BUFFER_LIST &entry = _env.searchBucketEntry(hash, bucketId);
      std::mutex &mutex = _env.getEntryMutex(bucketId);
      std::unique_lock<std::mutex> guard(mutex);

      SHARED_LOBC_BUFFER_LIST::iterator itr = entry.begin();
      while (entry.end() != itr)
      {
         sharedLobChunkBuffer &buffer = (*itr);
         bufferControlBlock block = buffer->ctl().load();
         if (block.isDiscarded())
         {
            itr = entry.erase(itr);
            continue;
         }
         else if (block.isNormal() &&
                  key == buffer->getKey() &&
                  buffer->ctl().incRefCntIfNormal())
         {
            break;
         }

         ++itr;
         continue;
      }

      if (entry.end() == itr)
      {
         bufferControlBlock block;
         block.init(BUFFER_STATUS::NORMAL, 1,
                    LOBC_BUFFER_CTL_FLAGS::BUSY);
         sharedLobChunkBuffer newBuffer =
                   makeSharedPtrFromPool<lobChunkBuffer>(key, block, pageSize, &_env);
         if (!newBuffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         out = newBuffer;
         entry.push_front(std::move(newBuffer));
      }
      else
      {
         sharedLobChunkBuffer &buffer = *itr;
         BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::BUSY;
         BUFFER_CTL_FLAG_WORD condition = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH;
         if (!buffer->ctl().setFlagIfNot(condition, flags))
         {
            bufferControlBlock block;
            block.init(BUFFER_STATUS::NORMAL, 1,
                       LOBC_BUFFER_CTL_FLAGS::BUSY);
            sharedLobChunkBuffer newBuffer =
                    makeSharedPtrFromPool<lobChunkBuffer>(key, block, pageSize, &_env);
            if (!newBuffer)
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            newBuffer->getBufferCtx().copyBuffers(buffer->getBufferCtx());
            out = newBuffer;
            *itr = std::move(newBuffer);
            /// we do not decrease the ref count of old buffer obj to avoid recycling.
            /// old buffer ptr will be destroyed when no longer be referenced.
         }
         else
         {
            out = buffer;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::_getBufferToRemove(const globalLobChunkKey &key,
                                                UINT32 pageSize,
                                                sharedLobChunkBuffer &out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      UINT32 hash = key.hash();
      UINT32 bucketId = 0;
      SHARED_LOBC_BUFFER_LIST &entry = _env.searchBucketEntry(hash, bucketId);
      std::mutex &mutex = _env.getEntryMutex(bucketId);

      std::unique_lock<std::mutex> guard(mutex);

      SHARED_LOBC_BUFFER_LIST::iterator itr = entry.begin();
      while (entry.end() != itr)
      {
         sharedLobChunkBuffer &buffer = (*itr);
         bufferControlBlock block = buffer->ctl().load();
         if (block.isDiscarded())
         {
            itr = entry.erase(itr);
            continue;
         }
         else if (block.isNormal() &&
                  key == buffer->getKey() &&
                  buffer->ctl().incRefCntIfNormal())
         {
            break;
         }

         ++itr;
         continue;
      }

      if (entry.end() == itr)
      {
         bufferControlBlock block;
         block.init(BUFFER_STATUS::NORMAL, 1,
                    LOBC_BUFFER_CTL_FLAGS::BUSY);
         sharedLobChunkBuffer newBuffer =
                   makeSharedPtrFromPool<lobChunkBuffer>(key, block, pageSize, &_env);
         if (!newBuffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         out = newBuffer;
         /// no need to insert buffer into bucket when removing.
         /// we can also release bucket latch first actually.
      }
      else
      {
         sharedLobChunkBuffer &buffer = *itr;
         BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::BUSY;
         BUFFER_CTL_FLAG_WORD condition = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH;
         if (!buffer->ctl().setFlagIfNot(condition, flags))
         {
            bufferControlBlock block;
            block.init(BUFFER_STATUS::NORMAL, 1,
                       LOBC_BUFFER_CTL_FLAGS::BUSY);
            sharedLobChunkBuffer newBuffer =
                    makeSharedPtrFromPool<lobChunkBuffer>(key, block, pageSize, &_env);
            if (!newBuffer)
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            out = newBuffer;
         }
         else
         {
            /// no need to hold buffer context,
            /// we do not access any page when remove it.
            buffer->getBufferCtx().clear();
            out = buffer;
         }

         entry.erase(itr);
      }

   done:
      return rc;
   error:
      goto done;
   }

   void lobChunkBufferPool::_endToRead(lobChunkBuffer *buffer)
   {
      if (nullptr != buffer)
      {
         buffer->ctl().decRefCnt();
         if (buffer->ctl().setRecyclingFromNormal())
         {
            buffer->getBufferCtx().clear();
            BOOLEAN r = buffer->ctl().setDiscardedFromRecycling();
            SDB_ASSERT(r, "can not be failed");
         }
      }
   }

   INT32 lobChunkBufferPool::_read(_accessingContext &context,
                                   storageFileCluster *fcluster)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(context.isReadyToRead(), "must be ready");
      SDB_ASSERT(nullptr != fcluster, "can not be invalid");
      
      strictBuffer &reqBuffer = context.requestBuffer;
      UINT32 offset = context.offset;
      UINT32 size = reqBuffer.getSize();
      UINT32 read = 0;

      ossPoolList<lobcExtentChain::extentRoadmap> roadmaps;
      rc = context.chain->createExtentRoadmaps(offset, size, roadmaps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create accessing roadmaps from chain:%d", rc);
         goto error;
      }

      for (auto itr = roadmaps.cbegin(); itr != roadmaps.cend(); ++itr)
      {
         UINT64 readFileOffset = 0;
         UINT64 readFileSize = 0;

         const lobcExtentChain::extentRoadmap &roadmap = *itr;
         for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)
         {
            PAGE_ID pid = roadmap.getPid(i);
            if (nullptr != context.chunkBuffer &&
                context.chunkBuffer->getBufferCtx().contains(pid))
            {
               strictBuffer pageBuffer =
                   context.chunkBuffer->getBufferCtx().getReadbleBuffer(pid);
               SDB_ASSERT(pageBuffer.isValid(), "can not be invalid");
               if (0 < readFileSize)
               {
                  CHAR *buf = reqBuffer.getWritablePtr(read, readFileSize);
                  rc = fcluster->read(readFileOffset, readFileSize, buf);
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to read data from file cluster:%d", rc);
                     goto error;
                  }

                  read += readFileSize;
                  readFileOffset = 0;
                  readFileSize = 0;
               }
               
               /// not else.
               {
                  slice ds = pageBuffer.getSlice(roadmap.getOffset(i),
                                                 roadmap.getSize(i));
                  rc = reqBuffer.write(read, ds.getSize(), ds.data());
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to copy data from buffer:%d", rc);
                     goto error;
                  }

                  read += ds.getSize();
               }

               continue;
            }

            /// context has no chunkBuffer or page not found in buffer context.
            /// we merge it into file request.
            if (0 == readFileSize)
            {
               readFileOffset =
                      static_cast<UINT64>(fcluster->getCoreArgs().pageSize) * pid;
               readFileOffset += roadmap.getOffset(i);
            }
            readFileSize += roadmap.getSize(i);
         }//for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)

         if (0 < readFileSize)
         {
            CHAR *buf = reqBuffer.getWritablePtr(read, readFileSize);
            rc = fcluster->read(readFileOffset, readFileSize, buf);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to read data from file cluster:%d", rc);
               goto error;
            }

            read += readFileSize;
         }
      }//for (auto itr = roadmaps.cbegin(); itr != roadmaps.cend(); ++itr)

      SDB_ASSERT(read == size, "must be same");
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::_write(_accessingContext &context,
                                    const writeOptions &o,
                                    storageFileCluster *fcluster)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(context.isReadyToWrite(), "must be ready");
      UINT32 newDataSize = 0;
      UINT32 sizeToOverwrite = getSizeToOverwrite(o.originalChunkSize,
                                                  fcluster->getCoreArgs().pageSize,
                                                  context.offset,
                                                  context.requestBuffer.getSize());
                                          
      context.chunkBuffer->setLSN(_DUMMY_LSN.fetch_add(1));
      
      if (0 < sizeToOverwrite)
      {
         rc = _overwrite(context, o.originalChunkSize, fcluster);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to overwrite lobc data:%d", rc);
            goto error;
         }

         newDataSize = context.requestBuffer.getSize() - sizeToOverwrite;
         if (0 == newDataSize)
         {
            goto done;
         }

         context.offset += sizeToOverwrite;
         context.requestBuffer =
                   context.requestBuffer.getReadableBuffer(newDataSize, sizeToOverwrite);
      }

      rc = _writeNewData(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write new data:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::_overwrite(_accessingContext &context,
                                        UINT32 originalChunkSize,
                                        storageFileCluster *fcluster)
   {
      INT32 rc = SDB_OK;
      UINT32 written = 0;
      UINT32 offset = context.offset;
      multiPageBufferContext &bufferCtx = context.chunkBuffer->getBufferCtx();
      UINT32 sizeToOverwrite = getSizeToOverwrite(originalChunkSize,
                                                  fcluster->getCoreArgs().pageSize,
                                                  context.offset,
                                                  context.requestBuffer.getSize());

      while (written < sizeToOverwrite)
      {
         lobcExtentChain::extentRoadmap roadmap;
         rc = context.chain->createExtentRoadmap(offset + written,
                                                 sizeToOverwrite - written,
                                                 roadmap);
         if (SDB_OK != rc)
         {
            /// panic here?
            PD_LOG(PDERROR, "failed to create accessing roadmap from chain:%d", rc);
            goto error;
         }

         for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)
         {
            PAGE_ID pid = roadmap.getPid(i);
            UINT32 poffset = roadmap.getOffset(i);
            UINT32 psize = roadmap.getSize(i);
            strictBuffer pageBuffer;
            slice dataToWrite = context.requestBuffer.getSlice(written, psize);
            SDB_ASSERT(dataToWrite.isValid(), "can not be invalid");

            /// if page already been buffered or
            /// totally overwritten, no need to load page from file.
            BOOLEAN needLoading = !bufferCtx.contains(pid) &&
                                  (
                                     0 != poffset ||
                                     (
                                        fcluster->getCoreArgs().pageSize != psize &&
                                        (offset + written + psize) < originalChunkSize
                                     )
                                  );

            rc = bufferCtx.makeBufferWritable(pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to make pid[%d] buffer writable:%d", rc);
               goto error;
            }

            pageBuffer = bufferCtx.getWritableBuffer(pid);
            SDB_ASSERT(pageBuffer.isWritable(), "must be writable");

            if (needLoading)
            {
               rc = fcluster->readPages(pid, 1, pageBuffer.getWPtr());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to read pid[%d] data:%d", pid, rc);
                  goto error;
               } 
            }
            
            rc = pageBuffer.write(poffset, dataToWrite.getSize(), dataToWrite.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to write page buffer:%d", rc);
               goto error;
            }

            written += psize;
         }//for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)
      }//while (written < sizeToOverwrite)

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobChunkBufferPool::_writeNewData(_accessingContext &context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(context.isReadyToWrite(), "can not be invalid");
      
      multiPageBufferContext &bufferCtx = context.chunkBuffer->getBufferCtx();
      strictBuffer &reqBuffer = context.requestBuffer;
      UINT32 size = reqBuffer.getSize();
      UINT32 offset = context.offset;
      UINT32 written = 0;

      do
      {
         lobcExtentChain::extentRoadmap roadmap;
         rc = context.chain->createExtentRoadmap(offset + written, size - written, roadmap);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create accessing roadmap from chain:%d", rc);
            goto error;
         }

         for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)
         {
            PAGE_ID pid = roadmap.getPid(i);
            UINT32 psize = roadmap.getSize(i);
            UINT32 poffset = roadmap.getOffset(i);
            strictBuffer pageBuffer;
            slice pageData = reqBuffer.getSlice(written, psize);
            SDB_ASSERT(pageData.isValid(), "can not be invalid");

            rc = bufferCtx.makeBufferWritable(pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to make pid[%d] writable:%d", pid, rc);
               goto error;
            }

            pageBuffer = bufferCtx.getWritableBuffer(pid);
            SDB_ASSERT(pageBuffer.isWritable(), "must be writable");

            rc = pageBuffer.write(poffset, psize, pageData.data());
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "failed to write page buffer:%d", rc);
               goto error;
            }

            written += psize;

         }//for (UINT32 i = 0; i < roadmap.getPcnt(); ++i)
      } while (written < size);
      
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 lobChunkBufferPool::getSizeToOverwrite(UINT32 originalSize,
                                                 UINT32 pageSize,
                                                 UINT32 offset,
                                                 UINT32 size)const
   {
      UINT32 result = 0;
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      if (0 < originalSize)
      {
         UINT32 capacity = ossAlignX(originalSize, pageSize);
         if (offset < capacity)
         {
            result = (offset + size) < capacity ?
                     size : (capacity - offset);
         }
      }
      return result;
   }

   void lobChunkBufferPool::waitUntilWatcherAttached()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      while (!isWatcherAttached())
      {
         ossSleepmillis(1);
      }
   }

   BOOLEAN lobChunkBufferPool::isWatcherAttached()const
   {
      return 1 == _watcherEnv._attached.load(std::memory_order_relaxed);
   }

   void lobChunkBufferPool::attachWatcher()
   {
      INT32 expected = 0;
      UINT32 millis = 10;
      backgroundEvent event, quitEvent;
      backgroundEvent flushEventRecved, runningFlushEvent;

      if (!_watcherEnv._attached.compare_exchange_strong(expected, 1))
      {
         SDB_ASSERT(FALSE, "already attached");
         goto done;
      }

      _watcherEnv.resetLastFlushTime();
      PD_LOG(PDINFO, "lobc buffer pool watcher attached");

      do
      {
         if (_watcherEnv._eventList.popOrWaitFor(millis, event))
         {
            if (event.isQuitEvent())
            {
               SDB_ASSERT(!quitEvent.isValid(), "already been quiting");
               quitEvent = event;
            }
            else if (BACKGROUND_EVENT_TYPE::FLUSH_LOB_BUF == event.getType())
            {
               SDB_ASSERT(!flushEventRecved.isValid() &&
                          !runningFlushEvent.isValid(), "already been running");
               SDB_ASSERT(!quitEvent.isValid(), "can not be quiting");
               if (isFlushing())
               {
                  /// if it is flushing, backup the event.
                  flushEventRecved = event;
               }
               else
               {
                  flushDirtyList(OSS_UINT64_MAX);
                  if (!isFlushing())
                  {
                     /// no dirty buffers or just finished.
                     if (event.hasResponser())
                     {
                        event.getResponser()->push(event.createSimpleResponse());
                     }
                  }
                  else
                  {
                     runningFlushEvent = event;
                  }
               }
            }
            else if (event.isResponseOf(BACKGROUND_EVENT_TYPE::LOB_BUF_TASK))
            {
               handleFlushTaskRes(event);
               if (!isFlushing())
               {
                  if (runningFlushEvent.isValid())
                  {
                     if (runningFlushEvent.hasResponser())
                     {
                        runningFlushEvent.getResponser()->push(
                                    runningFlushEvent.createSimpleResponse());
                     }
                     runningFlushEvent.reset();
                  }
                  else if (flushEventRecved.isValid())
                  {
                     runningFlushEvent = flushEventRecved;
                     flushEventRecved.reset();

                     flushDirtyList(OSS_UINT64_MAX);
                     if (!isFlushing())
                     {
                        if (runningFlushEvent.hasResponser())
                        {
                           runningFlushEvent.getResponser()->push(
                                   runningFlushEvent.createSimpleResponse());
                        }
                        runningFlushEvent.reset();
                     }
                  }
                  else if (!quitEvent.isValid())
                  {
                     UINT64 bufferSize = 0;
                     if (betterToFlush(bufferSize))
                     {
                        flushDirtyList(bufferSize);
                     }
                  }
               }
            }
            else
            {
               PD_LOG(PDERROR, "invalid event type found:%d", event.getType());
               SDB_ASSERT(FALSE, "unknown type");
            }
         }
         else
         {
            UINT64 bufferSize = 0;
            if (!quitEvent.isValid() && !isFlushing() && betterToFlush(bufferSize))
            {
               flushDirtyList(bufferSize);
            }
         }

         event.reset();
      } while (!quitEvent.isValid() || isFlushing());
      

      SDB_ASSERT(quitEvent.isQuitEvent(), "must be quit");
      if (quitEvent.hasResponser())
      {
         backgroundEvent res = quitEvent.createSimpleResponse();
         quitEvent.getResponser()->push(res);
      }
      _watcherEnv.fini();
      PD_LOG(PDINFO, "lobc buffer pool watcher detached");

   done:
      return;
   }

   void lobChunkBufferPool::detachWatcher()
   {
      if (isWatcherAttached())
      {
         backgroundEvent event = backgroundEvent::createQuitEvent();
         _watcherEnv._eventList.push(event);
         while (isWatcherAttached())
         {
            ossSleepmillis(10);
         }
      }
   }

   void lobChunkBufferPool::flushAllDirtyBuffers()
   {
      SDB_ASSERT(isValid() && isWatcherAttached(), "can not be invalid");
      backgroundEvent event, res;
      event.initAsRequest(BACKGROUND_EVENT_TYPE::FLUSH_LOB_BUF);
      autoEventList<backgroundEvent> list;
      event.setResponser(&list);
      _watcherEnv._eventList.push(event);
      list.popOrWait(res);
   }

   void lobChunkBufferPool::flushDirtyList(UINT64 flushBufferSize)
   {
      SDB_ASSERT(_watcherEnv._flushList.isEmpty(), "must be empty");
      SDB_ASSERT(0 < flushBufferSize, "can not be zero");

      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      backgroundWorkers &workers = tc->getEnv()->workers;
      backgroundEvent event;
      event.initAsRequest(BACKGROUND_EVENT_TYPE::LOB_BUF_TASK);
      event.setResponser(&_watcherEnv._eventList);
      
      _env.getDirtyList().makeFlushList(flushBufferSize,
                                        _watcherEnv._flushList);

      _watcherEnv._taskBuilder.build(_watcherEnv._flushList);

      ///TODO: push max lsn

      while(_watcherEnv._taskBuilder.hasMore())
      {
         bufferFlushTaskId task = _watcherEnv._taskBuilder.getNextTask();
         event.getShortData<bufferFlushTaskId>().offset = task.offset;
         event.getShortData<bufferFlushTaskId>().size = task.size;
         workers.pushEvent(event);
      }

      PD_LOG(PDDEBUG, "begin to flush dirty buffers, task count[%d, %d]",
             _watcherEnv._taskBuilder.getTotalTaskNum(),
             _watcherEnv._taskBuilder.getDispatchedTasks());

      /// finish at once if no task dispatched.
      if (0 == _watcherEnv._taskBuilder.getDispatchedTasks())
      {
         finishFlush();
      }

      return;
   }

   BOOLEAN lobChunkBufferPool::betterToFlush(UINT64 &flushSize)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      flushSize = 0;
      FLOAT32 memUsedPct = _env.getMemPool()->getUsedPct();
      constexpr UINT64 _MAX_TIMEOUT_FLUSH_SIZE = (UINT64)1 << 30;

      if (_o.flushDirtyListThreshold <= memUsedPct)
      {
         flushSize = _o.flushBatchSize;
      }
      else if (0 < _env.getMemPool()->getBlockAllocated() &&
               _o.flushDirtyListMillis <= _watcherEnv.getTimeSpanFromLastFlush())
      {
         flushSize = _env.getMemPool()->getTotalSizeAllocated() >> 2;
         if (_MAX_TIMEOUT_FLUSH_SIZE < flushSize)
         {
            flushSize = _MAX_TIMEOUT_FLUSH_SIZE;
         }
         else if (flushSize < _o.flushBatchSize)
         {
            flushSize = _o.flushBatchSize;
         }
      }
      
      return 0 < flushSize;
   }

   BOOLEAN lobChunkBufferPool::isFlushing()const
   {
      return !_watcherEnv._flushList.isEmpty();
   }

   void lobChunkBufferPool::handleFlushTaskRes(const backgroundEvent &event)
   {
      bufferFlushTaskId task;
      SDB_ASSERT(event.isResponseOf(BACKGROUND_EVENT_TYPE::LOB_BUF_TASK), "can not be others");
      task = event.getShortData<bufferFlushTaskId>();
      SDB_ASSERT(task.isValid() && task.offset < _watcherEnv._taskBuilder.getTotalTaskNum(),
                 "can not be invalid");
      SDB_ASSERT(_watcherEnv._completedTaskNum < _watcherEnv._taskBuilder.getDispatchedTasks(),
                 "can not be invalid");


      if (event.getRC() != SDB_OK)
      {
         PD_LOG(PDSEVERE, "failed to complete flush:%d", event.getRC());
      }

      if (++_watcherEnv._completedTaskNum ==
          _watcherEnv._taskBuilder.getDispatchedTasks())
      {
         finishFlush();
      }

      return;
   }

   INT32 lobChunkBufferPool::executeFlushTask(const bufferFlushTaskId &task)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!task.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isFlushing())
      {
         PD_LOG(PDERROR, "flush job is not running");
         SDB_ASSERT(FALSE, "impossible");
      }
      else if (_watcherEnv._taskBuilder.getTotalTaskNum() < (task.offset + task.size))
      {
         PD_LOG(PDERROR, "invalid task pos[%d,%d], current task num:%d",
                task.offset, task.size, _watcherEnv._taskBuilder.getTotalTaskNum());
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         const bufferFlushTask &firstTask = _watcherEnv._taskBuilder.get(task.offset);
         SDB_ASSERT(firstTask.isValid(), "can not be invalid");
         INT32 fileId = 0;
         storageFileCluster *fcluster =
                  tc->getEnv()->dms.getLobdFileCluster(firstTask.gpid().space());
         SDB_ASSERT(nullptr != fcluster, "can not be null");
         fileId = fcluster->getFileSpaceId(firstTask.gpid().page());

         rc = fcluster->writePages(firstTask.gpid().page(), 1, firstTask.getBuffer());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush page[%s], rc:%d",
                   firstTask.gpid().toString().c_str(), rc);
            goto error;
         }

         for (UINT32 i = 1; i < task.size; ++i)
         {
            const bufferFlushTask &otherTask =
                     _watcherEnv._taskBuilder.get(task.offset + i);
            INT32 fd = fcluster->getFileSpaceId(otherTask.gpid().page());
            SDB_ASSERT(fd == fileId, "must be same file");
            rc = fcluster->writePages(otherTask.gpid().page(), 1, otherTask.getBuffer());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to flush page[%s], rc:%d",
                     otherTask.gpid().toString().c_str(), rc);
               goto error;
            }
         }

         rc = fcluster->fsyncFile(fileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync file:%d", rc);
            goto error;
         }
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   void lobChunkBufferPool::finishFlush()
   {
      BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH |
                                   LOBC_BUFFER_CTL_FLAGS::IN_DIRTY_LIST;
      SHARED_LOBC_BUFFER_LIST &list = _watcherEnv._flushList._list;

      while (!list.empty())
      {
         sharedLobChunkBuffer &buffer = list.front();

         if (buffer->hasMetaDataToCommint())
         {
            ///TODO
            buffer->clearMetaData();
         }

         buffer->resetLSN();

         BUFFER_CTL_FLAG_WORD oldVal = buffer->ctl().clearFlag(flags);
         SDB_ASSERT(oldVal == flags, "must be same");

         if (buffer->ctl().setRecyclingFromNormal())
         {
            buffer->getBufferCtx().clear();
            BOOLEAN r = buffer->ctl().setDiscardedFromRecycling();
            SDB_ASSERT(r, "can not be failed");
         }

         list.pop_front();
      }

      _watcherEnv.flushDone();
      _env.getDirtyList().resetFlushLSN();
   }
} // namespace vesel

} // namespace engine
