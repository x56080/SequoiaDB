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

   Source File Name = rlogBufferManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RLOG_BUFFER_MANAGER_H_
#define VESSEL_RLOG_BUFFER_MANAGER_H_

#include "vessel/redoLogBuffer.h"
#include "dpsPageMeta.hpp"
#include "dpsLogRecord.hpp"
#include "utilSlice.hpp"
#include "redoLogDef.h"
#include <mutex>
#include <condition_variable>

namespace engine
{
namespace vessel
{
   class rlogBufferManager : public SDBObject
   {
      public:
         rlogBufferManager() = default;
         ~rlogBufferManager();
         rlogBufferManager(const rlogBufferManager &) = delete;
         rlogBufferManager &operator=(const rlogBufferManager &) = delete;

      public:
         INT32 init(UINT32 totalBufferSize, UINT64 startOffset);

         void reset();
         
         void reserveAndLock(UINT32 size, dpsPageMeta &desc);

         void writeAndUnlock(const dpsLogRecordHeader &header,
                             const utilSlice &body,
                             const dpsPageMeta &pm);

         UINT32 getTotalBufSize() const;

         void freeBuffer(UINT32 bufferId); 

         redoLogBuffer *getBufferByOffset(UINT64 offset);

      public:
         OSS_INLINE UINT32 getMaxRequestBufSize() const
         {
            constexpr UINT32 _MAX_REQ_BUF_SIZE = 16 << 20;
            return _MAX_REQ_BUF_SIZE;
         }

         OSS_INLINE UINT32 getBufferPageSize() const
         {
            return RLOG_BUFFER_PAGE_SIZE;
         }

         OSS_INLINE UINT64 getBufStartOffset() const { return _bufStartOffset; }

      private:
         using _BUFFER_VEC = std::vector<RLOG_BUFER_PTR>;

      private:
         OSS_INLINE redoLogBuffer *_getBufferByMod(UINT32 pos)
         {
            return _buffers.at(pos % _buffers.size()).get();
         }
         OSS_INLINE redoLogBuffer *_getWorkingBuffer()
         {
            return _buffers.at(_workingPos).get();
         }
         OSS_INLINE redoLogBuffer *_getBuffer(UINT32 pos)
         {
            return _buffers.at(pos).get();
         }

      private:
         void _lockBuffers(const dpsPageMeta &pm);
         void _unlockBuffers(const dpsPageMeta &pm);
         void _write(const void *data,
                     UINT32 size,
                     UINT32 &pos,
                     UINT32 &offset);
         INT32 _adjustStartPos(UINT64 startOffset);
         void _switchWorkingBuffer();

      private:
         _BUFFER_VEC _buffers;
         UINT32 _freeBufSize = 0;
         UINT32 _workingPos = 0;
         UINT64 _bufStartOffset = 0;
         std::mutex _mutex;
         std::condition_variable _cv;
   };//class rlogBufferManager
} // namespace vessel

} // namespace engine


#endif//VESSEL_RLOG_BUFFER_MANAGER_H_