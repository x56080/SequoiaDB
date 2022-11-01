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

   Source File Name = utilFragAllocator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_FRAG_ALLOCATOR_HPP_
#define UTIL_FRAG_ALLOCATOR_HPP_

#include "utilAllocator.hpp"
#include "utilEmbeddedList.hpp"

namespace engine
{

#pragma pack(4)
   class _utilFragAllocator : public utilBaseAllocator
   {
      public:
         static constexpr UINT32 MAX_BLOCK_SIZE = 1 << 30;

         struct options
         {
            UINT32 defaultBlockSize = 512;

            /// allocate block from exlusive block list
            /// if block size >= (defaultBlockSize >> exclusiveBlockBound)
            UINT32 exclusiveBlockBound = 1;
         };

      public:
         _utilFragAllocator() = default;
         explicit _utilFragAllocator(const options &o);
         virtual ~_utilFragAllocator() = default;
         _utilFragAllocator(const _utilFragAllocator &) = delete;
         _utilFragAllocator &operator=(const _utilFragAllocator &) = delete;

      public:
         void reset();
         void resetBlocks();
         void setOptions(const options &o)
         {
            _o = o;
         }
         BOOLEAN isEmpty()const
         {
            return _rep.isEmpty();
         }

      public:
         virtual void *malloc(size_t size) override;
         virtual void free(void *p) override;
         virtual void *realloc(void *p, size_t size) override;
         virtual BOOLEAN isMovable()const override;
         virtual BOOLEAN isMovable(const void *p)const override;

      public:
         void *allocateExclusiveBlock(UINT32 size);

      private:
         struct memBlockNode
         {
            memBlockNode() = default;

            ///WARNING: do not ever reinit node after it pushed into list. 
            OSS_INLINE void init()
            {
               node.reset();
               blockSize = 0;
               offset = 0;
               refCount = 0;
            }
            OSS_INLINE UINT32 getFreeSize()const
            {
               return blockSize - offset;
            }

            UTIL_EMBEDDED_LIST_NODE<memBlockNode> node;
            UINT32 blockSize = 0;
            UINT32 offset = 0;
            UINT32 refCount = 0;
         };
         static constexpr UINT32 MB_NODE_SIZE = sizeof(memBlockNode);

         struct mbNodeRef
         {
            UTIL_EMBEDDED_LIST_NODE<memBlockNode> *operator()(memBlockNode *o)const
            {
               return &(o->node);
            }
         };

         using MB_NODE_REF = mbNodeRef;
         using MB_NODE_DELETER = utilEmListPoolBlockDeleter;

         using MB_NODE_LIST = UTIL_EMBEDDED_LIST<memBlockNode, MB_NODE_REF, MB_NODE_DELETER>;

      public:
         class repertory
         {
            friend class _utilFragAllocator;
            public:
               repertory() = default;
               ~repertory()
               {
                  reset();
               }
               repertory(const repertory &) = delete;
               repertory &operator=(const repertory &) = delete;
               repertory(repertory &&o):
               _sl(std::move(o._sl)),
               _el(std::move(o._el))
               {}
               repertory &operator=(repertory &&o)
               {
                  _sl = std::move(o._sl);
                  _el = std::move(o._el);
                  return *this;
               }


            public:
               OSS_INLINE void reset()
               {
                  _sl.clear();
                  _el.clear();
               }
               OSS_INLINE BOOLEAN isEmpty()const
               {
                  return _sl.isEmpty() && _el.isEmpty();
               }

            private:
               MB_NODE_LIST _sl;
               MB_NODE_LIST _el;
         };//class repertory

          
         repertory reap();         

      private:
         void *_allocateExclusiveBlock(size_t size);
         void *_allocateFromSharedList(UINT32 size);
         CHAR *_getMemBlock(memBlockNode *node);

         memBlockNode *_allocateExclusiveNode(size_t blockSize);
         memBlockNode *_allocateSharedNode();
         CHAR *_allocateFromSharedNode(memBlockNode *node, UINT32 size);

         OSS_INLINE BOOLEAN _isExclusiveBlock(size_t size)const
         {
            return (_o.defaultBlockSize >> _o.exclusiveBlockBound) <= size;
         }

         BOOLEAN _findAndFree(MB_NODE_LIST &l, void *p);
         BOOLEAN _isBufferBelongToNode(const void *p, const memBlockNode *node)const;

      private:
         options _o;
         repertory _rep;
   };//class _utilFragAllocator
   using utilFragAllocator = _utilFragAllocator;

#pragma pack()
} // namespace engine


#endif//UTIL_FRAG_ALLOCATOR_HPP_