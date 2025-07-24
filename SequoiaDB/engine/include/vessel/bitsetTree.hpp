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

   Source File Name = bitsetTree .hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BITSET_TREE_H_
#define VESSEL_BITSET_TREE_H_

#include "ossMemPool.hpp"
#include "vessel/fixedBitset.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

#include <cmath>

namespace engine
{
namespace vessel
{
   template<UINT32 NODE_SIZE=512, UINT32 LEVEL_SIZE=3>
   class _bitsetTree  : public SDBObject
   {
      public:
         _bitsetTree();
         ~_bitsetTree();
         _bitsetTree(const _bitsetTree  &) = delete;
         _bitsetTree &operator=(const _bitsetTree  &) = delete;

      private:         
         typedef class fixedBitset<NODE_SIZE> _TREE_NODE;
         typedef class ossPoolVector<_TREE_NODE> _TREE_NODE_VEC;
         typedef class std::array<_TREE_NODE_VEC, LEVEL_SIZE> _TREE;

         static const UINT32 _MAX_BIT_COUNT = OSS_SINT32_MAX;

      public:
         UINT32 getTotalBitCount()const {return _totalBitCount;}
         UINT32 getMaxBitCount()const {return _maxBitCount;}

         BOOLEAN none()const {return _rootDepth < 0 || getRoot().none();}

         void fastRefill(UINT32 bitCount, BOOLEAN v);

         void clear();

         INT32 pushBack(BOOLEAN bit);

         void set(UINT32 pos, BOOLEAN *old=nullptr);
         void setAll();
         void reset(UINT32 pos, BOOLEAN *old=nullptr);
         void resetAll();

         BOOLEAN test(UINT32 pos)const;

         ///return -1 if not found
         INT32 findFirst()const;

      private:
         OSS_INLINE BOOLEAN isFirstBitInL0Node(UINT32 bitPos)const
         {
            return 0 == getPosInNode(bitPos);
         }
         OSS_INLINE UINT32 getNodePos(UINT32 bitPos)const
         {
            return bitPos >> _nodeSizeSquare;
         }
         OSS_INLINE UINT32 getPosInNode(UINT32 bitPos)const
         {
            return bitPos & (NODE_SIZE - 1);
         }
         OSS_INLINE const _TREE_NODE &getRoot()const
         {
            return _tree[_rootDepth].at(0);
         }

      private:
         void extendTreeFromL0();
         void setFromL0(UINT32 pos);
         void resetFromL0(UINT32 pos);

      private:
         UINT32 _nodeSizeSquare = 0;
         UINT32 _maxBitCount = 0;
         UINT32 _totalBitCount = 0;
         INT32 _rootDepth = -1;
         _TREE _tree;
   }; //class _bitsetTree

