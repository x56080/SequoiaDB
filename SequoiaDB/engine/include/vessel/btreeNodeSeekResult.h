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

   Source File Name = btreeNodeSeekResult.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
