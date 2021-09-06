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

   Source File Name = btreeAccessPath.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeAccessPath.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   btreeAccessPath::btreeAccessPath()
   {}

   btreeAccessPath::~btreeAccessPath()
   {

   }

   void btreeAccessPath::clear()
   {
      _size = 0;
      _dynamicNodes.clear();
      return;
   }


   void btreeAccessPath::push(PAGE_ID lpid, UINT32 splitedTimes)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
#if defined (_DEBUG)
      for (UINT32 i = 0; i < _size; ++i)
      {
         const _node &n = get(i);
         SDB_ASSERT(lpid != n.getLpid(), "duplicated lpid");
      }
#endif//_DEBUG

      if (_size < _DEFAULT_CAPACITY)
      {
         _staticNodes[_size] = _node(lpid, splitedTimes);
      }
      else
      {
         _dynamicNodes.push_back(_node(lpid, splitedTimes));
      }

      ++_size;
      return;
   }

   void btreeAccessPath::pop()
   {
      if (0 < _size)
      {
         if (_DEFAULT_CAPACITY < _size)
         {
            _dynamicNodes.pop_back();
         }

         --_size;
      }
      return;
   }

   btreeAccessPath::pathNode btreeAccessPath::operator[](UINT32 pos)const
   {
      if (OSS_LIKELY(pos < _size))
      {
         const _node &node = get(pos);
         return pathNode(pos, node.getLpid(), node.getSplitedTimes());
      }
      else
      {
         return pathNode();
      }
   }

   const btreeAccessPath::_node &btreeAccessPath::get(UINT32 pos)const
   {
      SDB_ASSERT(pos < _size, "out of bound");
      return (pos < _DEFAULT_CAPACITY) ? 
              _staticNodes[pos] : _dynamicNodes.at(pos - _DEFAULT_CAPACITY);
   }

   btreeAccessPath::pathNode btreeAccessPath::getCurrentNode()const
   {
      if (OSS_LIKELY(!isEmpty()))
      {
         const _node &node = get(_size - 1);
         return pathNode(_size - 1, node.getLpid(), node.getSplitedTimes());
      }
      else
      {
         return pathNode();
      }
   }

   btreeAccessPath::pathNode btreeAccessPath::getFatherOfCurrentNode()const
   {
      if (OSS_LIKELY(1 < _size))
      {
         const _node &node = get(_size - 2);
         return pathNode(_size - 2, node.getLpid(), node.getSplitedTimes());
      }
      else
      {
         return pathNode();
      }
   }

   btreeAccessPath::pathNode btreeAccessPath::getFatherNode(UINT32 depth)const
   {
      SDB_ASSERT(0 < depth, "root has no father node");
      SDB_ASSERT(depth < _size, "out of bound");
      const _node &node = get(depth - 1);
      return pathNode(depth - 1, node.getLpid(), node.getSplitedTimes());
   }
} // namespace vessel

} // namespace engine
