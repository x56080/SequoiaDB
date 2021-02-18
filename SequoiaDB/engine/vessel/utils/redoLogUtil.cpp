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

   Source File Name = redoLogUtil.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/redoLogUtil.h"
#include "dpsLogRecord.hpp"
#include "ossLikely.hpp"
#include "dpsLogRecordDef.hpp"
#include "vessel/vesselOptions.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/logRecordContext.h"
#include "vessel/IRedoLogger.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   INT32 initCreateCSLogRecord(const CHAR *name,
                               const SPACE_ID *sid,
                               const createCSOptions *options,
                               dpsLogRecord &lr)
   {
      INT32 rc = SDB_OK;
      dpsLogRecordHeader *head = NULL;
      if (OSS_UNLIKELY(NULL == name || NULL == options))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lr.clear();
      head = &(lr.head());
      head->_type = LOG_TYPE_CS_CRT;
      OSS_BIT_SET(head->_flags, DPS_VESSEL_LOG_FLAG_FROM_VESSEL);
      rc = lr.push(DPS_LOG_CSCRT_CSNAME, ossStrlen(name)+1, name);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push csname to log record:%d", rc);
         goto error;
      }

      rc = lr.push(DPS_LOG_CSCRT_VESSEL_CSCRT_OPTIONS, sizeof(createCSOptions), (const CHAR *)options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push crt options to log record:%d", rc);
         goto error;
      }

      rc = lr.push(DPS_LOG_CSCRT_VESSEL_SPACE_ID, sizeof(SPACE_ID), (const CHAR *)sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push sid to log record:%d", rc);
         goto error;
      }

      head->_length = lr.alignedLen();

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine