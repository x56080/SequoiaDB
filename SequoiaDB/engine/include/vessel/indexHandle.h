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

   Source File Name = indexHandle.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_HANDLE_H_
#define VESSEL_INDEX_HANDLE_H_

#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
   class indexHandle : public SDBObject
   {
      public:
         indexHandle(){}
         explicit indexHandle(INT32 indexSlot, UINT32 indexId):
         _indexSlot(indexSlot),
         _indexId(indexId){}
         ~indexHandle(){}
         indexHandle(const indexHandle &o):
         _indexSlot(o._indexSlot),
         _indexId(o._indexId){}
         indexHandle &operator=(const indexHandle &o)
         {
            _indexSlot = o._indexSlot;
            _indexId = o._indexId;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_LOGICAL_INDEX_ID != _indexId && isValidIndexSlot(_indexSlot);
         }
         OSS_INLINE INT32 getIndexSlot()const
         {
            return _indexSlot;
         }
         OSS_INLINE UINT32 getIndexId()const
         {
            return _indexId;
         }

      private:
         INT32 _indexSlot = -1;
         UINT32 _indexId = INVALID_LOGICAL_INDEX_ID;
   };//class indexHandle
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_HANDLE_H_
