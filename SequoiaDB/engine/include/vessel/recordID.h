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

   Source File Name = recordID.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RECORD_ID_H_
#define VESSEL_RECORD_ID_H_

#include "vessel/pageIdentifier.h"
#include "dms.hpp"
#include "xxHashInc.h"
#include "ossMemPool.hpp"
#include "pdTrace.hpp"
#include "../bson/util/builder.h"

namespace engine
{
namespace vessel
{
   typedef INT16 RECORD_SLOT_POS;
   constexpr RECORD_SLOT_POS INVALID_RECORD_SLOT_POS = (RECORD_SLOT_POS)-1;

   OSS_INLINE BOOLEAN isValidRecordSlotPosition(RECORD_SLOT_POS pos)
   {
      return 0 <= pos;
   }

#pragma pack(2)
   class recordID
   {
      public:
         OSS_INLINE recordID(){}
         OSS_INLINE ~recordID(){}
         OSS_INLINE explicit recordID(PAGE_ID pid, RECORD_SLOT_POS pos)
         :_pid(pid), _pos(pos){}

         OSS_INLINE recordID(const recordID &rid)
         :_pid(rid._pid),
          _pos(rid._pos)
         {
         }

         OSS_INLINE BOOLEAN operator==(const recordID &o)const
         {
            return _pid == o._pid && _pos == o._pos;
         }

         OSS_INLINE BOOLEAN operator!=(const recordID &o)const
         {
            return _pid != o._pid || _pos != o._pos;
         }

         OSS_INLINE recordID &operator=(const recordID &rid)
         {
            _pid = rid._pid;
            _pos = rid._pos;
            return *this;
         }

         OSS_INLINE BOOLEAN operator<(const recordID &r)const
         {
            return compare(r) < 0;
         }

         OSS_INLINE BOOLEAN operator<=(const recordID &r)const
         {
            return compare(r) <= 0;
         }

         OSS_INLINE INT32 compare(const recordID &rid)const
         {
            if (_pid < rid._pid)
            {
               return -1;     
            }
            else if(_pid > rid._pid)
            {
               return 1;
            }
            else if (_pos < rid._pos)
            {
               return -1;
            }
            else if (_pos > rid._pos)
            {
               return 1;
            }
            else
            {
               return 0;
            }
         }

         OSS_INLINE void setPid(PAGE_ID pid)
         {
            _pid = pid;
         }

         OSS_INLINE PAGE_ID getPid()const
         {
            return _pid;
         }

         OSS_INLINE void setPos(RECORD_SLOT_POS pos)
         {
            _pos = pos;
         }

         OSS_INLINE RECORD_SLOT_POS getPos()const
         {
            return _pos;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != _pid &&
                   isValidRecordSlotPosition(_pos);
         }

         OSS_INLINE dmsRecordID toDMSRid()const
         {
            return isValid() ? dmsRecordID(_pid, _pos) : dmsRecordID();
         }

         OSS_INLINE void resetByDmsRid(const dmsRecordID &rid)
         {
            reset();
            if (rid.isValid() && (rid._offset <= (INT32)OSS_SINT16_MAX))
            {
               _pid = rid._extent;
               _pos = rid._offset;
            }
            return;
         }

         OSS_INLINE void reset()
         {
            _pid = INVALID_PAGE_ID;
            _pos = INVALID_RECORD_SLOT_POS;
         }

         OSS_INLINE UINT32 hash()const
         {
            UINT64 v = _pid;
            v <<= 32;
            v |= _pos;
            return XXH3_64bits(&v, sizeof(v));
         }

         ossPoolString toString()const
         {
            bson::StringBuilder str(32);
            str << '[' << _pid << ',' << _pos << ']';
            return std::move(str.poolStr());
         }

         OSS_INLINE BOOLEAN isMinRid()const
         {
            return createMinRid() == *this;
         }

         OSS_INLINE BOOLEAN isMaxRid()const
         {
            return createMaxRid() == *this;
         }

         static recordID createMinRid()
         {
            return recordID(0, OSS_SINT16_MIN);
         }
         static recordID createMaxRid()
         {
            return recordID((UINT32)-1, OSS_SINT16_MAX);
         }

      private:
         PAGE_ID _pid = INVALID_PAGE_ID;
         RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
   }; /// end of class recordID
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_RECORD_ID_H_