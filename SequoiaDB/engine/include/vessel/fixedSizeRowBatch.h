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

   Source File Name = fixedSizeBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FIXED_SIZE_ROW_BATCH_H_
#define VESSEL_FIXED_SIZE_ROW_BATCH_H_

#include "vessel/rowBatch.h"

namespace engine
{
namespace vessel
{
   class fixedSizeRowBatch : public rowBatch
   {
      public:
         fixedSizeRowBatch(){}
         virtual ~fixedSizeRowBatch(){}

      public:
         virtual UINT32 getRowCount()const;
         virtual BOOLEAN isFreeToPush(UINT32 rowSize)const;
         virtual INT32 pushRow(const slice &row);
         virtual INT32 pushRowFragments(std::initializer_list<slice> il);
         virtual slice getRow(UINT32 pos)const;
         virtual void fini();
         virtual void clearRows();

      private:
         UINT32 _bufferSize = 0;
         CHAR *_buffer = NULL;
         UINT32 _rowCount = 0;
         UINT32 _backOffset = 0;
   };//class fixedSizeRowBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_SIZE_ROW_BATCH_H_
