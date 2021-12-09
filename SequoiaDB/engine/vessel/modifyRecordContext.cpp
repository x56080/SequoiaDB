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
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   slice modifyRecordContext::getTargetRecord()const
   {
      SDB_ASSERT(!_recordBuffer.isEmpty(), "can not be empty");
      return slice(_recordBuffer.getSize(),
                   _recordBuffer.getBuffer());
   }

   void modifyRecordContext::setOverflowInfo(BOOLEAN isBigRecord,
                                             const recordID &addr)
   {
      SDB_ASSERT(addr.isValid(), "can not be invalid");
      _overflow = TRUE;
      _bigRecord = isBigRecord;
      _overflowAddr = addr;
      return;
   }

   void modifyRecordContext::clearData()
   {
      _recordBuffer.resize(0);
      _transID = DPS_TRANS_ID();
      _overflow = FALSE;
      _bigRecord = FALSE;
      _overflowAddr = recordID();
   }

} // namespace vessel

} // namespace engine
