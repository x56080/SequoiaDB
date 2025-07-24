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

   Source File Name = hybridIndexTree.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/hybridIndexTree.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/requestContext.h"
#include "vessel/dmlContext.h"
#include "vessel/indexObject.h"
#include "ossLikely.hpp"
#include "vessel/collectionProperties.h"
#include "vessel/lsm/lsmIndexEntryValue.h"
#include "vessel/indexSpace.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/sliceTransfer.h"
#include "vessel/hybridTreeIterator.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "vessel/btreeWriter.h"
#include "vessel/spacePteAccessCtx.h"

namespace engine
{
namespace vessel
{
   hybridIndexTree::hybridIndexTree(indexSpace *is):
   _is(is)
   {
      SDB_ASSERT(nullptr != is && is->isOpen(), "can not be invalid");
   }

   void hybridIndexTree::init(indexSpace *is)
   {
      SDB_ASSERT(nullptr != is && is->isOpen(), "can not be invalid");
      _is = is;
   }

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
         STACK_KEY_STRING_BUILDER builder;
         globalLogicalClId glcl = context->getClProperties()->getGlobalLogicalId();
         globalIndexID indexId(glcl.getLogicalCSID(),
                               glcl.getLogicalCLID(),
                               obj->getLogicalID());
         orderingWrapper ow = obj->getProperties().getPattern().getOrdering();

         lsmIndexEntryValue value;
         value.type = LSM_INDEX_ENTRY_TYPE_INSERT;
         value.transID = transID;
         value.lsn = lsn;
         rocksdb::Slice valueSlice = rocksdb::Slice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
      
         lsmWriteBatch batch;
         cf.openBatch(batch);

