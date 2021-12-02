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

   Source File Name = stackAllocatorRowBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STACK_ALLOCATOR_ROW_BATCH_H_
#define VESSEL_STACK_ALLOCATOR_ROW_BATCH_H_

#include "vessel/rowBatch.h"
#include "../../bson/util/builder.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class stackAllocatorRowBatch : public rowBatch
   {
      public:
         stackAllocatorRowBatch(){}
         virtual ~stackAllocatorRowBatch(){}
      public:
         virtual UINT32 getRowCount()const;
         virtual BOOLEAN isFreeToPush(UINT32 rowSize)const;
         virtual INT32 pushRow(const slice &row);
         virtual INT32 pushRowFragments(std::initializer_list<slice> il);
         virtual slice getRow(UINT32 pos)const;
         virtual void fini();
         virtual void clearRows();

      private:
         /// <offset, size>
         typedef std::pair<UINT32, UINT32> _TAG;
         ossPoolVector<_TAG> _tags;
         bson::StackBufBuilder _builder;

   };//class stackAllocatorRowBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_STACK_ALLOCATOR_ROW_BATCH_H_