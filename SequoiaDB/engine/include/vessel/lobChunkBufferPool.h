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

   Source File Name = lobChunkBufferPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_CHUNK_BUFFER_POOL_H_
#define VESSEL_LOB_CHUNK_BUFFER_POOL_H_

#include "ossMemPool.hpp"
#include "vessel/lobcBufferPoolEnv.h"
#include "vessel/lobcBufferPoolWatcherEnv.h"
#include "vessel/strictBuffer.h"
#include "vessel/lextentDescriptor.h"
#include "vessel/lobcExtentChain.h"
#include "vessel/lobcBufferPoolOptions.h"

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
            BOOLEAN commitMetaData = FALSE;
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

      public:
         BOOLEAN isWatcherAttached()const;
         
         void attachWatcher();

         void waitUntilWatcherAttached();

         void detachWatcher();

         void flushAllDirtyBuffers();

         /// only for background workers!
         INT32 executeFlushTask(const lobcFlushTaskBuilder::taskId &task);

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
                                   UINT32 offset,
                                   UINT32 size)const;

      private:/// for pool watcher
         void flushDirtyList(UINT64 flushBufferSize);
         BOOLEAN betterToFlush(UINT64 &flushSize)const;
         BOOLEAN isFlushing()const;
         void handleFlushTaskRes(const backgroundEvent &event);

      private:
         lobcBufferPoolOptions _o;
         lobcBufferPoolEnv _env;
         lobcBufferPoolWatcherEnv _watcherEnv;
         
   };//class lobChunkBufferPool
} // namespace vessel

} // namespace engine

#endif//VESSEL_LOB_CHUNK_BUFFER_POOL_H_