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

   Source File Name = updateContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
