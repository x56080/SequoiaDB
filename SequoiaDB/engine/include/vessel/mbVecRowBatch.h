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

   Source File Name = mbVecRowBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_MB_VEC_ROW_BATCH_H_
#define VESSEL_MB_VEC_ROW_BATCH_H_

#include "vessel/rowBatch.h"
#include "vessel/memoryBlock.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class mbVecRowBatch : public rowBatch
   {
      public:
         mbVecRowBatch(){}
         virtual ~mbVecRowBatch(){}
      public:
         virtual void fini();
         virtual void clearRows();

      protected:
         virtual INT32 appendRowFragments(std::initializer_list<slice> il);

         virtual INT32 appendRow(const slice &row);

         virtual slice getRowByOffset(UINT32 offset, UINT32 size)const;
         virtual slice getRowByPos(UINT32 pos)const;

      private:
         virtual BOOLEAN searchByOffset()const {return FALSE;}
         virtual BOOLEAN searchByPos()const {return TRUE;}

      private:
         ossPoolVector<memoryBlock> _mbs;         
   };//class mbVecRowBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_MB_ROW_BATCH_H_