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
         OSS_INLINE UINT32 getBatchSize()const{return _rows.size();}

      public:
         void init(INT32 rowLimit = -1);
         void fini();
         void resetData();
         BOOLEAN isFreeToPush(UINT32 size)const;

      private:
         /// <offset, size>
         typedef std::pair<UINT32, UINT32> _ROW_TAG;

         virtual INT32 writeBuffer(UINT32 offset, const slice &data) = 0;
         virtual void clearBuffer() = 0;
         virtual slice getFromBuffer(const _ROW_TAG &rt)const = 0;
         virtual void _fini(){}

      private:
         ossPoolVector<_ROW_TAG> _rows;
         INT32 _rowLimit = -1;
         UINT32 _bufferCapacity = 0;
         UINT32 _size = 0;
   };//class rowBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_ROW_BATCH_H_