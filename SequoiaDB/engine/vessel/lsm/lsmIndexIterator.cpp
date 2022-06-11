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

   Source File Name = lsmIndexIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmIndexIterator.h"
#include "rocksdb/options.h"
#include "ixmKey.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/indexUtils.h"
#include "vessel/lsm/lsmDB.h"
#include "vessel/lsm/lsmIndexValue.hpp"
#include "vessel/lsm/lsmScanEntryParser.h"
#include "vessel/collectionProperties.h"
#include "vessel/lsm/lsmIndexKeyPacker.h"

namespace engine
{
namespace vessel
{
   lsmIndexIterator::~lsmIndexIterator()
   {
      if (NULL != _itr)
      {
         delete _itr;
      }
   }

   void lsmIndexIterator::_close()
   {
      _context = NULL;
      _obj = NULL;
      _globalId.reset();
      if (NULL != _itr)
      {
         delete _itr;
         _itr = NULL;
      }
      _lowKey = rocksdb::Slice();
      _upKey = rocksdb::Slice();
      _currentEntry.reset();
      _builder.reset();
      return;
   }

   void lsmIndexIterator::close()
   {
      _close();
      return;
   }

   BOOLEAN lsmIndexIterator::isReadyToRead()const
   {
      return _isReadyToRead();
   }

   BOOLEAN lsmIndexIterator::_isReadyToRead()const
   {
      return _currentEntry.isValid();
   }

   BOOLEAN lsmIndexIterator::isOpen()const
   {
      return NULL != _context;
   }

   INT32 lsmIndexIterator::open(requestContext *context,
                                indexObject *obj,
                                const options &o)
   {
      INT32 rc = SDB_OK;
      rocksdb::ReadOptions opt;
      globalIndexID indexId;
      globalCollectionId gcid;
      const CHAR *boundPtr = nullptr;
                        
      _close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       NULL == obj ||
                       !obj->isValid() ||
                       INDEX_TYPE_LSM != obj->getDescription().getType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _obj = obj;

      gcid = context->getClProperties()->getGlobalId();

      indexId = globalIndexID(gcid.getCSLid(),
                              gcid.getCLLid(),
                              obj->getIndexId().getLogicalIndexId());
      _initKeyBoundWhenOpen(indexId);

      _forward = o.isForward();
      _globalId = indexId;
      boundPtr = (const CHAR *)(&_lowBound);
      _lowKey = rocksdb::Slice(boundPtr, LSM_IDX_BOUNDARY_SIZE);
      boundPtr = (const CHAR *)(&_upBound);
      _upKey = rocksdb::Slice(boundPtr, LSM_IDX_BOUNDARY_SIZE);
      opt.iterate_lower_bound = &_lowKey;
      opt.iterate_upper_bound = &_upKey;
      opt.auto_prefix_mode = TRUE;
      _itr = context->getEnv()->lsm->getIdxColumnFamily().newIterator(opt);
      if (NULL == _itr)
      {
         PD_LOG(PDERROR, "failed to allocate new itr");
         rc = SDB_OOM;
         goto error;
      }
      _context = context;
   done:
      return rc;
   error:
      _close();
      goto done;
   }

   INT32 lsmIndexIterator::fastNext(const bson::BSONObj &prevKey,
                                    INT32 fieldCountToCmpInPrev,
                                    const VEC_ELE_CMP &matchEles,
                                    const inclusiveVec &matchInclusive,
                                    const seekOptions &o)
   {
      return seek(prevKey, fieldCountToCmpInPrev,
                  matchEles, matchInclusive, o);
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &prevKey,
                                INT32 fieldCountToCmpInPrev,
                                const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive,
                                const seekOptions &o)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj keyObj;
      seekOptions so(o);
      so.setInclusive(o.isInclusive() && matchInclusive.allInclusive());
      
      _builder.reset();
      keyObj = indexUtils::buildKeyToSeek(prevKey, fieldCountToCmpInPrev,
                                          matchEles, &_builder);
      rc = seekKey(ixmKeyOwned(keyObj), so);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::contains(const ixmKey &key, recordID &rid)
   {
      INT32 rc = SDB_OK;
      seekOptions o(TRUE);
      rid = recordID();

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = seekKey(key, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

      if (_isReadyToRead())
      {
         if (_currentEntry.getKey().woEqual(key))
         {
            rid = _currentEntry.getRid();
         }
      }

      _currentEntry.reset();
   done:
      return rc;
   error:
      close();
      rid = recordID();
      goto done;
   }

   INT32 lsmIndexIterator::seekKey(const ixmKey &key,
                                   const seekOptions &o)
   {
      INT32 rc = SDB_OK;
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      lsmIndexKeyStackPacker packer;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_forward)
      {
         rid = o.isInclusive() ?
               recordID::createMinRid() :
               recordID::createMaxRid();
      }
      else
      {
         rid = o.isInclusive() ?
               recordID::createMaxRid() :
               recordID::createMinRid();
      }

      rc = packer.packFullKey(key, _globalId,
                              _obj->getDescription().getPattern().getOrdering(),
                              rid, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack full key by packer failed, rc:%d", rc);
         goto error;
      }

      rc = seekFullKey(packer.getFullKeySlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek full key:%d", rc);
         goto error;
      }

      rc = ensureVisiblePosition();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure visible position:%d", rc);
         goto error;
      }

