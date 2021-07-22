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

   Source File Name = recordData.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RECORD_DATA_H_
#define VESSEL_RECORD_DATA_H_

#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   const static UINT32 RECORD_DATA_TYPE_BSON = 0x0;

   class recordData : public SDBObject
   {
      public:
         OSS_INLINE recordData():
         _type(RECORD_DATA_TYPE_BSON)
         {}

         OSS_INLINE ~recordData(){}

         OSS_INLINE recordData(UINT32 type, const slice &s):
         _type(type),
         _slice(s){}

         OSS_INLINE recordData(const recordData &o):
         _type(o._type),
         _slice(o._slice){}

         OSS_INLINE recordData &operator=(const recordData &o)
         {
            _type = o._type;
            _slice = o._slice;
            return *this;
         }

      public:
         OSS_INLINE UINT32 getType()const
         {
            return _type;
         }
         OSS_INLINE const slice &getSlice()const
         {
            return _slice;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return _slice.valid();
         }
         OSS_INLINE void reset()
         {
            _type = RECORD_DATA_TYPE_BSON;
            _slice.reset();
            return;
         }
         OSS_INLINE void reset(UINT32 type, const slice &s)
         {
            _type = type;
            _slice = s;
            return;
         }

      private:
         UINT32 _type;
         slice _slice;

   };//class recordData
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_DATA_H_