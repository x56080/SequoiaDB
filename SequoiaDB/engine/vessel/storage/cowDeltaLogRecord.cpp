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

   Source File Name = cowDeltaLogRecord.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/cowDeltaLogRecord.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN cowDeltaLogRecord::setAsIdxMSmpAllocate(PAGE_ID pid, UINT32 count)
   {
      BOOLEAN r = FALSE;
      reset();
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid || 0 == count))
      {
         goto done;
      }
      _version = DELTA_LOG_RECORD_VERSION;
      _type = DELTA_LOG_TYPE_IDX_M_SMP_ALLOCATE;
      _pid = pid;
      _pad0 = count;
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN cowDeltaLogRecord::getAsIdxMSmpAllocate(PAGE_ID *pid, UINT32 *count)const
   {
      BOOLEAN r = FALSE;
      if (OSS_UNLIKELY(!isValid()))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(DELTA_LOG_TYPE_IDX_M_SMP_ALLOCATE != _type))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == _pid ||
                            0 == _pad0))
      {
         goto done;
      }

      if (NULL != pid)
      {
         *pid = _pid;
      }
      if (NULL != count)
      {
         *count = _pad0;
      }
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine