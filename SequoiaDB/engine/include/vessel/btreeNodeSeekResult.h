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

   Source File Name = btreeNodeSeekResult.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_NODE_SEEK_RESULT_H_
#define VESSEL_BTREE_NODE_SEEK_RESULT_H_

#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   struct btreeNodeSeekResult : public SDBObject
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return isValidRecordSlotPosition(slotPos);
      }
      OSS_INLINE void reset()
      {
         res = FALSE;
         child = INVALID_PAGE_ID;
         slotPos = INVALID_RECORD_SLOT_POS;
         isUpperBound = FALSE;
         return;
      }
      OSS_INLINE BOOLEAN isIdentical() const
      {
         return 0 == res && !isUpperBound;
      }
      OSS_INLINE BOOLEAN hasChild()const {return INVALID_PAGE_ID != child;}

      INT32 res = 0;
      PAGE_ID child = INVALID_PAGE_ID;
      RECORD_SLOT_POS slotPos = INVALID_RECORD_SLOT_POS;
      BOOLEAN isUpperBound = FALSE;
   };//class btreeNodeSeekResult

   
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_SEEK_RESULT_H_
