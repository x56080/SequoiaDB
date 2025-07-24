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

   Source File Name = redoLogBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/redoLogBuffer.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   redoLogBuffer::redoLogBuffer(UINT32 bufferId,
                                UINT32 bufferSize):
   _bufferId(bufferId),
   _mb(bufferSize)
   {
      SDB_ASSERT(ossIsPowerOf2(bufferSize), "must be power of 2");
   }

   redoLogBuffer::~redoLogBuffer()
   {

   }

   BOOLEAN redoLogBuffer::isValid() const
   {
      return 0 < _mb.size();
   }

   redoLogBuffer::block redoLogBuffer::reserve(UINT32 size)
   {
      UINT32 currentOffset = _mb.length();
      UINT32 reserving = size <= _mb.idleSize() ?
                         size : _mb.idleSize();
      _mb.writePtr(currentOffset + reserving);
      return block(currentOffset, reserving);
   }

   void redoLogBuffer::fill(UINT32 offset, UINT32 size, const CHAR *data)
   {
      SDB_ASSERT((offset + size) <= _mb.length(), "out of bound");
      ossMemcpy(_mb.offset(offset), data, size);
      return;
   }

   void redoLogBuffer::move(UINT32 size)
   {
      SDB_ASSERT(size < _mb.size(), "out of size");
      _mb.writePtr(size);
      _mb.readPtr(size);
      return;
   }

   void redoLogBuffer::updateSyncedPos(UINT32 size)
   {
      SDB_ASSERT(size <= getUnsyncedBufSize(), "out of size");
      _mb.moveReadPtr(size);
      return;
   }

   void redoLogBuffer::refresh()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      _mb.readPtr(0);
      _mb.writePtr(0);
      return;
   }
} // namespace vessel

} // namespace engine
