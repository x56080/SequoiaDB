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

   Source File Name = lobChunkBufferPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOB_CHUNK_BUFFER_POOL_H_
#define VESSEL_LOB_CHUNK_BUFFER_POOL_H_

#include "ossMemPool.hpp"
#include "vessel/lobcBufferPoolEnv.h"
#include "vessel/lobcBufferPoolWatcherEnv.h"
#include "vessel/strictBuffer.h"
#include "vessel/lextentDescriptor.h"
#include "vessel/lobcExtentChain.h"
#include "vessel/bufferPoolOptions.h"

namespace engine
{
namespace vessel
{
   class storageFileCluster;

   class lobChunkBufferPool : public SDBObject
   {
      public:
         lobChunkBufferPool();
         ~lobChunkBufferPool();
         lobChunkBufferPool(const lobChunkBufferPool &) = delete;
         lobChunkBufferPool &operator=(const lobChunkBufferPool &) = delete;

      public:
         struct writeOptions : public SDBObject
         {
            UINT32 originalChunkSize = 0;
         };//struct writeOptions

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _env.isValid();
         }
         INT32 init(const lobcBufferPoolOptions &o);
         void fini();

         /// get x lock of key outside first
         INT32 write(const globalLobChunkKey &key,
                     const lobcExtentChain &chain,
                     UINT32 offset,
                     const slice &data,
                     const writeOptions &o);

         /// get s/x lock of key outside first
         INT32 read(const globalLobChunkKey &key,
                    const lobcExtentChain &chain,
                    UINT32 offset,
                    UINT32 size,
                    CHAR *buf);

         /// get x lock of key outside first
         INT32 remove(const globalLobChunkKey &key);

         INT32 truncate(const globalLobChunkKey &key,
                        const lobcExtentChain &chain);

         //discard all buffers of the collection space
         void discard(SPACE_ID sid);

         /// ensure no one can access the collection to be discarded.
         //discard all buffers of the collection
         void discard(SPACE_ID sid, CL_MB_ID mbid);

      public:
         BOOLEAN isWatcherAttached()const;
         
         void attachWatcher();

         void waitUntilWatcherAttached();

         void detachWatcher();

         void flushAllDirtyBuffers();

         /// only for background workers!
         INT32 executeFlushTask(const bufferFlushTaskId &task);

      private:
         struct _accessingContext : public SDBObject
         {
            void resetToRead(const globalLobChunkKey *key,
                             const lobcExtentChain *chain,
                             UINT32 offset, UINT32 size, CHAR *buf,
                             lobChunkBuffer *chunkBuffer)
            {
               this->key = key;
               this->chain = chain;
               this->offset = offset;
               requestBuffer.makeWritable(size, buf);
               this->chunkBuffer = chunkBuffer;
            }

            BOOLEAN isReadyToRead()const
            {
               return nullptr != key && key->isValid() &&
                      nullptr != chain && !chain->isEmpty() &&
                      requestBuffer.isWritable();
            }

            void resetToWrite(const globalLobChunkKey *key,
                              const lobcExtentChain *chain,
                              UINT32 offset, UINT32 size,
                              const CHAR *buf,
                              lobChunkBuffer *chunkBuffer)
            {
               this->key = key;
               this->chain = chain;
               this->offset = offset;
               requestBuffer.reset(size, buf);
               this->chunkBuffer = chunkBuffer;
            }

            BOOLEAN isReadyToWrite()const
            {
               return nullptr != key && key->isValid() &&
                      nullptr != chain && !chain->isEmpty() &&
                      requestBuffer.isValid() &&
                      nullptr != chunkBuffer &&
                      chunkBuffer->isValid();
            }

            const globalLobChunkKey *key = nullptr;
            const lobcExtentChain *chain = nullptr;
            UINT32 offset = 0;
            strictBuffer requestBuffer;
            lobChunkBuffer *chunkBuffer = nullptr;
         };//struct _accessingContext

      private:///bucket accessing
         BOOLEAN _findBufferToRead(const globalLobChunkKey &key,
                                   sharedLobChunkBuffer &out);
         INT32 _ensureBufferToWrite(const globalLobChunkKey &key,
                                    UINT32 pageSize,
                                    sharedLobChunkBuffer &out);

         INT32 _getBufferToRemove(const globalLobChunkKey &key,
                                  UINT32 pageSize,
                                  sharedLobChunkBuffer &out);

         void _discardBuffersInDirtyList(SPACE_ID sid,
                                         CL_MB_ID mbid);

         void _discardBuffersInBuckets(SPACE_ID sid,
                                      CL_MB_ID mbid);

         INT32 _write(_accessingContext &context,
                      const writeOptions &o,
                      storageFileCluster *fcluster);

         INT32 _read(_accessingContext &context,
                     storageFileCluster *fcluster);

      private:
         INT32 _writeNewData(_accessingContext &context);

         INT32 _overwrite(_accessingContext &context,
                          UINT32 originalChunkSize,
                          storageFileCluster *fcluster);

         void _endToRead(lobChunkBuffer *buffer);

         UINT32 getSizeToOverwrite(UINT32 originalSize,
                                   UINT32 pageSize,
                                   UINT32 offset,
                                   UINT32 size)const;

      private:/// for pool watcher
         void flushDirtyList(UINT64 flushBufferSize);
         BOOLEAN betterToFlush(UINT64 &flushSize)const;
         BOOLEAN isFlushing()const;
         void handleFlushTaskRes(const backgroundEvent &event);
         void finishFlush();

      private:
         lobcBufferPoolOptions _o;
         lobcBufferPoolEnv _env;
         lobcBufferPoolWatcherEnv _watcherEnv;
         
   };//class lobChunkBufferPool
} // namespace vessel

} // namespace engine

#endif//VESSEL_LOB_CHUNK_BUFFER_POOL_H_