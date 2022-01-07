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

   Source File Name = rowBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ROW_BATCH_H_
#define VESSEL_ROW_BATCH_H_

#include "vessel/slice.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class rowBatch : public SDBObject
   {
      public:
         rowBatch(){}
         virtual ~rowBatch(){}
         rowBatch(const rowBatch &) = delete;
         rowBatch &operator=(const rowBatch &) = delete;

      public:
         virtual UINT32 getRowCount()const = 0;
         virtual BOOLEAN isFreeToPush(UINT32 rowSize)const = 0;
         virtual INT32 pushRow(const slice &row) = 0;
         virtual INT32 pushRowFragments(std::initializer_list<slice> il) = 0;
         virtual slice getRow(UINT32 pos)const = 0;
         virtual void fini() = 0;
         virtual void clearRows() = 0;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == getRowCount();
         }
         slice operator[](UINT32 pos)const {return getRow(pos);}
         

      public:
         OSS_INLINE void setRowLimit(UINT32 v)
         {
            _rowLimit = v;
         }
         OSS_INLINE BOOLEAN hasRowLimit()const
         {
            return 0 < _rowLimit;
         }
         OSS_INLINE void setBufferSizeLimit(UINT32 v)
         {
            _bufferSizeLimit = v;
         }
         OSS_INLINE BOOLEAN hasBufferSizeLimit()const
         {
            return 0 < _bufferSizeLimit;
         }
      protected:
         UINT32 _rowLimit = 0;
         UINT32 _bufferSizeLimit = 0;
   };//class rowBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_ROW_BATCH_H_