   done:
      packer.reset();
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::moveToTheNextOfEntry(const slice &entry)
   {
      INT32 rc = SDB_OK;
      lsmIndexKeyStackPacker packer;
      lsmScanEntryParser parser;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _currentEntry.reset();

      rc = parser.parse(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse entry data:%d", rc);
         goto error;
      }

      if (_forward)
      {
         lsn = 0;
      }
      else
      {
         lsn = DPS_INVALID_LSN_OFFSET;
      }

      rc = packer.packFullKey(parser.getKey(), _globalId,
                              _obj->getDescription().getPattern().getOrdering(),
                              parser.getRid(), lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack index full key by packer failed, rc:%d", rc);
         goto error;
      }

      rc = seekFullKey(packer.getFullKeySlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek full key:%d", rc);
         goto error;
      }

      rc = ensureVisiblePosition();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure visible position:%d", rc);
         goto error;
      }

   done:
      packer.reset();
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::seekFullKey(const rocksdb::Slice &fullKey)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr, "can not be null");
      SDB_ASSERT(0 < fullKey.size(), "can not be empty");
      _currentEntry.reset();

      if (_forward)
      {
         _itr->Seek(fullKey);
      }
      else
      {
         _itr->SeekForPrev(fullKey);
      }
      
      if (_itr->Valid())
      {
         if (LSM_IDX_MIN_FULL_KEY_SIZE > _itr->key().size()|| 
             LSM_VALUE_SIZE != _itr->value().size())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid key or value:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lsmIndexIterator::_isMarkedRemoved(rocksdb::Iterator *itr)const
   {
      const lsmIndexValue * lsmValue = NULL;
      BOOLEAN r = FALSE;
      rocksdb::Slice value = itr->value();
      SDB_ASSERT(sizeof(lsmIndexValue) == value.size(), "impossible");
      lsmValue = (const lsmIndexValue *)(value.data());
      r = lsmValue->isDeleted();
      return r;
   }

   UINT64 lsmIndexIterator::getLSN()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getDataLsn();
   }

   bson::BSONObj lsmIndexIterator::getKeyObj(bson::BufBuilder *builder)const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getKey().toBson(builder);
   }

   DPS_TRANS_ID lsmIndexIterator::getTransID()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      const lsmIndexValue *value = reinterpret_cast<const lsmIndexValue *>
                                  (_itr->value().data());
      return value->getTransID();
   }

   recordID lsmIndexIterator::getRid()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getRid();
   }

/*
   indexScanEntry lsmIndexIterator::getCurrentEntry()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      indexScanEntry entry;
      slice entryData = slice(_itr->key().size(), _itr->key().data());
      lsmScanEntryParser parser;

      rc = entry.init(entryData, &parser);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to parse entry data:%d", rc);
      }
      return entry;
   }*/

   UINT32 lsmIndexIterator::getCurrentEntrySize()const
   {
      if (_backwardCurrentEntryCache.isEmpty())
      {
         return _itr->key().size();
      }
      else
      {
         return _backwardCurrentEntryCache.getSize();
      }

   }

   INT32 lsmIndexIterator::pushCurrentEntryToBatch(rowBatch &batch)const
   {
      INT32 rc = SDB_OK;
      slice entry;

      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_backwardCurrentEntryCache.isEmpty())
      {
         entry = slice(_itr->key().size(), _itr->key().data());
         rc = batch.pushRow(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add entry to batch:%d", rc);
            goto error;
         }
      }
      else
      {
         entry = slice(_backwardCurrentEntryCache.getSize(),
                       _backwardCurrentEntryCache.getBuffer());
         rc = batch.pushRow(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add entry to batch:%d", rc);
            goto error;
         }
      }


   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lsmIndexIterator::equalToCurrentKey(const ixmKey &key)const
   {
      return isReadyToRead() && _currentEntry.getKey().woEqual(key);
   }

   void lsmIndexIterator::pause()
   {
      return;
   }

   INT32 lsmIndexIterator::next()
   {
      INT32 rc = SDB_OK;
      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = moveToNextVisiblePosition();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to move to next visible:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::moveIterator(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr && _itr->Valid(), "must be valid");
      if (forward)
      {
         _itr->Next();
      }
      else
      {
         _itr->Prev();
      }

      if (_itr->Valid())
      {
         if (LSM_IDX_MIN_FULL_KEY_SIZE > _itr->key().size()|| 
             LSM_VALUE_SIZE != _itr->value().size())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid key or value:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::forwardToNextVisiblePostion()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_itr->Valid(), "can not be invalid");
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      bson::StackBufBuilder builder;
      builder.appendBuf(_currentEntry.getKey().data(),
                        _currentEntry.getKey().dataSize());
      recordID lastRid = _currentEntry.getRid();
      
      _currentEntry.reset();
      do
      {
         lsmPureKeyEntry currentEntry;
         ixmKey lastKey(builder.buf());

         rc = moveIterator(TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to move iterator:%d", rc);
            goto error;
         }

         if (!_itr->Valid())
         {
            break;
         }

         rc = currentEntry.shallowCopy(_itr->key());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to cache entry %d", rc);
            goto error;
         }

         if (currentEntry.getRid() == lastRid &&
             currentEntry.getKey().woEqual(lastKey))
         {
            continue;
         }

         if (!_isMarkedRemoved(_itr))
         {
            rc = _currentEntry.shallowCopy(_itr->key());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update current entry:%d", rc);
               goto error;
            }
            break;
         }
         else
         {
            builder.reset();
            builder.appendBuf(currentEntry.getKey().data(),
                              currentEntry.getKey().dataSize());
            lastRid = currentEntry.getRid();
         }

      } while(_itr->Valid());
   done:
      return rc;
   error:
      _close();
      goto done;
   }

   INT32 lsmIndexIterator::ensureBackwardToVisiblePosition()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_itr->Valid(), "can not be invalid");

      _currentEntry.reset();
      _backwardCurrentEntryCache.resize(0);
      while (_itr->Valid())
      {
         lsmPureKeyEntry entryInItr;
         lsmPureKeyEntry entryInCache;
         BOOLEAN removedFlag = _isMarkedRemoved(_itr);

         rc = _backwardCurrentEntryCache.copy(_itr->key().size(),
                                              _itr->key().data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to cache entry:%d", rc);
            goto error;
         }
         
         rc = moveIterator(FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to move iterator:%d", rc);
            goto error;
         }

         if (!_itr->Valid())
         {
            if (removedFlag)
            {
               _backwardCurrentEntryCache.resize(0);
            }
            break;
         }
         else if (removedFlag)
         {
            _backwardCurrentEntryCache.resize(0);
         }
         else
         {
            rc = entryInCache.shallowCopy(
                     rocksdb::Slice(_backwardCurrentEntryCache.getBuffer(),
                                    _backwardCurrentEntryCache.getSize()));
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to cache entry%d", rc);
               goto error;
            }

            rc = entryInItr.shallowCopy(_itr->key());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to cache entry%d", rc);
               goto error;
            }

            if (entryInItr.getRid() == entryInCache.getRid() &&
                entryInItr.getKey().woEqual(entryInCache.getKey()))
            {
               continue;
            }
            else
            {
               break;
            }
         }
      } 

      if (!_backwardCurrentEntryCache.isEmpty())
      {
         rc = _currentEntry.shallowCopy(
                  rocksdb::Slice(_backwardCurrentEntryCache.getBuffer(),
                                 _backwardCurrentEntryCache.getSize()));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update current entry:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      _close();
      goto done;

   }

   INT32 lsmIndexIterator::moveToNextVisiblePosition()
   {
      INT32 rc = SDB_OK;

      if (_itr->Valid())
      {
         if (_forward)
         {
            rc = forwardToNextVisiblePostion();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to next visible position:%d", rc);
               goto error;
            }
         }
         else
         {
            rc = ensureBackwardToVisiblePosition();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to next visible position:%d", rc);
               goto error;
            }
         }
      }
      else
      {
         _currentEntry.reset();
         _backwardCurrentEntryCache.resize(0);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::ensureVisiblePosition()
   {
      INT32 rc = SDB_OK;

      if (_itr->Valid())
      {
         if (_forward)
         {
            _currentEntry.shallowCopy(_itr->key());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to update current entry:%d", rc);
               goto error;
            }

            if (_isMarkedRemoved(_itr))
            {
               rc = forwardToNextVisiblePostion();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to move to visible position%d", rc);
                  goto error;
               }
            }
         }
         else
         {
            rc = ensureBackwardToVisiblePosition();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure the visible position:%d", rc);
               goto error;
            }
         }
      }
      else
      {
         _currentEntry.reset();
         _backwardCurrentEntryCache.resize(0);
      }

   done:
      return rc;
   error:
      goto done;

   }

   void lsmIndexIterator::_initKeyBoundWhenOpen(const globalIndexID &id)
   {
      SDB_ASSERT(id.isValid(), "can not be invalid");
      _lowBound = id;
      _upBound.reset(id.getLogicalCSID(),
                     id.getLogicalCLID(),
                     id.getLogicalIndexID() + 1);
      return;
   }
}//namespace vessel
}//namespace engine