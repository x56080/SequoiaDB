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

   Source File Name = lobChunkBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
