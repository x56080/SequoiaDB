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
   void dirtyLobcBufferList::clear()
   {
      std::unique_lock<std::mutex> guard(_mutex);
      _totalBufferSize = 0;
      _minFlushLSN = DPS_INVALID_LSN_OFFSET;
      _minListLSN = DPS_INVALID_LSN_OFFSET;
      _list.clear();
      return;
   }

   void dirtyLobcBufferList::upsert(sharedLobChunkBuffer &buffer)
   {
      SDB_ASSERT(buffer && buffer->isValid(), "can not be invalid");

      if (!buffer->isBufferSizeRegistered())
      {
         std::unique_lock<std::mutex> guard(_mutex);
         _pushBackToList(buffer);
      }
      
      INT64 delta = static_cast<INT64>(buffer->getBufferCtx().getBufferSize()) -
                    static_cast<INT64>(buffer->getRegisteredBufferSize());
      SDB_ASSERT(0 <= delta, "impossible");
      _totalBufferSize.fetch_add(delta, std::memory_order_relaxed);
      buffer->registerBufferSize();
   }

   void dirtyLobcBufferList::makeFlushList(UINT64 bufferSizeLimit,
                                           lobcFlushList &fl)
   {
      SDB_ASSERT(fl.isEmpty(), "must be empty");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET == _minFlushLSN, "task still running");

      std::unique_lock<std::mutex> guard(_mutex);

      BUFFER_CTL_FLAG_WORD flags = LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH;
      BUFFER_CTL_FLAG_WORD condition = LOBC_BUFFER_CTL_FLAGS::BUSY |
                                       LOBC_BUFFER_CTL_FLAGS::TRASH;
      SHARED_LOBC_BUFFER_LIST::iterator left = _list.begin();
      SHARED_LOBC_BUFFER_LIST::iterator right = _list.begin();

      while (_list.end() != right)
      {
         BUFFER_CTL_FLAG_WORD oldVal = 0;
         sharedLobChunkBuffer &buffer = *right;
         if (!buffer->ctl().setFlagsIfNot(condition, flags, &oldVal))
         {
            if (left != right)
            {
               fl._list.splice(fl._list.end(), _list, left, right);
            }

            SDB_ASSERT(0 == OSS_BIT_TEST(oldVal, LOBC_BUFFER_CTL_FLAGS::PENDING_FLUSH), "impossible");
            SDB_ASSERT(0 != OSS_BIT_TEST(oldVal, LOBC_BUFFER_CTL_FLAGS::DIRTY), "impossible");
            if (0 != OSS_BIT_TEST(oldVal, LOBC_BUFFER_CTL_FLAGS::TRASH))
            {
               right = _list.erase(right);
               left = right;
            }
            else
            {
               SDB_ASSERT(0 != OSS_BIT_TEST(oldVal, LOBC_BUFFER_CTL_FLAGS::BUSY), "impossible");
               left = ++right;
            }
         }
         else
         {
            if (DPS_INVALID_LSN_OFFSET == fl._maxDirtyLSN ||
                fl._maxDirtyLSN < buffer->getMaxLSN())
            {
               fl._maxDirtyLSN = buffer->getMaxLSN();
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
         fl._list.splice(fl._list.end(), _list, left, right);
      }

      if (!_list.empty())
      {
         _minListLSN = _list.front()->getMinLSN();
      }
      else
      {
         _minListLSN = DPS_INVALID_LSN_OFFSET;
      }
      
      if (!fl.isEmpty())
      {
         _minFlushLSN = fl._list.front()->getMinLSN();
      }

      guard.unlock();
      _totalBufferSize.fetch_sub(fl._totalBufferSize, std::memory_order_relaxed);
   
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

   void dirtyLobcBufferList::resetFlushLSN()
   {
      _minFlushLSN = DPS_INVALID_LSN_OFFSET;
      return;
   }

   DPS_LSN_OFFSET dirtyLobcBufferList::peekMinDirtyLSN()const
   {
      return (DPS_INVALID_LSN_OFFSET == _minFlushLSN) ?
              _minListLSN : std::min(_minFlushLSN, _minListLSN);
   }

   void dirtyLobcBufferList::_pushBackToList(sharedLobChunkBuffer &buffer)
   {
      SDB_ASSERT(buffer->hasValidLSNPair(), "can not be invalid");

      DPS_LSN_OFFSET lsn = buffer->getMinLSN();

      if (!_list.empty())
      {
         SHARED_LOBC_BUFFER_LIST::iterator pos = _list.end();
         SHARED_LOBC_BUFFER_LIST::iterator itr = _list.end();
         do
         {
            --itr;
            if (lsn < (*itr)->getMinLSN())
            {
               pos = itr;
            }
            else
            {
               break;
            }
         } while (itr != _list.begin());
         
         _list.insert(pos, buffer);
      }
      else
      {
         _list.push_back(buffer);
      }

      if (DPS_INVALID_LSN_OFFSET == _minListLSN ||
          lsn < _minListLSN)
      {
         _minListLSN = lsn;
      }

      return;
   }

   void dirtyLobcBufferList::_pushFrontToList(sharedLobChunkBuffer &buffer)
   {
      SDB_ASSERT(buffer->hasValidLSNPair(), "can not be invalid");
      DPS_LSN_OFFSET lsn = buffer->getMinLSN();

      SHARED_LOBC_BUFFER_LIST::iterator pos = _list.begin();
      for (; pos != _list.end(); ++pos)
      {
         if (lsn < (*pos)->getMinLSN())
         {
            break;
         }
      }

      _list.insert(pos, buffer);

      if (DPS_INVALID_LSN_OFFSET == _minListLSN ||
          lsn < _minListLSN)
      {
         _minListLSN = lsn;
      }

      return;
   }

} // namespace vessel

} // namespace engine
