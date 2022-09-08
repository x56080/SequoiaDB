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
   class btreeNodeSeekResult : public SDBObject
   {
      friend class btreeNodeBase;
      public:
         btreeNodeSeekResult() = default;
         ~btreeNodeSeekResult() = default;
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidRecordSlotPosition(_slotPos);
         }
         OSS_INLINE void reset()
         {
            _res = FALSE;
            _child = INVALID_PAGE_ID;
            _slotPos = INVALID_RECORD_SLOT_POS;
            _isUpperBound = FALSE;
            _isMarkedDeleted = FALSE;
            return;
         }
         OSS_INLINE BOOLEAN isIdentical() const
         {
            return 0 == _res && !_isUpperBound;
         }
         OSS_INLINE BOOLEAN getChild()const {return _child;}
         OSS_INLINE BOOLEAN hasChild()const {return INVALID_PAGE_ID != _child;}
         OSS_INLINE BOOLEAN isMaredDeleted()const {return _isMarkedDeleted;}
         OSS_INLINE RECORD_SLOT_POS getPos()const {return _slotPos;}
         OSS_INLINE INT32 getCmpRes()const {return _res;}
         OSS_INLINE BOOLEAN isUpperBound()const {return _isUpperBound;}

      private:
         INT32 _res = 0;
         PAGE_ID _child = INVALID_PAGE_ID;
         BOOLEAN _isUpperBound = FALSE;
         BOOLEAN _isMarkedDeleted = FALSE;
         RECORD_SLOT_POS _slotPos = INVALID_RECORD_SLOT_POS;
   };//class btreeNodeSeekResult

   
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_NODE_SEEK_RESULT_H_
