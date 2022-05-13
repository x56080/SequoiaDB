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

   Source File Name = dirtyBufferList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DIRTY_BUFFER_LIST_HPP_
#define VESSEL_DIRTY_BUFFER_LIST_HPP_

#include "pdTrace.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"

#include <mutex>//c++11
#include <atomic>//c++11

namespace engine
{
namespace vessel
{
   template <class T>
   class dirtyBufferList : public SDBObject
   {
      public:
         dirtyBufferList() = default;
         virtual ~dirtyBufferList() = default;
         dirtyBufferList(const dirtyBufferList &) = delete;
         dirtyBufferList &operator=(const dirtyBufferList &) = delete;

      public:
         void clear();

         /// it is thread-safe but read without mutex
         DPS_LSN_OFFSET peekMinDirtyLSN()const;

         /// lock mutex and read
         DPS_LSN_OFFSET getMinDirtyLSN();

         void resetFlushLSN() {_minFlushLSN = DPS_INVALID_LSN_OFFSET;}

         UINT32 getSize()const {return _size.load(std::memory_order_relaxed);}

      protected:
         /// hold mutex outside
         void pushBackToList(T &buffer);
         /// hold mutex outside
         void pushFrontToList(T &buffer);

         void resetMinListLSNAndSize(BOOLEAN lock);
      
      protected:
         std::mutex _mutex;
         DPS_LSN_OFFSET _minFlushLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _minListLSN = DPS_INVALID_LSN_OFFSET;

         typedef ossPoolList<T> _BUFFER_LIST;
         _BUFFER_LIST _l;
         std::atomic_uint _size = {0};

   };//class dirtyBufferList

   template<class T>
   void dirtyBufferList<T>::clear()
   {
      std::unique_lock<std::mutex> guard(_mutex);
      _minFlushLSN = DPS_INVALID_LSN_OFFSET;
      _minListLSN = DPS_INVALID_LSN_OFFSET;
      _l.clear();
      _size.store(0, std::memory_order_relaxed);
      return;
   }

   template<class T>
   DPS_LSN_OFFSET dirtyBufferList<T>::peekMinDirtyLSN()const
   {
      return (DPS_INVALID_LSN_OFFSET == _minFlushLSN) ?
              _minListLSN : OSS_MIN(_minFlushLSN, _minListLSN);
   }
   
   template<class T>
   DPS_LSN_OFFSET  dirtyBufferList<T>::getMinDirtyLSN()
   {
      std::unique_lock<std::mutex> guard(_mutex);
      return peekMinDirtyLSN();
   }

   template<class T>
   void dirtyBufferList<T>::pushBackToList(T &buffer)
   {
      SDB_ASSERT(!(!buffer), "can not be invalid");
      DPS_LSN_OFFSET minLSN = buffer->getMinDirtyLSN();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != minLSN, "can not be invalid");
      
      if (!_l.empty())
      {
         typename _BUFFER_LIST::iterator pos = _l.end();
         typename _BUFFER_LIST::iterator itr = _l.end();
         do
         {
            --itr;
            if (minLSN < (*itr)->getMinDirtyLSN())
            {
               pos = itr;
            }
            else
            {
               break;
            }
         } while (itr != _l.begin());
         
         _l.insert(pos, buffer);
      }
      else
      {
         _l.push_back(buffer);
      }

      if (DPS_INVALID_LSN_OFFSET == _minListLSN ||
          minLSN < _minListLSN)
      {
         _minListLSN = minLSN;
      }

      _size.fetch_add(1, std::memory_order_relaxed);

      return;
   }

   template<class T>
   void dirtyBufferList<T>::pushFrontToList(T &buffer)
   {
      SDB_ASSERT(!(!buffer), "can not be invalid");
      DPS_LSN_OFFSET minLSN = buffer->getMinDirtyLSN();
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != minLSN, "can not be invalid");
      typename _BUFFER_LIST::iterator pos = _l.begin();

      for (; pos != _l.end(); ++pos)
      {
         if (minLSN <= (*pos)->getMinDirtyLSN())
         {
            break;
         }
      }

      _l.insert(pos, buffer);

      if (DPS_INVALID_LSN_OFFSET == _minListLSN ||
          minLSN < _minListLSN)
      {
         _minListLSN = minLSN;
      }

      _size.fetch_add(1, std::memory_order_relaxed);

      return;
   }

   template<class T>
   void dirtyBufferList<T>::resetMinListLSNAndSize(BOOLEAN lock)
   {
      std::unique_lock<std::mutex> guard(_mutex, std::defer_lock);
      if (lock)
      {
         guard.lock();
      }

      _minListLSN = _l.empty() ?
                    DPS_INVALID_LSN_OFFSET :
                    _l.front()->getMinDirtyLSN();
      _size.store(_l.size(), std::memory_order_relaxed);
      return;
   }
} // namespace vessel

} // namespace engine


#endif//VESSEL_DIRTY_BUFFER_LIST_HPP_