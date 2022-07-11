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
         virtual BOOLEAN isMovable()const = 0;
         virtual UINT32 getFastAllocSize()const {return 0;}
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

         virtual BOOLEAN isMovable()const override
         {
            return TRUE;
         }
   };//utilPoolAllocator

   template<UINT32 STACK_SIZE=512>
   class utilStackAllocator : public utilBaseAllocator
   {
      public:
         UINT32 getMaxStackBufSize()const {return STACK_SIZE;}

         virtual UINT32 getFastAllocSize()const override {return STACK_SIZE;}

         virtual void *malloc(size_t size) override
         {
            void *buf = nullptr;
            /// stack buffer is allocated exclusively.
            if (0 == _offset && size <= STACK_SIZE)
            {
               buf = _statckBuf;
               _offset = size;
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
            if (nullptr == p)
            {
               buf = this->malloc(size);
            }
            else if (isStackBuffer(p))
            {
               if (size <= STACK_SIZE)
               {
                  buf = _statckBuf;
                  _offset = size;
               }
               else
               {
                  buf = _pallocator.malloc(size);
                  if (nullptr != buf)
                  {
                     ossMemcpy(buf, _statckBuf, _offset);
                     _offset = 0;
                  }
               }
            }
            else
            {
               buf = _pallocator.realloc(p, size);
            }
            return buf;
         }

         virtual void free(void *p) override
         {
            if (nullptr != p)
            {
               if (isStackBuffer(p))
               {
                  _offset = 0;
               }
               else
               {
                  _pallocator.free(p);
               }
            }

            return;
         }

         BOOLEAN isStackBuffer(const void *p)const
         {
            return _statckBuf == (const CHAR *)p;
         }

         virtual BOOLEAN isMovable()const override
         {
            return TRUE;
         }
      
      private:
         CHAR _statckBuf[STACK_SIZE] = {};
         UINT32 _offset = 0;
         utilPoolAllocator _pallocator;
   };//
}

#endif // UTIL_ALLOCATOR_HPP_

