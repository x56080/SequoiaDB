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

   Source File Name = lobChunkBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lobChunkBuffer.h"
#include "pdTrace.hpp"
#include "vessel/lobcBufferPoolEnv.h"

namespace engine
{
namespace vessel
{
   lobChunkBuffer::lobChunkBuffer(const globalLobChunkKey &key,
                                  const bufferControlBlock &blk,
                                  UINT32 pageSize,
                                  lobcBufferPoolEnv *env):
   _key(key),
   _ctl(blk),
   _bufferCtx(pageSize, env->getMemPool())
   {

   }

   lobChunkBuffer::~lobChunkBuffer()
   {

   }

   void lobChunkBuffer::setLSN(const DPS_LSN_OFFSET &lsn)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      if (DPS_INVALID_LSN_OFFSET == _minLSN ||
          lsn < _minLSN)
      {
         _minLSN = lsn;
      }

      if (DPS_INVALID_LSN_OFFSET == _maxLSN ||
          _maxLSN < lsn)
      {
         _maxLSN = lsn;
      }
      
      return;
   }

   void lobChunkBuffer::resetLSN()
   {
      _minLSN = DPS_INVALID_LSN_OFFSET;
      _maxLSN = DPS_INVALID_LSN_OFFSET;
   }

   void lobChunkBuffer::exportTasks(ossPoolVector<bufferFlushTask> &tasks)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const ossPoolSet<PAGE_ID> &dirtyPids = _bufferCtx.getDirtyPids();
      for (ossPoolSet<PAGE_ID>::const_iterator itr = dirtyPids.cbegin();
           itr != dirtyPids.cend(); ++itr)
      {
         globalPageID gpid(_key.getSpaceId(), SPACE_TYPE_LOB,
                           FILE_TYPE_DATA_STORAGE, *itr);
         strictBuffer buffer = _bufferCtx.getReadbleBuffer(*itr);
         tasks.push_back(bufferFlushTask(gpid, buffer.getRPtr()));
      }
   }
} // namespace vesssel

} // namespace engine
