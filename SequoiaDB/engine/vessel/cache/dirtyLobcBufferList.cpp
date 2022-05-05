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

   Source File Name = dirtyLobcBufferList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dirtyLobcBufferList.h"
#include "pdTrace.hpp"

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
         buffer->ctl().setFlags(LOBC_BUFFER_CTL_FLAGS::IN_DIRTY_LIST);
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
         if (!buffer->ctl().setFlagsIfNot(condition, flags, &oldVal))
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

            if (buffer->isTrash())
            {
               if (left != right)
               {
                  fl._list.splice(fl._list.end(), _l, left, right);
               }
               right = _l.erase(right);
               left = right;
            }
            else
            {
               ++right;
            }
            
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

      if (!_l.empty())
      {
         _minListLSN = _l.front()->getMinDirtyLSN();
      }
      else
      {
         _minListLSN = DPS_INVALID_LSN_OFFSET;
      }
      
      if (!fl.isEmpty())
      {
         _minFlushLSN = fl._list.front()->getMinDirtyLSN();
      }      
   
   /* splice whole list first, and repush back busy ones.
      if (!_list.empty())
      {
         _minFlushLSN = _minListLSN;
         _minListLSN = DPS_INVALID_LSN_OFFSET;
         flushList.splice(flushList.end(), _list);
         _totalBufferSize = 0;

         guard.unlock();

         /// push back busy buffers. it should not be too many.
         BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH;
         BUFFER_CTL_FLAG_WORD condition = LOBC_BUFFER_CTL_FLAGS::BUSY;
         SHARED_LOBC_BUFFER_LIST::iterator itr = flushList.begin();
         while (itr != flushList.end())
         {
            sharedLobChunkBuffer &buffer = *itr;
            if (!buffer->ctl().setFlagsIfNot(condition, flags))
            {
               guard.lock();
               _pushFrontToList(buffer);
               _totalBufferSize += buffer->getRegisteredBufferSize();
               guard.unlock();
               itr = flushList.erase(itr);
               continue;
            }
            else
            {
               ++itr;
            }
         }
      }
      */
      return;
   }

} // namespace vessel

} // namespace engine
