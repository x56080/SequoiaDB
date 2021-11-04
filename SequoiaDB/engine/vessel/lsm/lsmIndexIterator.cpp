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
#include "vessel/lsm/lsmIndexValue.hpp"
#include "vessel/lsm/lsmScanEntryParser.h"

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
      _ic = NULL;
      _globalId.reset();
      _lsmDB = NULL;
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
      return NULL != _itr && _itr->Valid() && _currentEntry.isValid();
   }

   BOOLEAN lsmIndexIterator::isOpen()const
   {
      return NULL != _context;
   }

   INT32 lsmIndexIterator::open(requestContext *context,
                                indexContext *ic)
   {
      INT32 rc = SDB_OK;
      CHAR minKey = 1;
      rocksdb::ReadOptions o;
      globalIndexID indexId;
      globalIndexID upperIndexId;
      StackBufBuilder minKeyBuilder;
                        
      _close();

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       INDEX_TYPE_LSM != ic->getIndexType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _ic = ic;

      indexId = globalIndexID(context->getLogicalCSID(),
                              context->getLogicalCLID(),
                              ic->getIndexID());
      upperIndexId = globalIndexID(context->getLogicalCSID(),
                                   context->getLogicalCLID(),
                                   ic->getIndexID() + 1);

      ixmKeyUtils::buildMinKey(ic->getObj().getPattern().getKeyCount(),
                               ic->getObj().getPattern().getOrdering().toBsonOrdering(),
                               minKeyBuilder);

      rc = lsmPackIndexFullKey(_lowBoundKey,
                               LSM_MIN_FULL_KEY_SIZE,
                               indexId,
                               ic->getObj().getPattern().getOrdering(),
                               minKeyBuilder.buf(),
                               recordID::createMinRid(),
                               DPS_INVALID_LSN_OFFSET,
                               DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build low bound key:%d", rc);
         goto error;
      }

      rc = lsmPackIndexFullKey(_upperBoundKey,
                               LSM_MIN_FULL_KEY_SIZE,
                               upperIndexId,
                               ic->getObj().getPattern().getOrdering(),
                               ixmKey(&minKey),
                               recordID::createMinRid(),
                               DPS_INVALID_LSN_OFFSET,
                               DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build upper bound key:%d", rc);
         goto error;
      }

      _globalId.reset(context->getLogicalCSID(),
                      context->getLogicalCLID(),
                      ic->getIndexID());
      _lsmDB = &context->getEnv()->lsm;
      _lowKey = rocksdb::Slice(_lowBoundKey, LSM_MIN_FULL_KEY_SIZE - 1 + minKeyBuilder.len());
      _upKey = rocksdb::Slice(_upperBoundKey, LSM_MIN_FULL_KEY_SIZE);
      o = context->getEnv()->lsm.getReadOpt();
      o.iterate_lower_bound = &_lowKey;
      o.iterate_upper_bound = &_upKey;
      o.auto_prefix_mode = TRUE;
      _itr = _lsmDB->NewIterator(o, LSM_CF_INDEX);
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

   INT32 lsmIndexIterator::seekFromCurrentPosition(const bson::BSONObj &prevKey,
                                                   INT32 fieldCountToCmpInPrev,
                                                   const VEC_ELE_CMP &matchEles,
                                                   const inclusiveVec &matchInclusive,
                                                   const options &o)
   {
      return seek(prevKey, fieldCountToCmpInPrev,
                  matchEles, matchInclusive, o);
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &prevKey,
                                INT32 fieldCountToCmpInPrev,
                                const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive,
                                const options &o)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj keyObj;
      options so(o);
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
      options o(TRUE, TRUE);
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
                                   const options &o)
   {
      INT32 rc = SDB_OK;
      rocksdb::Slice fullKey;
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      bson::StackBufBuilder fullKeyBuilder;

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

      if (o.isForward())
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

      fullKey = packFullKey(key, rid,
                            lsn, DPS_TRANS_ID(), fullKeyBuilder);
      if (fullKey.empty())
      {
         PD_LOG(PDERROR, "failed to build full key:%d", rc);
         goto error;
      }

      rc = seekFullKey(fullKey, !o.isForward());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek full key:%d", rc);
         goto error;
      }

      rc = moveIfEntryRemoved(o.isForward());
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::seekEntry(const slice &entry,
                                     const options &o)
   {
      INT32 rc = SDB_OK;
      rocksdb::Slice fullKey;
      StackBufBuilder builder;
      lsmScanEntryParser parser;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = parser.parse(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse entry data:%d", rc);
         goto error;
      }

      if (o.isForward())
      {
         lsn = o.isInclusive() ? DPS_INVALID_LSN_OFFSET : 0;
      }
      else
      {
         lsn = o.isInclusive() ? 0 : DPS_INVALID_LSN_OFFSET;
      }

      fullKey = packFullKey(parser.getKey(), parser.getRid(),
                            lsn, DPS_TRANS_ID(), builder);
      if (fullKey.empty())
      {
         PD_LOG(PDERROR, "failed to build full key:%d", rc);
         goto error;
      }

      rc = seekFullKey(fullKey, !o.isForward());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek full key:%d", rc);
         goto error;
      }

      rc = moveIfEntryRemoved(o.isForward());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed move iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::moveToNextDiffKeyOrRid(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      static const UINT32 _NEXT_COUNT = 8;
      bson::StackBufBuilder builder;

      do
      {
         rocksdb::Slice fullKeySlice;
         builder.reset();
         builder.appendBuf(_currentEntry.getKey().data(),
                           _currentEntry.getKey().dataSize());
         ixmKey lastKey(builder.buf());
         recordID lastRid = _currentEntry.getRid();

         for (UINT32 i = 0; i < _NEXT_COUNT; ++i)
         {
            rc = moveIterator(forward);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move iterator:%d", rc);
               goto error;
            }

            /// hit the end
            if (!_isReadyToRead())
            {
               goto done;
            }

            if (_currentEntry.getRid() != lastRid)
            {
               goto done;
            }
            
            if (!_currentEntry.getKey().woEqual(lastKey))
            {
               goto done;
            }
         }

         fullKeySlice = packFullKey(_currentEntry.getKey(),
                                    _currentEntry.getRid(),
                                    forward ? 0 : DPS_INVALID_LSN_OFFSET,
                                    DPS_TRANS_ID(), builder);

         rc = seekFullKey(fullKeySlice, !forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek full key:%d", rc);
            goto error;
         }
         
      } while(_isReadyToRead());
   done:
      return rc;
   error:
      close();
      goto done;
   }


   INT32 lsmIndexIterator::seekFullKey(const rocksdb::Slice &fullKey,
                                       BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr, "can not be null");
      SDB_ASSERT(0 < fullKey.size(), "can not be empty");
      _currentEntry.reset();

      if (forPrev)
      {
         _itr->SeekForPrev(fullKey);
      }
      else
      {
         _itr->Seek(fullKey);
      }

      if (_itr->Valid())
      {
         rc = cacheCurrentEntry();
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

   rocksdb::Slice lsmIndexIterator::packFullKey(const ixmKey &key,
                                                const recordID &rid,
                                                DPS_LSN_OFFSET lsn,
                                                const DPS_TRANS_ID &transID,
                                                StackBufBuilder &builder)
   {
      INT32 rc = SDB_OK;
      rocksdb::Slice fullKey;
      UINT32 keySize = key.dataSize();
      UINT32 bufSize = lsmCalFullDataKeyLen(keySize);
      builder.reset();
      builder.reserveBytes(bufSize);
      orderingWrapper ow = _ic->getObj().getPattern().getOrdering();

      rc = lsmPackIndexFullKey(builder.buf(),
                               bufSize,
                               _globalId,
                               ow, key, rid, lsn, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to pack full key:%d", rc);
         goto done;
      }

      fullKey = rocksdb::Slice(builder.buf(), bufSize);
      builder.setlen(bufSize);
      
   done:
      return fullKey;
   }

   INT32 lsmIndexIterator::moveIterator(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "must be ready");
      _currentEntry.reset();
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
         rc = cacheCurrentEntry();
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

   INT32 lsmIndexIterator::moveIfEntryRemoved(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      while (_isReadyToRead() && _isMarkedRemoved())
      {
         rc = moveToNextDiffKeyOrRid(forward);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to move iterator:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::cacheCurrentEntry()
   {
      SDB_ASSERT(NULL != _itr && _itr->Valid(), "must be valid");
      INT32 rc = SDB_OK;
      rocksdb::Slice value = _itr->value();
   
      rc = _currentEntry.shallowCopy(_itr->key());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update current entry, entry size[%d], rc:%d",
                _itr->key().size(), rc);
         goto error;
      }

      if (!_currentEntry.getRid().valid() ||
          DPS_INVALID_LSN_OFFSET == _currentEntry.getDataLsn())
      {
         PD_LOG(PDERROR, "invalid entry data found in entry[%s]",
                _itr->key().ToString(TRUE).c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!value.empty())
      {
         const lsmIndexValue * lsmValue = NULL;
         if (value.size() != sizeof(lsmIndexValue))
         {
            PD_LOG(PDERROR, "invalid value size[%d] of entry[%s]",
                   _itr->key().ToString(TRUE).c_str());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         
         lsmValue = (const lsmIndexValue *)(value.data());
         if (!lsmValue->isValid())
         {
            PD_LOG(PDERROR, "invalid value content of entry[%s]",
                   _itr->key().ToString(TRUE).c_str());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

   done:
      return rc;
   error:
      _currentEntry.reset();
      goto done;
   }

   BOOLEAN lsmIndexIterator::_isMarkedRemoved()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      const lsmIndexValue * lsmValue = NULL;
      BOOLEAN r = FALSE;
      rocksdb::Slice value = _itr->value();
      if (value.empty())
      {
         goto done;
      }
      
      SDB_ASSERT(sizeof(lsmIndexValue) == value.size(), "impossible");
      lsmValue = (const lsmIndexValue *)(value.data());
      r = lsmValue->isDeleted();
   done:
      return r;
   }

   UINT64 lsmIndexIterator::getLSN()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getDataLsn();
   }

   slice lsmIndexIterator::getKey()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return slice(_currentEntry.getKey().dataSize(), _currentEntry.getKey().data());
   }

   DPS_TRANS_ID lsmIndexIterator::getTransID()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getTransID();
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

   INT32 lsmIndexIterator::pushCurrentEntryToBatch(indexScanEntryBatch &batch)const
   {
      INT32 rc = SDB_OK;
      slice entry;

      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      entry = slice(_itr->key().size(), _itr->key().data());
      rc = batch.addEntry(entry);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to add entry to batch:%d", rc);
         goto error;
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

   INT32 lsmIndexIterator::next(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = moveToNextDiffKeyOrRid(forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to move iterator:%d", rc);
         goto error;
      }

      rc = moveIfEntryRemoved(forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed move iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine