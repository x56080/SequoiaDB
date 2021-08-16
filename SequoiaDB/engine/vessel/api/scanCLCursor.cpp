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

   Source File Name = scanCLCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/scanCLCursor.h"

namespace engine
{
namespace vessel
{
   static const UINT32 MIN_CONTENT_SIZE = sizeof(recordID) + sizeof(DPS_TRANS_ID);

   INT32 scanCLCursor::getNext(ISession *session,
                               slice &record,
                               recordID *rid,
                               DPS_TRANS_ID *transID)
   {
      INT32 rc = SDB_OK;
      slice content;
      UINT32 offset = 0;

      rc = cursorKernal::getNext(session, content);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (content.len() < MIN_CONTENT_SIZE)
      {
         PD_LOG(PDERROR, "invalid content len:%d", content.len());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL != rid)
      {
         *rid = *((const recordID *)((ossValuePtr)(content.data()) + offset));
      }
      offset += sizeof(recordID);

      if (NULL != transID)
      {
         *transID = *((const DPS_TRANS_ID *)((ossValuePtr)(content.data()) + offset));
      }
      offset += sizeof(DPS_TRANS_ID);

      record.reset(content.len() - MIN_CONTENT_SIZE, content.data() + MIN_CONTENT_SIZE);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine