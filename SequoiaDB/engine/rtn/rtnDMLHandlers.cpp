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

   Source File Name = rtnDMLHandlers.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "rtnDMLHandlers.hpp"
#include "pdTrace.hpp"
#include "pmd.hpp"
#include "dmsEngineCB.hpp"


namespace engine
{
/////////rtnInsertHandler begin

   void rtnInsertHandler::init(const CHAR *name,
                               const utilCLUniqueID &uniqueId,
                               const bson::BSONObj &rows,
                               UINT32 rowCount)
   {
      SDB_ASSERT(NULL != name && 0 < rowCount, "can not be invalid");

      _fullName = name;
      _uniqueId = uniqueId;
      _rows = rows;
      _count = rowCount;
      return;
   }

   INT32 rtnInsertHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != cb, "can not be null");
      SDB_ASSERT(0 < _count, "can not be invalid");
      SDB_DMS_ENGINE_CB *engineCB = pmdGetKRCB()->getDMSEngineCB();
      IDataStorageEngine *engine = engineCB->getEngine();
      DATA_COLLECTION_PTR cl;

      if (UTIL_IS_VALID_CLUNIQUEID(_uniqueId))
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else
      {
         rc = engine->openCL(cb, _fullName, dmsOpenCLOptions(), cl);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      if (1 == _count)
      {
         rc = cl->insertRecord(cb, _rows, dmsInsertRecordOptions(), _result);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         ossPoolVector<bson::BSONObj> batch;
         batch.reserve(_count);
         INT64 size = 0;
         
         for (UINT32 i = 0; i < _count; ++i)
         {
            const CHAR *p = _rows.objdata() + size;
            bson::BSONObj row(p);
            batch.push_back(row);
            size += ossAlign4((UINT32)(row.objsize()));
         }

         rc = cl->insertBatch(cb, batch, dmsInsertRecordOptions(), _result);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
/////////rtnInsertHandler end
} // namespace engine
