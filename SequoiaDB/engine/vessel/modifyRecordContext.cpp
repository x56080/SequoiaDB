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
   void modifyRecordContext::setTargetRecord(const slice &record)
   {
      SDB_ASSERT(record.isValid(), "can not be invalid");
      _recordData = record;
   }
   slice modifyRecordContext::getTargetRecord()const
   {
      return _recordData;
   }

   void modifyRecordContext::setOverflowAddr(const recordID &addr)
   {
      SDB_ASSERT(addr.isValid(), "can not be invalid");
      _overflowAddr = addr;
      return;
   }

   void modifyRecordContext::setAsBigRecord()
   {
      _bigRecord = TRUE;
   }

   void modifyRecordContext::clear()
   {
      _recordData.reset();
      _transID = DPS_TRANS_ID();
      _bigRecord = FALSE;
      _overflowAddr = recordID();
   }

} // namespace vessel

} // namespace engine
