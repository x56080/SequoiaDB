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

   Source File Name = redoLogBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REDO_LOG_BUFFER_H_
#define VESSEL_REDO_LOG_BUFFER_H_

#include "dpsMessageBlock.hpp"
#include "ossRWMutex.hpp"
#include <memory>

namespace engine
{
namespace vessel
{
   class redoLogBuffer : public SDBObject
   {
      public:
         redoLogBuffer(UINT32 bufferId, UINT32 bufferSize);
         ~redoLogBuffer();
         redoLogBuffer(const redoLogBuffer &) = delete;
         redoLogBuffer &operator=(const redoLogBuffer &) = delete;

      public:
         BOOLEAN isValid() const;
         
         OSS_INLINE UINT32 getBufferId() const { return _bufferId; }
         OSS_INLINE UINT32 getTotalSize() const { return _mb.size(); }
         OSS_INLINE UINT32 getFreeSize() const { return _mb.idleSize(); }
         OSS_INLINE UINT32 getReservedSize() const { return _mb.length(); }
         OSS_INLINE UINT32 getUnsyncedBufSize() const { return _mb.getUnreadSize(); }
         OSS_INLINE const CHAR *getUnsyncedPtr() const { return _mb.readPtr(); }
         OSS_INLINE void lockShared() { _mtx.lock_r(); }
         OSS_INLINE void unlockShared() { _mtx.release_r(); }
         OSS_INLINE void lock() { _mtx.lock_w(); }
         OSS_INLINE void unlock() { _mtx.release_w(); }
         OSS_INLINE void wait(UINT32 &unsyncedSize)
         {
            _mtx.lock_w();
            unsyncedSize = getUnsyncedBufSize();
            _mtx.release_w();
            return;
         }
         
         struct block
         {
            block() = default;
            block(UINT32 o, UINT32 s):
            offset(o), size(s) {}
            UINT32 offset = 0;
            UINT32 size = 0;
         }; 

         block reserve(UINT32 size);

         void fill(UINT32 offset, UINT32 size, const CHAR *data);

         void updateSyncedPos(UINT32 size);

         void move(UINT32 size);

         void refresh();

      private:
         const UINT32 _bufferId = 0;
         ossRWMutex _mtx;
         dpsMessageBlock _mb;
   };//class redoLogBuffer

   using RLOG_BUFER_PTR = std::unique_ptr<redoLogBuffer>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_REDO_LOG_BUFFER_H_