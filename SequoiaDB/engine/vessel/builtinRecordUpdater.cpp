/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = builtinRecordUpdater.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/builtinRecordUpdater.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void bsonRecordUpdater::setModifier(mthModifier *modifier)
   {
      clearResult();
      SDB_ASSERT(NULL != modifier && modifier->isInitialized(), "can not be invalid");
      _modifier = modifier;
      return;
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
      else if (NULL == _modifier)
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

      rc = _modifier->modify(obj, _result, NULL, &_changed);
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

