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

   Source File Name = updateContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/modifyRecordContext.h"

namespace engine
{
namespace vessel
{
   void modifyRecordContext::modifyDone()
   {
      dmlContext::clearDmlHistroy();
      _target._recordBuffer.release();
      _target._transID = DPS_TRANS_ID();
      _target._overflow = FALSE;
      _target._bigRecord = FALSE;
      _target._overflowAddr = recordID();
   }

   slice modifyRecordContext::getTargetRecord()const
   {
      SDB_ASSERT(!_target._recordBuffer.isEmpty(), "can not be empty");
      return slice(_target._recordBuffer.getSize(),
                   _target._recordBuffer.getBuffer());
   }

   void modifyRecordContext::setOverflowInfo(BOOLEAN isBigRecord,
                                             const recordID &addr)
   {
      SDB_ASSERT(addr.isValid(), "can not be invalid");
      _target._overflow = TRUE;
      _target._bigRecord = isBigRecord;
      _target._overflowAddr = addr;
      return;
   }

   void modifyRecordContext::adoptRecordBuffer(memoryBlock &mb)
   {
      SDB_ASSERT(!mb.isEmpty(), "can not be invalid");
      _target._recordBuffer = std::move(mb);
   }
} // namespace vessel

} // namespace engine
