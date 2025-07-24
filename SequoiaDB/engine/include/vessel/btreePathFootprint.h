/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = btreePathFootprint.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         btreePathFootprint() = default;
         ~btreePathFootprint() = default;
         btreePathFootprint(RECORD_SLOT_POS pos, BOOLEAN isUpperBound):
         _pos(pos)
         {
            if (isUpperBound)
            {
               setUpperBound();
            }
         }

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
         OSS_INLINE UINT16 getFlags()const {return _flags;}
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

         OSS_INLINE UINT32 encode()const
         {
            UINT32 c = _flags;
            c <<= 16;
            c |= _pos;
            return c;
         }

         OSS_INLINE void decodeFrom(UINT32 c)
         {
            _pos = (RECORD_SLOT_POS)c;
            _flags = c >> 16;
            return;
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