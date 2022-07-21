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

   Source File Name = btreePathFootprint.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_PATH_FOOTPRINT_H_
#define VESSEL_BTREE_PATH_FOOTPRINT_H_

#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class btreePathFootprint : public SDBObject
   {
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidRecordSlotPosition(_pos);
         }
         OSS_INLINE void setPos(RECORD_SLOT_POS pos)
         {
            _pos = pos;
            return;
         }
         OSS_INLINE RECORD_SLOT_POS getPos()const
         {
            return _pos;
         }
         OSS_INLINE void setUpperBound()
         {
            OSS_BIT_SET(_flags, FLAG_IS_UPPER_BOUND);
         }
         OSS_INLINE void setUpperBound(BOOLEAN v)
         {
            if (v)
            {
               OSS_BIT_SET(_flags, FLAG_IS_UPPER_BOUND);
            }
            else
            {
               OSS_BIT_CLEAR(_flags, FLAG_IS_UPPER_BOUND);
            }
            return;
         }
         OSS_INLINE BOOLEAN isUpperBound()const
         {
            return 0 != OSS_BIT_TEST(_flags, FLAG_IS_UPPER_BOUND);
         }

         OSS_INLINE UINT32 encoding()const
         {
            UINT32 c = _flags;
            c <<= 16;
            c |= _pos;
            return c;
         }

      private:
         static const UINT16 FLAG_IS_UPPER_BOUND = 0x01;

      private:
         RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
         UINT16 _flags = 0;
   };//class btreePathFootprint
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_PATH_FOOTPRINT_H_