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

   Source File Name = deltaLogRecord.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogRecord.h"
#include "xxHashInc.h"

namespace engine
{
namespace vessel
{
   deltaLogRecord::deltaLogRecord()
   {}

   deltaLogRecord::~deltaLogRecord()
   {}

   void deltaLogRecord::reset(const void * ptr)
   {
      _head = (const deltaLogRecordHead *)ptr;
      return;
   }

   const void *deltaLogRecord::getRecordBodyPtr(UINT32 offset, UINT32 len)const
   {
      SDB_ASSERT(isValid(), "must be valid");
      const void *ptr = NULL;
      if (OSS_UNLIKELY(!isValid()))
      {
         goto done;
      }
      
      if (OSS_LIKELY((DELTA_LOG_RECORD_HEAD_SIZE + offset + len) <= _head->_size))
      {
         ptr = (const void *)((ossValuePtr)_head + DELTA_LOG_RECORD_HEAD_SIZE + offset);
      }
   done:
      return ptr;
   }

   INT32 deltaLogRecord::readAsMappingRecord(const UINT32 **count,
                                             const UINT32 **lpids,
                                             const UINT32 **pids)const
   {
      INT32 rc = SDB_OK;
      const void *ptr = NULL;
      UINT32 mappingCount = 0;
      UINT32 offset = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(DELTA_LOG_TYPE_MAPPING != getLogHead()->_type))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      
      ptr = getRecordBodyPtr(offset, sizeof(UINT32));
      if (OSS_UNLIKELY(NULL == ptr))
      {
         PD_LOG(PDERROR, "failed to get body ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      mappingCount = *((UINT32 *)ptr);
      if (OSS_UNLIKELY(0 == mappingCount))
      {
         PD_LOG(PDERROR, "mapping count is zero");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine
