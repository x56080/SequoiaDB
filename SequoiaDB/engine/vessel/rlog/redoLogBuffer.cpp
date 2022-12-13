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

   Source File Name = redoLogBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
