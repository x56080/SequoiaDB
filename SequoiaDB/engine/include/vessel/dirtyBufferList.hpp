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

   Source File Name = dirtyBufferList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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