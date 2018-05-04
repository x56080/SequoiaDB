/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = impCatalogAgent.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impCatalogAgent.hpp"
#include "clsCatalogAgent.hpp"

using namespace bson;

namespace import
{
   CatalogAgent::CatalogAgent()
   {
      _cataAgent = NULL;
   }

   CatalogAgent::~CatalogAgent()
   {
      SAFE_OSS_DELETE(_cataAgent);
   }

   INT32 CatalogAgent::updateCatalog(const char* bsonData)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != bsonData, "bsonData can't be NULL");

      if (NULL == _cataAgent)
      {
         _cataAgent = SDB_OSS_NEW engine::_clsCatalogAgent();
         if (NULL == _cataAgent)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to alloc _clsCatalogAgent");
            goto error;
         }
      }

      try
      {
         BSONObj obj(bsonData);

         rc = _cataAgent->updateCatalog(0, 0, bsonData, obj.objsize());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update _cataAgent from bson");
            goto error;
         }
      }
      catch (std::exception &e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "failed to update _cataAgent from bson,"
                "received unexcepted error:%s", e.what());
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CatalogAgent::getCataInfo(const string& collectionName, CataInfo& cataInfo)
   {
      INT32 rc = SDB_OK;
      engine::_clsCatalogSet* cataSet = NULL;

      SDB_ASSERT(NULL != _cataAgent, "_cataAgent can't be NULL");
      SDB_ASSERT(NULL == cataInfo._cataSet, "cataInfo must be empty");

      cataSet = _cataAgent->collectionSet(collectionName.c_str());
      if (NULL == cataSet)
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to find catalog of collection %s",
                collectionName.c_str());
         goto error;
      }

      cataInfo._cataSet = cataSet;
      cataInfo._collectionName = collectionName;

   done:
      return rc;
   error:
      goto done;
   }
}
