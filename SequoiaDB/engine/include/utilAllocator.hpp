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

   Source File Name = utilAllocator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/04/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_ALLOCATOR_HPP_
#define UTIL_ALLOCATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"

using namespace std ;

#define UTIL_ALLOCATOR_SIZE   256

namespace engine
{
   template < UINT32 stackSize = UTIL_ALLOCATOR_SIZE >
   class _utilStackOnlyAllocator
   {
      public :
         _utilStackOnlyAllocator()
         {
            _offset = 0 ;
         }

         virtual ~_utilStackOnlyAllocator()
         {
            _offset = 0 ;
         }

         void* allocate ( size_t size )
         {
            void *p = NULL ;
            if ( _offset + size <= stackSize )
            {
               p = _mem + _offset ;
               _offset += size ;
            }

            return p ;
         }

         BOOLEAN isAllocatedByme ( void *p )
         {
            if ( p >= _mem && p < _mem + stackSize )
            {
               return TRUE ;
            }

            return FALSE ;
         }

      protected :
         char _mem[ stackSize ] ;
         INT32 _offset ;
   } ;//class _utilStackOnlyAllocator

   class utilBaseAllocator : public SDBObject
   {
      public:
         utilBaseAllocator() = default;
         virtual ~utilBaseAllocator() = default;
         utilBaseAllocator(const utilBaseAllocator &) = delete;
         utilBaseAllocator &operator=(const utilBaseAllocator &) = delete;
      public:
         virtual void *malloc(size_t size) = 0;
         virtual void free(void *p) = 0;
         virtual void *realloc(void *p, size_t size) = 0;
   };//class utilBaseAllocator

   class utilPoolAllocator : public utilBaseAllocator
   {
      public:
         virtual void *malloc(size_t size) override
         {
            return SDB_THREAD_ALLOC(size);
         }
         virtual void *realloc(void *p, size_t size) override
         {
            void *ptr = nullptr;
            if (nullptr == p)
            {
               ptr = this->malloc(size);
            }
            else
            {
               ptr = SDB_THREAD_REALLOC(p, size);
            }
            return ptr;
         }
         virtual void free(void *p) override
         {
            SDB_THREAD_FREE(p);
         }
   };//utilPoolAllocator

   template<UINT32 STACK_SIZE=512>
   class utilStackAllocator : public utilBaseAllocator
   {
      public:
         virtual void *malloc(size_t size) override
         {
            void *buf = nullptr;
            if (size <= STACK_SIZE)
            {
               buf = _statckBuf;
            }
            else
            {
               buf = _pallocator.malloc(size);
            }
            return buf;
         }

         virtual void *realloc(void *p, size_t size) override
         {
            void *buf = nullptr;
            if (p == _statckBuf)
            {
               if (size <= STACK_SIZE)
               {
                  buf = _statckBuf;
               }
               else
               {
                  buf = _pallocator.malloc(size);
                  if (nullptr != buf)
                  {
                     ossMemcpy(buf, _statckBuf, STACK_SIZE);
                  }
               }
            }
            else if (nullptr == p)
            {
               buf = this->malloc(size);
            }
            else
            {
               buf = _pallocator.realloc(p, size);
            }
            return buf;
         }

         virtual void free(void *p) override
         {
            if (p != _statckBuf && nullptr != p)
            {
               _pallocator.free(p);
            }
            return;
         }

         BOOLEAN isStackBuffer(const void *p)
         {
            return _statckBuf == (const CHAR *)p;
         }
      
      private:
         CHAR _statckBuf[STACK_SIZE];
         utilPoolAllocator _pallocator;
   };//
}

#endif // UTIL_ALLOCATOR_HPP_

