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

   Source File Name = ioBufferControlBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/ioBufferControlBlock.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   ioBufferControlBlock::ioBufferControlBlock(const globalPageID &gpid,
                                              const bufferControlBlock &ctl,
                                              const mmapPagePointer &ptr):
   _gpid(gpid),
   _ctl(ctl),
   _mptr(ptr)
   {
      SDB_ASSERT(_gpid.isValid(), "can not be invalid");
      SDB_ASSERT(ctl.isNormal(), "must be normal");
      SDB_ASSERT(_mptr.isValid(), "can not be invalid");
   }

   void ioBufferControlBlock::updateLSNPair(DPS_LSN_OFFSET lsn)
   {
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      if (DPS_INVALID_LSN_OFFSET == _minDirtyLSN ||
          lsn < _minDirtyLSN)
      {
         _minDirtyLSN = lsn;
      }
      if (DPS_INVALID_LSN_OFFSET == _maxDirtyLSN ||
          _maxDirtyLSN < lsn)
      {
         _maxDirtyLSN = lsn;
      }
      return;
   }

   void ioBufferControlBlock::resetLSNPair()
   {
      _minDirtyLSN = DPS_INVALID_LSN_OFFSET;
      _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
      return;
   }

   void ioBufferControlBlock::releaseMemoryBlock(blockBasedMemPool &pool)
   {
      if (_mb.isValid())
      {
         pool.release(_mb);
         _mb.reset();
      }
   }
} // namespace vessel

} // namespace engine
