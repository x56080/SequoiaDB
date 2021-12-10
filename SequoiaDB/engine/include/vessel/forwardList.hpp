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

   Source File Name = forwardList.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FORWARD_LIST_HPP_
#define VESSEL_FORWARD_LIST_HPP_

#include "utilPooledObject.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class forwardListDummyLocker : public SDBObject
   {
      public:
         forwardListDummyLocker(){}
         ~forwardListDummyLocker(){}

      public:
         void lock(){}
         void unlock(){}
   };//class forwardListDummyLocker

   template<typename T, typename LOCKER=forwardListDummyLocker>
   class forwardList : public SDBObject
   {
      public:
         forwardList(){}
         ~forwardList(){}
         forwardList(const forwardList &) = delete;
         forwardList &operator=(const forwardList &) = delete;

      private:
         class _item : public _utilPooledObject
         {
            public:
               _item(){}
               ~_item(){}
               _item(const _item &) = delete;
               _item &operator=(const _item &) = delete;

            public:
               _item *next = NULL;
               T data;
         };//class _item

      public:
         INT32 pushForward(const T &data)
         {
            INT32 rc = SDB_OK;
            _item *item = SDB_OSS_NEW _item();
            if (OSS_UNLIKELY(NULL == item))
            {
               rc = SDB_OOM;
               goto error;
            }
            item->data = data;

            pushForward(TRUE, item);
         done:
            return rc;
         error:
            goto done;
         }

         INT32 pushForwardWithNoLock(const T &data)
         {
            INT32 rc = SDB_OK;
            _item *item = SDB_OSS_NEW _item();
            if (OSS_UNLIKELY(NULL == item))
            {
               rc = SDB_OOM;
               goto error;
            }
            item->data = data;

            pushForward(FALSE, item);
         done:
            return rc;
         error:
            goto done;
         }

         BOOLEAN popForward(T &data)
         {
            BOOLEAN r = FALSE;
            _item *item = NULL;
            if (popForward(TRUE, &item))
            {
               data = item->data;
               SDB_OSS_DEL item;
               r = TRUE;
            }
            return r;
         }

         BOOLEAN popForwardWithNoLock(T &data)
         {
            BOOLEAN r = FALSE;
            _item *item = NULL;
            if (popForward(FALSE, &item))
            {
               data = item->data;
               SDB_OSS_DEL item;
               r = TRUE;
            }
            return r;
         }

         /// WARNING: Will not hold latch.
         void clear()
         {
            T data;
            while (popForwardWithNoLock(data));
            return;
         }

         UINT32 peekSize()const
         {
            return *((const volatile UINT32 *)(&_size));
         }

         UINT32 getSize()
         {
            _latch.lock();
            UINT32 size = _size;
            _latch.unlock();
            return size;
         }

      private:
         void pushForward(BOOLEAN lock, _item *item)
         {
            if (lock)
            {
               _latch.lock();
            }

            item->next = _head;
            _head = item;
            ++_size;

            if (lock)
            {
               _latch.unlock();
            }
            return;
         }

         BOOLEAN popForward(BOOLEAN lock, _item **out)
         {
            BOOLEAN r = FALSE;
            if (lock)
            {
               _latch.lock();
            }

            if (0 < _size)
            {
               *out = _head;
               _head = _head->next;
               --_size;
               r = TRUE;
            }

            if (lock)
            {
               _latch.unlock();
            }

            return r;
         }

      private:
         LOCKER _latch;
         UINT32 _size = 0;
         _item *_head = NULL;
   };//class forwardList
}//namespace vessel
}//namespace engine

#endif//VESSEL_FORWARD_LIST_HPP_