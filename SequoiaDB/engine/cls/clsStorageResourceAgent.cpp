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

   Source File Name = clsStorageResourceAgent.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsStorageResourceAgent.hpp"
#include "clsIndexInfo.hpp"
#include "dmsEngineOptions.hpp"
#include "interface/IDataCollection.h"
#include "interface/IDataStorageEngine.h"
#include "msgDef.h"
#include "ossLikely.hpp"
#include "ossMemPool.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "sdbInterface.hpp"
#include "utilUniqueID.hpp"
#include <memory>

namespace engine
{
class _clsStorageResourceAgentImpl : public clsStorageResourceAgent
{
public:
   _clsStorageResourceAgentImpl( IDataManagementService *dms ) : _dms( dms )
   {
   }

public:
   virtual INT32 getIndexes( IExecutor *executor,
                             const CHAR *clFullName,
                             clsIndexInfoSetPtr &indexSetPtr ) override;

   virtual INT32 getIndexes( IExecutor *executor,
                             utilCLUniqueID cluid,
                             clsIndexInfoSetPtr &indexSetPtr ) override;

   virtual INT32 getCLMetaCache( IExecutor *executor,
                                 const CHAR *clFullName,
                                 clsCLMetaCachePtr &clCachePtr ) override;

   virtual INT32 getCLMetaCache( IExecutor *executor,
                                 utilCLUniqueID cluid,
                                 clsCLMetaCachePtr &clCachePtr ) override;

private:
   IDataManagementService *_dms = nullptr;
};
using clsStorageResourceAgentImpl = _clsStorageResourceAgentImpl;

std::unique_ptr< clsStorageResourceAgent > newClsStorageResourceAgentImpl(
   IDataManagementService *dms )
{
   return std::unique_ptr< clsStorageResourceAgent >(
      SDB_OSS_NEW clsStorageResourceAgentImpl( dms ) );
}

INT32 _clsStorageResourceAgentImpl::getIndexes(
   IExecutor *executor,
   const CHAR *clFullName,
   clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   indexSetPtr.reset();
   ossPoolVector< BSONObj > indexes;
   dmsOpenCLOptions options;
   DATA_COLLECTION_PTR cl;
   rc = _dms->openCL( executor, clFullName, options, cl );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to open collection" );
      goto error;
   }
   rc = cl->listIndex( executor, indexes );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get index meta data" );
      goto error;
   }
   indexSetPtr = clsIndexInfoSet::buildIndexSetFromBsonVec( indexes );
done:
   if ( cl )
   {
      cl->close();
   }
   return rc;
error:
   indexSetPtr.reset();
   goto done;
}

INT32 _clsStorageResourceAgentImpl::getIndexes(
   IExecutor *executor, utilCLUniqueID cluid, clsIndexInfoSetPtr &indexSetPtr )
{
   INT32 rc = SDB_OK;
   indexSetPtr.reset();
   ossPoolVector< BSONObj > indexes;
   dmsOpenCLOptions options;
   DATA_COLLECTION_PTR cl;
   rc = _dms->openCL( executor, cluid, options, cl );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to open collection" );
      goto error;
   }
   rc = cl->listIndex( executor, indexes );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get index meta data" );
      goto error;
   }
   indexSetPtr = clsIndexInfoSet::buildIndexSetFromBsonVec( indexes );
done:
   if ( cl )
   {
      cl->close();
   }
   return rc;
error:
   indexSetPtr.reset();
   goto done;
}

INT32 _clsStorageResourceAgentImpl::getCLMetaCache(
   IExecutor *executor, const CHAR *clFullName, clsCLMetaCachePtr &clCachePtr )
{
   INT32 rc = SDB_OK;
   clCachePtr .reset();
   ossPoolVector< BSONObj > indexes;
   dmsOpenCLOptions options;
   DATA_COLLECTION_PTR cl;
   BSONObj meta;
   utilCLUniqueID cluid = UTIL_UNIQUEID_NULL;
   rc = _dms->openCL( executor, clFullName, options, cl );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to open collection" );
      goto error;
   }
   rc = cl->listIndex( executor, indexes );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get index meta data" );
      goto error;
   }
   rc = cl->getMetaData( executor, meta );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get collection meta data" );
      goto error;
   }
   cluid = meta.getField( FIELD_NAME_CL_UNIQUEID ).Long();
   clCachePtr = std::make_shared< clsCLMetaCache >(
      clFullName, cluid, clsIndexInfoSet::buildIndexSetFromBsonVec( indexes ) );
done:
   if ( cl )
   {
      cl->close();
   }
   return rc;
error:
   clCachePtr .reset();
   goto done;
}

INT32 _clsStorageResourceAgentImpl::getCLMetaCache(
   IExecutor *executor, utilCLUniqueID cluid, clsCLMetaCachePtr &clCachePtr )
{
   INT32 rc = SDB_OK;
   clCachePtr .reset();
   ossPoolVector< BSONObj > indexes;
   dmsOpenCLOptions options;
   DATA_COLLECTION_PTR cl;
   BSONObj meta;
   std::string clFullName;
   rc = _dms->openCL( executor, cluid, options, cl );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to open collection" );
      goto error;
   }
   rc = cl->listIndex( executor, indexes );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get index meta data" );
      goto error;
   }
   rc = cl->getMetaData( executor, meta );
   if ( OSS_UNLIKELY( SDB_OK != rc ) )
   {
      PD_LOG( PDERROR, "failed to get collection meta data" );
      goto error;
   }
   clFullName = meta.getField( FIELD_NAME_NAME ).String();
   clCachePtr = std::make_shared< clsCLMetaCache >(
      std::move( clFullName ),
      cluid,
      clsIndexInfoSet::buildIndexSetFromBsonVec( indexes ) );
done:
   if ( cl )
   {
      cl->close();
   }
   return rc;
error:
   clCachePtr .reset();
   goto done;
}
} // namespace engine
