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

   Source File Name = redoLogBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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