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

   Source File Name = btreeStatistics.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_STATISTICS_H_
#define VESSEL_BTREE_STATISTICS_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   struct btreeStatistics
   {
      void reset()
      {
         nonleafNodeNum = 0;
         leafNodeNum = 0;
         totalEntryNum = 0;
         origTotalEntrySize = 0;
         realTotalEntrySize = 0;

         totalEntryInserted = 0;
         totalEntryRemoved = 0;
         nodesAllocated = 0;
         nodesDestroyed = 0;
         newRootCreatedNum = 0;
         childNodesRefilled = 0;
         return;
      }

      OSS_INLINE UINT32 getTotalNodeNum()const
      {
         return nonleafNodeNum + leafNodeNum;
      }

      ///real time
      UINT32 nonleafNodeNum = 0;
      UINT32 leafNodeNum = 0;
      UINT64 totalEntryNum = 0;
      UINT64 origTotalEntrySize = 0;
      UINT64 realTotalEntrySize = 0;

      /// history
      UINT64 totalEntryInserted = 0;
      UINT64 totalEntryRemoved = 0;
      UINT64 nodesAllocated = 0;
      UINT64 nodesDestroyed = 0;
      UINT32 newRootCreatedNum = 0;
      UINT64 childNodesRefilled = 0;
   };//struct btreeStatistics
} // namespace vesel

} // namespace engine



#endif//VESSEL_BTREE_STATISTICS_H_

