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

   Source File Name = dmsCursorReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsCursorReader.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   constexpr UINT32 _RID_AND_TRANSID_SIZE = sizeof(dmsRecordID) + sizeof(DPS_TRANS_ID);

   void _dmsBsonCursorReader::init(const DATA_CURSOR_PTR &cursor)
   {
      SDB_ASSERT(cursor && !cursor->isClosed(), "invalid cursor");
      fini(TRUE);
      _cursor = cursor;
   }

   void _dmsBsonCursorReader::fini(BOOLEAN closeCursor)
   {
      clearDataFetched();
      if (_cursor)
      {
         if (closeCursor)
         {
            _cursor->close();
         }
         _cursor.reset();
      }
      return;
   }

   INT32 _dmsBsonCursorReader::fetchNext(IExecutor *executor)
   {
      INT32 rc = SDB_OK;
      vessel::slice s;

      if (!_cursor)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      clearDataFetched();

      rc = _cursor->fetchNext(executor);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _rid = _cursor->getRid();
      _transID = _cursor->getTransId();
      s = _cursor->getDataSlice();

      if (s.getSize() <= sizeof(UINT32))
      {
         PD_LOG(PDERROR, "invalid slice size:%d", s.getSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      try
      {
         _record = bson::BSONObj(s.data());
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to parse record data:%s", e.what());
         rc = SDB_DMS_RECORD_INVALID;
         goto error;
      }
   
   done:
      return rc;
   error:
      fini(TRUE);
      goto done;
   }

   void _dmsBsonCursorReader::clearDataFetched()
   {
      _record = bson::BSONObj();
      _rid.reset();
      _transID.reset();
   }
}//namespace engine