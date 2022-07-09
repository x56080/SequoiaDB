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

   Source File Name = hybridIndexTree.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/hybridIndexTree.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/requestContext.h"
#include "vessel/dmlContext.h"
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "vessel/lsm/lsmIndexKeyPacker.h"
#include "vessel/collectionProperties.h"
#include "vessel/lsm/lsmIndexEntryValue.h"

namespace engine
{
namespace vessel
{
   INT32 hybridIndexTree::insert(requestContext *context,
                                 const indexObject *obj,
                                 const bson::BSONObjSet &keys,
                                 const DPS_LSN_OFFSET &lsn,
                                 const recordID &rid,
                                 const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      if (OSS_UNLIKELY(!cf.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isClPropertiesSet() ||
                            nullptr == obj ||
                            !obj->isValid() ||
                            keys.empty() ||
                            DPS_INVALID_LSN_OFFSET == lsn ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         globalLogicalClId glcl = context->getClProperties()->getGlobalLogicalId();
         globalIndexID indexId(glcl.getLogicalCSID(),
                               glcl.getLogicalCLID(),
                               obj->getLogicalID());
         orderingWrapper ow = obj->getProperties().getPattern().getOrdering();
         rocksdb::Slice valueSlice;
         lsmIndexEntryValue value;
         if (transID.isValid())
         {
            value.type = LSM_INDEX_ENTRY_TYPE_INSERT;
            value.transID = transID;
            valueSlice = rocksdb::Slice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
         }
      
         lsmIndexKeyStackPacker packer;
         lsmWriteBatch batch;
         cf.openBatch(batch);

         for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr)
         {
            
            
            ixmKeyOwned key(*itr);
            rc = packer.packFullKey(key, indexId, ow, rid, lsn);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to pack full key:%d", rc);
               goto error;
            }

            rc = batch.put(packer.getFullKeySlice(), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }

            packer.reset();
         }//for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr)

         batch.setMinDirtyLsn(lsn);
         rc = batch.commit();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit batch:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::write(requestContext *context,
                                const indexObject *obj,
                                const ossPoolList<bson::BSONObj> *insert,
                                const ossPoolList<bson::BSONObj> *remove,
                                const DPS_LSN_OFFSET &lsn,
                                const recordID &rid,
                                const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      if (OSS_UNLIKELY(!cf.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == obj ||
                            !obj->isValid() ||
                            !context->isClPropertiesSet() ||
                            DPS_INVALID_LSN_OFFSET == lsn ||
                            !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         globalLogicalClId clid = context->getClProperties()->getGlobalLogicalId();
         lsmWriteBatch batch;
         cf.openBatch(batch);
         rc = _fillBatch(clid, obj, insert, remove, lsn, rid, transID, &batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fill batch:%d", rc);
            goto error;
         }

         if (batch.isEmpty())
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = batch.commit();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit batch:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::contains(requestContext *context,
                                   const indexObject *obj,
                                   const bson::BSONObj &key,
                                   recordID &rid)
   {
      return SDB_OK;
   }

   INT32 hybridIndexTree::contains(requestContext *context,
                                   const indexObject *obj,
                                   const bson::BSONObjSet &keys,
                                   recordID &rid)
   {
      return SDB_OK;
   }

   INT32 hybridIndexTree::truncate(requestContext *context,
                                   indexObject *obj)
   {
      return SDB_OK;
   }

   INT32 hybridIndexTree::handleDmlRequests(dmlContext *context,
                                            const ossPoolVector<dmlIndexRequest *> &requests)
   {
      INT32 rc = SDB_OK;
      lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
      if (OSS_UNLIKELY(!cf.isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !context->isClPropertiesSet() ||
                            !context->isDmlPositionSet() ||
                            DPS_INVALID_LSN_OFFSET == context->getDmlLSN() ||
                            requests.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         globalLogicalClId clid = context->getClProperties()->getGlobalLogicalId();
         DPS_TRANS_ID transID = context->getOrigTransId();
         lsmWriteBatch batch;
         cf.openBatch(batch);

         for (auto itr = requests.cbegin(); itr != requests.cend(); ++itr)
         {
            const dmlIndexRequest *request = *itr;
            SDB_ASSERT(nullptr != request && request->isValid(), "can not be invalid");
            if (request->isExecuted())
            {
               continue;
            }

            const ossPoolList<bson::BSONObj> *insert = request->getKeysToInsert().empty() ?
                                                       nullptr : &(request->getKeysToInsert());
            const ossPoolList<bson::BSONObj> *remove = request->getKeysToRemove().empty() ?
                                                       nullptr : &(request->getKeysToRemove());
            
            rc = _fillBatch(clid, request->getObject(),
                            insert, remove,
                            context->getDmlLSN(),
                            context->getRid(),
                            transID, &batch);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fill batch:%d", rc);
               goto error;
            }
         }//for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr)

         if (!batch.isEmpty())
         {
            rc = batch.commit();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to commit batch:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::_fillBatch(const globalLogicalClId &clid,
                                     const indexObject *obj,
                                     const ossPoolList<bson::BSONObj> *insert,
                                     const ossPoolList<bson::BSONObj> *remove,
                                     const DPS_LSN_OFFSET &lsn,
                                     const recordID &rid,
                                     const DPS_TRANS_ID &transID,
                                     lsmWriteBatch *batch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(clid.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != obj, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != batch && batch->isOpen(), "can not be invalid");

      globalIndexID indexId(clid.getLogicalCSID(),
                            clid.getLogicalCLID(),
                            obj->getLogicalID());
      orderingWrapper ow = obj->getProperties().getPattern().getOrdering();
      lsmIndexKeyStackPacker packer;

      if (nullptr != remove)
      {
         lsmIndexEntryValue value;
         value.type = LSM_INDEX_ENTRY_TYPE_DELETE;
         value.transID = transID;
         rocksdb::Slice valueSlice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
         
         for (auto itr = remove->cbegin(); itr != remove->cend(); ++itr)
         {
            ixmKeyOwned key(*itr);
            rc = packer.packFullKey(key, indexId, ow, rid, lsn);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to pack full key:%d", rc);
               goto error;
            }

            rc = batch->put(packer.getFullKeySlice(), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }

            packer.reset();
         }
      }

      if (nullptr != insert)
      {
         rocksdb::Slice valueSlice;
         lsmIndexEntryValue value;
         if (transID.isValid())
         {
            value.type = LSM_INDEX_ENTRY_TYPE_INSERT;
            value.transID = transID;
            valueSlice = rocksdb::Slice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
         }
         
         for (auto itr = remove->cbegin(); itr != remove->cend(); ++itr)
         {
            ixmKeyOwned key(*itr);
            rc = packer.packFullKey(key, indexId, ow, rid, lsn);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to pack full key:%d", rc);
               goto error;
            }

            rc = batch->put(packer.getFullKeySlice(), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }

            packer.reset();
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

