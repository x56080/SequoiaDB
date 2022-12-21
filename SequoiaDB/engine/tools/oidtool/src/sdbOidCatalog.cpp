/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = sdbOidCatalog.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/

// Note: the functions in this file is copied from impCatalogAgent.hpp and impCataInfo.hpp

#include "clsCatalogAgent.hpp"
#include "../../../client/client.hpp"
#include "sdbOidCatalog.hpp"

using namespace bson;

/**
 * CataInfo
 */
CataInfo::CataInfo()
{
   _cataSet = NULL;
}

CataInfo::~CataInfo()
{
   _cataSet = NULL;
}

INT32 CataInfo::getGroupByRecord(const char* bsonData, UINT32& groupId)
{
   INT32 rc = SDB_OK;

   SDB_ASSERT(NULL != _cataSet, "Must be inited");
   SDB_ASSERT(NULL != bsonData, "BsonData can't be NULL");

   try
   {
      BSONObj obj(bsonData);

      rc = _cataSet->findGroupID(obj, groupId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get group by record, rc=%d", rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_INVALIDARG;
      PD_LOG(PDERROR, "Failed to get group by record,"
               "received unexcepted error:%s", e.what());
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 CataInfo::getAllGroupID( vector<UINT32>& list )
{
   INT32 rc = SDB_OK ;
   INT32 size = 0 ;
   engine::VEC_GROUP_ID tmpList ;
   engine::VEC_GROUP_ID::iterator it ;

   SDB_ASSERT( NULL != _cataSet, "Must be inited" ) ;

   size = _cataSet->getAllGroupID( tmpList );
   if ( 0 > size )
   {
      rc = size ;
      PD_LOG( PDERROR, "Failed to get all group id, rc=%d", rc ) ;
      goto error ;
   }

   try
   {
      for ( it = tmpList.begin(); it != tmpList.end(); ++it )
      {
         list.push_back( *it ) ;
      }
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_OOM ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/**
 * CatalogAgent
 */
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

   SDB_ASSERT(NULL != bsonData, "BsonData can't be NULL");

   if (NULL == _cataAgent)
   {
      _cataAgent = SDB_OSS_NEW engine::_clsCatalogAgent();
      if (NULL == _cataAgent)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "Failed to alloc _clsCatalogAgent");
         goto error;
      }
   }

   try
   {
      BSONObj obj(bsonData);

      rc = _cataAgent->updateCatalog(0, 0, bsonData, obj.objsize());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to update _cataAgent from bson");
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_INVALIDARG;
      PD_LOG(PDERROR, "Failed to update _cataAgent from bson,"
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
      PD_LOG(PDERROR, "Failed to find catalog of collection %s",
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