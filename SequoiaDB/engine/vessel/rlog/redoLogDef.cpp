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

   Source File Name = redoLogDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/redoLogDef.h"
#include "dpsDef.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{
   void redoLogFileHeader::init(UINT32 fileSize, UINT32 logicalId, UINT64 startLSN)
   {
      ossMemset(this, 0x0, RLOG_FILE_HEAD_SIZE);
      ossMemcpy(eyeCatcher, EYE_CATCHER_STR, sizeof(eyeCatcher));
      this->version = CURRENT_VERSION;
      this->fileSize = fileSize;
      this->logicalId = logicalId;
      this->startLSN = startLSN;
   }

   BOOLEAN redoLogFileHeader::validate() const
   {
      if (0 != ossMemcmp(this->eyeCatcher, EYE_CATCHER_STR, sizeof(eyeCatcher)))
      {
         return FALSE;
      }
      else if (CURRENT_VERSION != this->version)
      {
         return FALSE;
      }
      else if (this->fileSize != RLOG_FILE_SIZE)
      {
         return FALSE;
      }
      else if (DPS_INVALID_LSN_OFFSET == this->startLSN)
      {
         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }
} // namespace vessel

} // namespace engine