   typedef class _bitsetTree<512, 3> bitsetTree;

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   _bitsetTree<NODE_SIZE, LEVEL_SIZE>::_bitsetTree()
   {
      static_assert(0 < NODE_SIZE && 0 < LEVEL_SIZE, "can not be zero");
      static_assert(NODE_SIZE <= 512, "can not be too large");
      BOOLEAN r = ossIsPowerOf2(NODE_SIZE, &_nodeSizeSquare);
      SDB_ASSERT(r, "must be power of 2");
      UINT64 size = static_cast<UINT64>(std::pow(NODE_SIZE, LEVEL_SIZE));
      _maxBitCount = std::min(size, (UINT64)OSS_SINT32_MAX);
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   _bitsetTree<NODE_SIZE, LEVEL_SIZE>::~_bitsetTree()
   {
      clear();
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::clear()
   {
      for (INT32 i = 0; i <= _rootDepth; ++i)
      {
         _tree[i].clear();
         _tree[i].shrink_to_fit();
      }

      _totalBitCount = 0;
      _rootDepth = -1;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::fastRefill(UINT32 bitCount, BOOLEAN v)
   {
      clear();
      if (0 == bitCount)
      {
         goto done;
      }
      else if (bitCount <= _maxBitCount)
      {
         UINT32 lvl0Size = ossAlignX(bitCount, NODE_SIZE) / NODE_SIZE;
         _tree[0].resize(lvl0Size);
         UINT32 size = lvl0Size;
         _rootDepth = 0;
         _totalBitCount = bitCount;
         INT32 lvl = 1;

         while (1 < size)
         {
            UINT32 lvlSize = ossAlignX(size, NODE_SIZE) / NODE_SIZE;
            _tree[lvl].resize(lvlSize);
            ++_rootDepth;
            size = lvlSize;
            ++lvl;
         }

         if (v)
         {
            setAll();
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "out of resource");
      }

   done:
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   INT32 _bitsetTree<NODE_SIZE, LEVEL_SIZE>::pushBack(BOOLEAN bit)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(_totalBitCount == _maxBitCount))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      if (isFirstBitInL0Node(_totalBitCount))
      {
         extendTreeFromL0();
      }

      ++_totalBitCount;

      if (bit)
      {
         setFromL0(_totalBitCount - 1);
      }
   done:
      return rc;
   error:
      goto done;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::set(UINT32 pos, BOOLEAN *old)
   {
      SDB_ASSERT(pos < _totalBitCount, "out of bound");
      if (nullptr != old)
      {
         *old = test(pos);
      }

      setFromL0(pos);
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::setAll()
   {
      UINT32 totalBit = _totalBitCount;
      for (INT32 i = 0; i <= _rootDepth; ++i)
      {
         _TREE_NODE_VEC &nodes = _tree[i];
         UINT32 batchCount = totalBit >> _nodeSizeSquare;
         UINT32 tailCount = totalBit & (NODE_SIZE - 1);
         for (UINT32 node = 0; node < batchCount; ++node)
         {
            nodes.at(node).setAll();
         }

         for (UINT32 tail = 0; tail < tailCount; ++tail)
         {
            nodes.back().set(tail);
         }

         totalBit = nodes.size();
      }
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::resetAll()
   {
      for (INT32 i = _rootDepth; 0 <= i; --i)
      {
         _TREE_NODE_VEC &nodes = _tree[i];
         for (typename _TREE_NODE_VEC::iterator itr = nodes.begin();
              itr != nodes.end(); ++itr)
         {
            itr->clearAll();
         }
      }
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::reset(UINT32 pos, BOOLEAN *old)
   {
      SDB_ASSERT(pos < _totalBitCount, "out of bound");
      if (nullptr != old)
      {
         *old = test(pos);
      }

      resetFromL0(pos);
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   BOOLEAN _bitsetTree<NODE_SIZE, LEVEL_SIZE>::test(UINT32 pos)const
   {
      SDB_ASSERT(pos < _totalBitCount, "out of bound");
      UINT32 nodeId = getNodePos(pos);
      UINT32 bit = getPosInNode(pos);
      return _tree[0].at(nodeId).test(bit);
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   INT32 _bitsetTree<NODE_SIZE, LEVEL_SIZE>::findFirst()const
   {
      INT32 bit = -1;

      if (0 <= _rootDepth)
      {
         SDB_ASSERT(_tree[_rootDepth].size() == 1, "must exist");
         INT32 nextLevelBit = getRoot().findFirst();
         if (nextLevelBit < 0)
         {
            goto done;
         }
         
         for (INT32 i = _rootDepth - 1; 0 <= i; --i)
         {
            const _TREE_NODE_VEC &nodes = _tree[i];
            SDB_ASSERT(nextLevelBit < (INT32)nodes.size(), "out of bound");
            INT32 tmp = nodes.at(nextLevelBit).findFirst();
            SDB_ASSERT(0 <= tmp, "must be found");
            nextLevelBit = (nextLevelBit << _nodeSizeSquare) + tmp;
         }

         bit = nextLevelBit;
         SDB_ASSERT(bit < (INT32)_totalBitCount, "impossible");
      }

   done:
      return bit;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::setFromL0(UINT32 pos)
   {
      UINT32 posInCurrentLevel = pos;

      for (INT32 i = 0; i <= _rootDepth; ++i)
      {
         UINT32 nodeId = getNodePos(posInCurrentLevel);
         UINT32 bit = getPosInNode(posInCurrentLevel);
         _TREE_NODE_VEC &nodes = _tree[i];
         if (nodes.size() <= nodeId)
         {
            goto done;
         }
         else
         {
            _TREE_NODE &node = nodes.at(nodeId);
            if (node.test(bit))
            {
               goto done;
            }

            node.set(bit);
            posInCurrentLevel = nodeId;
         }
      }

   done:
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::resetFromL0(UINT32 pos)
   {
      UINT32 posInCurrentLevel = pos;

      for (INT32 i = 0; i <= _rootDepth; ++i)
      {
         UINT32 nodeId = getNodePos(posInCurrentLevel);
         UINT32 bit = getPosInNode(posInCurrentLevel);
         _TREE_NODE_VEC &nodes = _tree[i];
         if (nodes.size() <= nodeId)
         {
            goto done;
         }
         else
         {
            _TREE_NODE &node = nodes.at(nodeId);
            if (!node.test(bit))
            {
               goto done;
            }

            node.clear(bit);

            if (node.any())
            {
               goto done;
            }

            posInCurrentLevel = nodeId;
         }
      }

   done:
      return;
   }

   template<UINT32 NODE_SIZE, UINT32 LEVEL_SIZE>
   void _bitsetTree<NODE_SIZE, LEVEL_SIZE>::extendTreeFromL0()
   {
      _tree[0].resize(_tree[0].size() + 1);
      UINT32 prevLevelNodeCount = _tree[0].size();
      INT32 lvl = 1;

      while (1 < prevLevelNodeCount)
      {
         _TREE_NODE_VEC &nodes = _tree[lvl];
         UINT32 size = ossAlignX(prevLevelNodeCount, NODE_SIZE) / NODE_SIZE;
         if (nodes.size() == size)
         {
            break;
         }

         SDB_ASSERT((nodes.size() + 1) == size, "impossible");
         nodes.resize(size);
         if (1 == nodes.size() &&
             _tree[lvl - 1].at(0).any())
         {
            /// the first node of this level,
            /// need to init the first bit by pre level node.
            nodes.front().set(0);
            
         }
         prevLevelNodeCount = nodes.size();
         ++lvl;
      }

      if (_rootDepth < (lvl - 1))
      {
         _rootDepth = lvl - 1;
      }

      return;
   }
} // namespace vessel

} // namespace engine


#endif//VESSEL_BITSET_TREE_H_