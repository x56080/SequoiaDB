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

   Source File Name = ioBufferControlBlock.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
