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

#include "vessel/dmlContext.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class modifyRecordContext : public dmlContext
   {
      public:
         modifyRecordContext(){}
         virtual ~modifyRecordContext(){}

      private:
         struct _targetInfo
         {
            memoryBlock _recordBuffer;
            DPS_TRANS_ID _transID;
            BOOLEAN _overflow = FALSE;
            BOOLEAN _bigRecord = FALSE;
            recordID _overflowAddr;
         };//struct _target

      public:
         OSS_INLINE BOOLEAN isOverflow()const
         {
            return _target._overflow;
         }
         OSS_INLINE BOOLEAN isBigRecord()const
         {
            return _target._bigRecord;
         }

         OSS_INLINE const recordID &getOverflowAddr()const
         {
            return _target._overflowAddr;
         }

         void modifyDone();

         slice getTargetRecord()const;

         /// mb will no longer be valid
         void adoptRecordBuffer(memoryBlock &mb);

         void setOverflowInfo(BOOLEAN isBigRecord,
                              const recordID &addr);

      private:
         _targetInfo _target;
         
   };//class modifyRecordContext
} // namespace vessel

} // namespace engine


#endif//VESSE_MODIFY_RECORD_CONTEXT_H_