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

   _dmsBsonCursorReader::~_dmsBsonCursorReader()
   {
      _record = bson::BSONObj();
      _ptr.reset();
   }

   void _dmsBsonCursorReader::init(const DATA_CURSOR_PTR &dcp,
                                   BOOLEAN onlyRecord)
   {
      SDB_ASSERT(dcp && !dcp->isClosed(), "can not be invalid");
      SDB_ASSERT(!isValid(), "do not reinit");
      _onlyRecord = onlyRecord;
      _ptr = dcp;
   }

   void _dmsBsonCursorReader::fini(BOOLEAN closeCursor)
   {
      if (isValid())
      {
         _onlyRecord = TRUE;
         clearDataFetched();
         if (closeCursor)
         {
            _ptr->close();
         }
         _ptr.reset();
      }
      return;
   }

   INT32 _dmsBsonCursorReader::fetchNext(IExecutor *executor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be closed");
      SDB_ASSERT(NULL != executor, "can not be null");

      vessel::slice data;
      const CHAR *recordBuf = NULL;
      clearDataFetched();

      rc = _ptr->fetchNext(executor);
      if (SDB_OK != rc)
      {
         goto error;
      }

      data = _ptr->getFetchedData();
      if (_onlyRecord)
      {
         recordBuf = data.data();
      }
      else
      {
         if (data.getSize() <= _RID_AND_TRANSID_SIZE)
         {
            PD_LOG(PDERROR, "invalid data size[%d] fetched", data.getSize());
            rc = SDB_DMS_RECORD_INVALID;
            goto error;
         }

         recordBuf = data.data() + _RID_AND_TRANSID_SIZE;
         _rid = *((const dmsRecordID *)data.data());
         _transID = *((const DPS_TRANS_ID *)(data.data() + sizeof(dmsRecordID)));

         SDB_ASSERT(_rid.isValid(), "can not be invalid");
      }

      try
      {
         _record = bson::BSONObj(recordBuf);
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
      fini();
      goto done;
   }

   void _dmsBsonCursorReader::clearDataFetched()
   {
      _record = bson::BSONObj();
      _rid.reset();
      _transID.reset();
   }
}//namespace engine