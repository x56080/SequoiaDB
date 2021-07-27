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
}//namespace vessel
}//namespace engine
