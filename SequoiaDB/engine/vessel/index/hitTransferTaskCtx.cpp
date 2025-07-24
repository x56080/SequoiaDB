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

   Source File Name = hitTransferTaskCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/hitTransferTaskCtx.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   hitTransferTaskCtx::hitTransferTaskCtx(const hitIndexTransferTask &task):
   _task(task)
   {
      SDB_ASSERT(_task.isValid(), "can not be invalid");
   }

   void hitTransferTaskCtx::reset()
   {
      _task.reset();
      _batch = nullptr;
      _done = FALSE;
      _rc = SDB_OK;
      _insertedEntryNum = 0;
      _removedEntryNum = 0;
      _btreeEntryPageUnstable = FALSE;
   }

   void hitTransferTaskCtx::resetToRedo()
   {
      _batch = nullptr;
      _done = FALSE;
      _rc = SDB_OK;
      _insertedEntryNum = 0;
      _removedEntryNum = 0;
      _btreeEntryPageUnstable = FALSE;
   }
} // namespace vessel

} // namespace engine