         for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr)
         {
            keyString ks;

            rc = builder.buildIndexEntryKey(*itr, ow, rid, &indexId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build entry key string:%d", rc);
               goto error;
            }

            ks = builder.getShallowKeyString();

            rc = batch.put(toRocksdbSlice(ks.getRawData()), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }
         }

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
                                   indexObject *obj,
                                   const bson::BSONObj &key,
                                   recordID &rid)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;
      hybridTreeIterator iterator;
      indexIterator::options o;
      o.pointGetOptimized = TRUE;
      inclusiveVec iv;
      globalLogicalClId clid;
      globalIndexID indexId;
      orderingWrapper ordering;
      CHAR buf[keyStringCoder::INDEX_ID_ENCODEING_SIZE] = {};
      slice s(sizeof(buf), buf);
      keyString ks;

      rid.reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == obj ||
                       !obj->isValid() ||
                       !key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      clid = context->getClProperties()->getGlobalLogicalId();
      indexId.reset(clid.getLogicalCSID(),
                    clid.getLogicalCLID(),
                    obj->getLogicalID());
      keyStringCoder().encodeGlobalIndexId(indexId, FALSE, buf);
      iv.setAll(obj->getProperties().getPattern().getKeyCount(), TRUE);
      ordering = obj->getProperties().getPattern().getOrdering();

      rc = builder.buildPredicate(key, ordering, iv, TRUE, s);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      ks = builder.getShallowKeyString();

      rc = iterator.init(context, _is,
                         GET_HYBRID_INDEX_COLUMN_FAMILY(),
                         obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init iterator:%d", rc);
         goto error;
      }

      rc = iterator.seek(ks, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find key:%d", rc);
         goto error;
      }

      if (iterator.isReadyToRead() &&
          0 == ks.compareElements(iterator.getCurrentKeyString()))
      {
         rid = iterator.getRid();
      }
   done:
      iterator.reset();
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::contains(requestContext *context,
                                   indexObject *obj,
                                   const bson::BSONObjSet &keys,
                                   recordID &rid)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;
      indexIterator::options o;
      o.pointGetOptimized = TRUE;
      inclusiveVec iv;
      hybridTreeIterator iterator;
      UINT32 fields = 0;
      globalLogicalClId clid;
      globalIndexID indexId;
      orderingWrapper ordering;
      CHAR buf[keyStringCoder::INDEX_ID_ENCODEING_SIZE] = {};
      slice s(sizeof(buf), buf);

      rid.reset();

      if (OSS_UNLIKELY(nullptr != context ||
                       !context->isClPropertiesSet() ||
                       nullptr == obj ||
                       !obj->isValid() ||
                       keys.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      clid = context->getClProperties()->getGlobalLogicalId();
      indexId.reset(clid.getLogicalCSID(),
                    clid.getLogicalCLID(),
                    obj->getLogicalID());
      keyStringCoder().encodeGlobalIndexId(indexId, FALSE, buf);
      fields = obj->getProperties().getPattern().getKeyCount();
      iv.setAll(fields, TRUE);
      ordering = obj->getProperties().getPattern().getOrdering();

      rc = iterator.init(context, _is,
                         GET_HYBRID_INDEX_COLUMN_FAMILY(),
                         obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init iterator:%d", rc);
         goto error;
      }

      for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr)
      {
         keyString ks;
         rc = builder.buildPredicate(*itr, ordering, iv, TRUE, s);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build predicate:%d", rc);
            goto error;
         }

         ks = builder.getShallowKeyString();
         rc = iterator.seek(ks, o);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find key:%d", rc);
            goto error;
         }

         if (iterator.isReadyToRead())
         {
            if (0 == ks.compareElements(iterator.getCurrentKeyString()))
            {
               rid = iterator.getRid();
               break;
            }
         }
      }

   done:
      iterator.reset();
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::truncate(requestContext *context,
                                   indexObject *obj,
                                   spacePteAccessCtx *ac,
                                   BOOLEAN removeEntryPage)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == obj ||
                       !obj->isValid() ||
                       nullptr == ac))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         lsmColumnFamily cf = GET_HYBRID_INDEX_COLUMN_FAMILY();
         globalLogicalClId gclid = context->getClProperties()->getGlobalLogicalId();
         globalIndexID indexId(gclid, obj->getLogicalID());
         lsmIteratorBound bound;

         rc = _truncateBtree(context, obj, ac, removeEntryPage);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate btree%d", rc);
            goto error;
         }

         rc = bound.init(indexId);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to build key bound:%d", rc);
            goto error;
         }

         rc = cf.truncate(*bound.getLowBound(),
                          *bound.getUpBound());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate column family:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
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
      STACK_KEY_STRING_BUILDER builder;

      if (nullptr != remove)
      {
         lsmIndexEntryValue value;
         value.type = LSM_INDEX_ENTRY_TYPE_DELETE;
         value.transID = transID;
         value.lsn = lsn;
         rocksdb::Slice valueSlice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
         
         for (auto itr = remove->cbegin(); itr != remove->cend(); ++itr)
         {
            keyString ks;

            rc = builder.buildIndexEntryKey(*itr, ow, rid, &indexId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build entry key string:%d", rc);
               goto error;
            }

            ks = builder.getShallowKeyString();

            rc = batch->put(toRocksdbSlice(ks.getRawData()), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }
         }
      }

      if (nullptr != insert)
      {
         rocksdb::Slice valueSlice;
         lsmIndexEntryValue value;
         value.type = LSM_INDEX_ENTRY_TYPE_INSERT;
         value.transID = transID;
         value.lsn = lsn;
         valueSlice = rocksdb::Slice((const CHAR *)(&value), LSM_INDEX_ENTRY_VALUE_SIZE);
         
         for (auto itr = insert->cbegin(); itr != insert->cend(); ++itr)
         {
            keyString ks;

            rc = builder.buildIndexEntryKey(*itr, ow, rid, &indexId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to build entry key string:%d", rc);
               goto error;
            }

            ks = builder.getShallowKeyString();

            rc = batch->put(toRocksdbSlice(ks.getRawData()), valueSlice);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to put entry into batch:%d", rc);
               goto error;
            }
         }
      }

      batch->setMinDirtyLsn(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridIndexTree::createIterator(requestContext *context,
                                         indexObject *obj,
                                         INDEX_ITERATOR_UPTR &ptr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _is, "can not be invalid");
      SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");

      hybridTreeIterator *itr = nullptr;
      ptr.reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       nullptr == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      ptr.reset(SDB_OSS_NEW hybridTreeIterator());
      if (OSS_UNLIKELY(!ptr))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }
      itr = static_cast<hybridTreeIterator *>(ptr.get());

      rc = itr->init(context, _is, GET_HYBRID_INDEX_COLUMN_FAMILY(), obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init hybrid tree iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      ptr.reset();
      goto done;
   }

   INT32 hybridIndexTree::_truncateBtree(requestContext *context,
                                         indexObject *obj,
                                         spacePteAccessCtx *ac,
                                         BOOLEAN removeEntryPage)
   {
      INT32 rc = SDB_OK;
      btreeWriter bw;
      rc = bw.init(context, _is, obj, ac);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree writer:%d", rc);
         goto error;
      }

      rc = bw.truncate(removeEntryPage);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to truncate btree");
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine

