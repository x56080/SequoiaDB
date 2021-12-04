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

   Source File Name = builtinRecordUpdater.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/builtinRecordUpdater.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 bsonRecordUpdater::init(const bson::BSONObj &pattern)
   {
      INT32 rc = SDB_OK;
      clearResult();

      rc = _modifier.loadPattern(pattern);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init modifier:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void bsonRecordUpdater::clearResult()
   {
      _result = bson::BSONObj();
      _changed = bson::BSONObj();
   }

   INT32 bsonRecordUpdater::update(UINT32 size,
                                   const CHAR *data)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj obj;
      if (0 == size || NULL == data)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_modifier.isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      clearResult();

      try
      {
         obj = bson::BSONObj(data);
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to init bsonobj:%s", e.what());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _modifier.modify(obj, _result, NULL, &_changed);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to modify record:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN bsonRecordUpdater::nothingUpdated()const
   {
      SDB_ASSERT(done(), "must be done");
      return _changed.isEmpty();
   }

   const CHAR *bsonRecordUpdater::getResultRecord()const
   {
      SDB_ASSERT(done(), "must be done");
      return _result.objdata();
   }

   UINT32 bsonRecordUpdater::getResultRecordSize()const
   {
      SDB_ASSERT(done(), "must be done");
      return _result.objsize();
   }

   BOOLEAN bsonRecordUpdater::isWholeRecordReset()const
   {
      SDB_ASSERT(done(), "must be done");
      return FALSE;
   }

   void bsonRecordUpdater::dumpUpdatedFields(ossPoolVector<const CHAR *> &fields)const
   {
      SDB_ASSERT(done(), "must be done");
      fields.clear();
      bson::BSONObjIterator itr(_changed);
      while (itr.more())
      {
         bson::BSONElement e = itr.next();
         SDB_ASSERT(e.isABSONObj(), "must be obj");
         bson::BSONObj o = e.embeddedObject();
         const CHAR *name = o.firstElementFieldName();
         SDB_ASSERT(NULL != name, "impossible");
         fields.push_back(name);
      }
      return;
   }
} // namespace vessel

} // namespace engine

