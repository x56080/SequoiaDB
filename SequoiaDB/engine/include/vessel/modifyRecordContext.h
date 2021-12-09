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

   Source File Name = modifyRecordContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSE_MODIFY_RECORD_CONTEXT_H_
#define VESSE_MODIFY_RECORD_CONTEXT_H_

#include "vessel/memoryBlock.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class modifyRecordContext : public SDBObject
   {
      public:
         modifyRecordContext(){}
         ~modifyRecordContext(){}
         modifyRecordContext(const modifyRecordContext &) = delete;
         modifyRecordContext &operator=(const modifyRecordContext &) = delete;

      public:
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         
         OSS_INLINE BOOLEAN isOverflow()const
         {
            return _overflow;
         }
         OSS_INLINE BOOLEAN isBigRecord()const
         {
            return _bigRecord;
         }

         OSS_INLINE const recordID &getOverflowAddr()const
         {
            return _overflowAddr;
         }
         OSS_INLINE void setRid(const recordID &rid)
         {
            _rid = rid;
         }
         OSS_INLINE memoryBlock &getRecordBuffer()
         {
            return _recordBuffer;
         }

         slice getTargetRecord()const;

         void setOverflowInfo(BOOLEAN isBigRecord,
                              const recordID &addr);

         void clearData();

         void setTransID(const DPS_TRANS_ID &transID)
         {
            _transID = transID;
         }

      private:
         recordID _rid;
         memoryBlock _recordBuffer;
         DPS_TRANS_ID _transID;
         BOOLEAN _overflow = FALSE;
         BOOLEAN _bigRecord = FALSE;
         recordID _overflowAddr;
         
   };//class modifyRecordContext
} // namespace vessel

} // namespace engine


#endif//VESSE_MODIFY_RECORD_CONTEXT_H_