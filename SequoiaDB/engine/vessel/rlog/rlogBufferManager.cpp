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

   Source File Name = rlogBufferManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rlogBufferManager.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/redoLogDef.h"

namespace engine
{
namespace vessel
{
   rlogBufferManager::~rlogBufferManager()
   {

   }

   INT32 rlogBufferManager::init(UINT32 totalBufferSize, UINT64 startOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_buffers.empty(), "do not reinit");
      UINT32 alignedBufferSize = ossAlignX(totalBufferSize, RLOG_BUFFER_PAGE_SIZE);
      UINT32 bufferNum = alignedBufferSize / RLOG_BUFFER_PAGE_SIZE;

      SDB_ASSERT((getMaxRequestBufSize() * 2) <= totalBufferSize, "buf size too small");

      try
      {
         _buffers.reserve(bufferNum);
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "failed to reserve space from vec:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }

      for (UINT32 i = 0; i < bufferNum; ++i)
      {
         RLOG_BUFER_PTR ptr(SDB_OSS_NEW redoLogBuffer(i, RLOG_BUFFER_PAGE_SIZE));
         if (!ptr || !ptr->isValid())
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         _buffers.push_back(std::move(ptr));
      }

      _freeBufSize = alignedBufferSize;

      _adjustStartPos(startOffset);
      
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void rlogBufferManager::reset()
   {
      _buffers.clear();
      _freeBufSize = 0;
      _workingPos = 0;
      _bufStartOffset = 0;
      return;
   }

   UINT32 rlogBufferManager::getTotalBufSize() const
   {
      return _buffers.size() * RLOG_BUFFER_PAGE_SIZE;
   }

   void rlogBufferManager::reserveAndLock(UINT32 size, dpsPageMeta &desc)
   {
      SDB_ASSERT(0 < size && size <= getTotalBufSize(), "invalid buffer size");
      UINT32 reserved = 0;
      desc.clear();
      std::unique_lock<std::mutex> lk(_mutex);
      _cv.wait(lk, [&, this]{return size <= _freeBufSize;});

      _freeBufSize -= size;
      redoLogBuffer *buffer = _getWorkingBuffer();
      SDB_ASSERT(0 < buffer->getFreeSize(), "invalid idle size");
      redoLogBuffer::block b = buffer->reserve(size);
      desc.offset = b.offset;
      desc.beginSub = buffer->getBufferId();
      desc.totalLen = size;
      desc.pageNum = 1;
      reserved = b.size;

      if (0 == buffer->getFreeSize())
      {
         _switchWorkingBuffer();
      }

      while (reserved < size)
      {
         buffer = _getWorkingBuffer();
         SDB_ASSERT(0 < buffer->getFreeSize(), "invalid idle size");
         b = buffer->reserve(size - reserved);
         reserved += b.size;
         ++desc.pageNum;
         if (0 == buffer->getFreeSize())
         {
            _switchWorkingBuffer();
         }
      }

      _lockBuffers(desc);

      return;
   }

   void rlogBufferManager::_lockBuffers(const dpsPageMeta &pm)
   {
      SDB_ASSERT(pm.valid(), "can not be invalid");
      UINT32 pos = pm.beginSub;
      for (UINT32 i = 0; i < pm.pageNum; ++i)
      {
         _getBufferByMod(pos)->lockShared();
         ++pos;
      }
      return;
   }

   void rlogBufferManager::writeAndUnlock(const dpsLogRecordHeader &header,
                                          const utilSlice &body,
                                          const dpsPageMeta &pm)
   {
      UINT32 pos = pm.beginSub;
      UINT32 offset = pm.offset;
      _write(&header, DPS_LOG_HEAD_SIZE, pos, offset);
      if (0 < body.size())
      {
         _write(body.data(), body.size(), pos, offset);
      }

      _unlockBuffers(pm);
   }

   void rlogBufferManager::_unlockBuffers(const dpsPageMeta &pm)
   {
      SDB_ASSERT(pm.valid(), "can not be invalid");
      UINT32 pos = pm.beginSub;
      for (UINT32 i = 0; i < pm.pageNum; ++i)
      {
         _getBufferByMod(pos)->unlockShared();
         ++pos;
      }
      return;
   }

   void rlogBufferManager::_write(const void *data,
                                  UINT32 size,
                                  UINT32 &pos,
                                  UINT32 &offset)
   {
      const CHAR *src = (const CHAR *)data;
      UINT32 srcOffset = 0;
      UINT32 lastDataSize = size;

      while (0 < lastDataSize)
      {
         redoLogBuffer *buffer = _getBufferByMod(pos);
         SDB_ASSERT(offset < buffer->getReservedSize(), "impossible");
         UINT32 fillingSize = buffer->getReservedSize() - offset;

         /// more buffer was reserved by someone else or size was aligned.
         if (lastDataSize < fillingSize)
         {
            fillingSize = lastDataSize;
         }
         else
         {
            SDB_ASSERT(lastDataSize == fillingSize ||
                       0 == buffer->getFreeSize(), "impossible");
         }

         buffer->fill(offset, fillingSize, src + srcOffset);

         srcOffset += fillingSize;
         lastDataSize -= fillingSize;
         offset += fillingSize;

         if (0 == offset % RLOG_BUFFER_PAGE_SIZE)
         {
            ++pos;
            offset = 0;
         }
      }

      return;
   }

   INT32 rlogBufferManager::_adjustStartPos(UINT64 startOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_buffers.empty(), "can not be empty");
      SDB_ASSERT(_freeBufSize == getTotalBufSize(), "must be same");
      _bufStartOffset = ossRoundDownToMultipleX(startOffset, RLOG_BUFFER_PAGE_SIZE);
      UINT32 movingSize = startOffset - _bufStartOffset;
      if (0 < movingSize)
      {
         _getBuffer(0)->move(movingSize);
         _freeBufSize -= movingSize;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void rlogBufferManager::_switchWorkingBuffer()
   {
      SDB_ASSERT(!_buffers.empty(), "can not be empty");
      if (++_workingPos == _buffers.size())
      {
         _workingPos = 0;
      }
      return;
   }

   void rlogBufferManager::freeBuffer(UINT32 bufferId)
   {
      SDB_ASSERT(bufferId < _buffers.size(), "out of bound");
      UINT32 totalBufSize = getTotalBufSize();
      redoLogBuffer *buffer = _getBuffer(bufferId);
   
      if (OSS_LIKELY(0 < buffer->getReservedSize()))
      {
         std::unique_lock<std::mutex> lk(_mutex);
         SDB_ASSERT(_freeBufSize + buffer->getReservedSize() <= totalBufSize, "out of size");
         _freeBufSize += buffer->getReservedSize();
         buffer->refresh();
         _cv.notify_all();
      }

      return;
   }

   redoLogBuffer *rlogBufferManager::getBufferByOffset(UINT64 offset)
   {
      SDB_ASSERT(!_buffers.empty(), "can not be empty");
      SDB_ASSERT(_bufStartOffset <= offset, "invalid offset");
      UINT64 pageStartOffset = ossRoundDownToMultipleX(offset, RLOG_BUFFER_PAGE_SIZE);
      UINT32 pos = ((pageStartOffset - _bufStartOffset) / RLOG_BUFFER_PAGE_SIZE);
      return _getBufferByMod(pos);
   }
} // namespace vessel

} // namespace engine
