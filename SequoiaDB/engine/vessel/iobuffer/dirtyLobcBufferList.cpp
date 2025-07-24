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

   Source File Name = dirtyLobcBufferList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/dirtyLobcBufferList.h"
#include "pdTrace.hpp"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   void dirtyLobcBufferList::insert(sharedLobChunkBuffer &buffer)
   {
      SDB_ASSERT(buffer && buffer->isValid(), "can not be invalid");
      SDB_ASSERT(buffer->hasMetaDataToCommint() ||
                 buffer->getBufferCtx().hasDirtyBuffer(), "nothing to flush");

      if (!buffer->isInDirtyList())
      {
         buffer->ctl().setFlag(LOBC_BUFFER_CTL_FLAGS::IN_DIRTY_LIST);
         std::unique_lock<std::mutex> guard(_mutex);
         pushBackToList(buffer);
      }
   }

   void dirtyLobcBufferList::makeFlushList(UINT64 bufferSizeLimit,
                                           lobcFlushList &fl)
   {
      SDB_ASSERT(fl.isEmpty(), "must be empty");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == _minFlushLSN, "task still running");

      std::unique_lock<std::mutex> guard(_mutex);

      BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH;
      BUFFER_CTL_FLAG_WORD condition = LOBC_BUFFER_CTL_FLAGS::BUSY;
      SHARED_LOBC_BUFFER_LIST::iterator left = _l.begin();
      SHARED_LOBC_BUFFER_LIST::iterator right = _l.begin();

      while (_l.end() != right)
      {
         BUFFER_CTL_FLAG_WORD oldVal = 0;
         sharedLobChunkBuffer &buffer = *right;
         if (!buffer->ctl().setFlagIfNot(condition, flags, &oldVal))
         {
            if (left != right)
            {
               fl._list.splice(fl._list.end(), _l, left, right);
            }

            SDB_ASSERT(0 != OSS_BIT_TEST(oldVal, LOBC_BUFFER_CTL_FLAGS::BUSY), "impossible");
            left = ++right;
         }
         else
         {
            if (DPS_INVALID_LSN_OFFSET == fl._maxDirtyLSN ||
                fl._maxDirtyLSN < buffer->getMaxDirtyLSN())
            {
               fl._maxDirtyLSN = buffer->getMaxDirtyLSN();
            }

            fl._totalBufferSize += buffer->getBufferCtx().getBufferSize();
            fl._dirtyPageCount += buffer->getBufferCtx().getDirtyBufferCount();
            ++right;

            if ((0 < bufferSizeLimit) && (bufferSizeLimit <= fl._totalBufferSize))
            {
               break;
            }
         }
      }

      if (left != right)
      {
         fl._list.splice(fl._list.end(), _l, left, right);
      }

      resetMinListLSNAndSize(FALSE);
      
      if (!fl.isEmpty())
      {
         _minFlushLSN = fl._list.front()->getMinDirtyLSN();
      }      
   
      return;
   }

   void dirtyLobcBufferList::discard(SPACE_ID sid, CL_MB_ID mbid, SHARED_LOBC_BUFFER_LIST &l) 
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);
      SHARED_LOBC_BUFFER_LIST::iterator left = _l.begin(), right = _l.begin();
      while (_l.end() != right)
      {
         if ((*right)->getKey().getSpaceId() == sid &&
             (INVALID_CL_MB_ID == mbid || (*right)->getKey().getMbId() == mbid))
         {
            ++right;
         }
         else if (left != right)
         {
            l.splice(l.end(), _l, left, right);
            ++right;
            left = right;
         }
         else
         {
            ++left;
            ++right;
         }
      }

      if (left != right)
      {
         l.splice(l.end(), _l, left, right);
      }

      resetMinListLSNAndSize(FALSE);
      return;
   }

} // namespace vessel

} // namespace engine
