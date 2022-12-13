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

   Source File Name = redoLogDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
