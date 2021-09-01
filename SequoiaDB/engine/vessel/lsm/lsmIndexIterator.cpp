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
      if (_globalId.isValid())
      {
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
      }
      return;
   }

   void lsmIndexIterator::close()
   {
      _close();
      indexIterator::_close();
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

   INT32 lsmIndexIterator::open(requestContext *context,
                                const indexHandle &handle,
                                const orderingWrapper &ordering,
                                INT32 direction)
   {
      INT32 rc = SDB_OK;
      CHAR minKey = 1;
      rocksdb::ReadOptions o;
      globalIndexID indexId(context->getLogicalCSID(),
                            context->getLogicalCLID(),
                            handle.getIndexId());
      globalIndexID upperIndexId(context->getLogicalCSID(),
                                 context->getLogicalCLID(),
                                 handle.getIndexId() + 1);
      recordID minRid(0, 0);
                        
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       !handle.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      indexIterator::_open(context, handle, ordering, direction);

      rc = lsmPackIndexFullKey(_lowBoundKey,
                               LSM_MIN_FULL_KEY_SIZE,
                               indexId,
                               ordering,
                               ixmKey(&minKey),
                               minRid,
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
                               ordering,
                               ixmKey(&minKey),
                               minRid,
                               DPS_INVALID_LSN_OFFSET,
                               DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build upper bound key:%d", rc);
         goto error;
      }

      _globalId.reset(context->getLogicalCSID(),
                      context->getLogicalCLID(),
                      handle.getIndexId());
      _lsmDB = &context->getEnv()->lsm;
      _lowKey = rocksdb::Slice(_lowBoundKey, LSM_MIN_FULL_KEY_SIZE);
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
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &prevKey,
                                INT32 fieldCountToCmpInPrev,
                                BOOLEAN upperBound,
                                const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj keyToSeek;
      
      if (OSS_UNLIKELY(!indexIterator::_isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      
      _builder.reset();
      keyToSeek = indexUtils::buildKeyToSeek(prevKey, fieldCountToCmpInPrev,
                                             matchEles, &_builder);
      if (upperBound || !matchInclusive.allInclusive())
      {
         rc = upperBoundKey(keyToSeek);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to upper bound key:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = lowerBoundKey(keyToSeek);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lower bound key:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &key,
                                const recordID &rid,
                                BOOLEAN upperBound)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!indexIterator::_isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (upperBound)
      {
         rc = upperBoundKeyAndRid(key, rid);
      }
      else
      {
         rc = lowerBoundKeyAndRid(key, rid);
      }

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

   INT32 lsmIndexIterator::seek(const bson::BSONObj &key,
                                BOOLEAN upperBound)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!indexIterator::_isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (upperBound)
      {
         rc = upperBoundKey(key);
      }
      else
      {
         rc = lowerBoundKey(key);
      }

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

   INT32 lsmIndexIterator::nextTo(const bson::BSONObj &prevKey,
                                  INT32 fieldCountToCmpInPrev,
                                  BOOLEAN upperBound,
                                  const VEC_ELE_CMP &matchEles,
                                  const inclusiveVec &matchInclusive)
   {
      INT32 rc = SDB_OK;
      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = seek(prevKey, fieldCountToCmpInPrev,
                upperBound, matchEles,
                matchInclusive);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }
   
   INT32 lsmIndexIterator::next()
   {
      INT32 rc = SDB_OK;
      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = moveIterator();
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

   INT32 lsmIndexIterator::nextDiffKeyOrRid()
   {
      INT32 rc = SDB_OK;
      lsmKeyEntry entry;
      static const UINT32 _NEXT_COUNT = 8;
      bson::BSONObj keyObj;
      memoryBlock mb;

      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _backupEntry(_currentEntry, entry, mb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to backup current entry:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < _NEXT_COUNT; ++i)
      {
         INT32 cmp = 0;
         /// current entry will may be updated fro here.
         rc = moveIterator();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to move iterator:%d", rc);
            goto error;
         }

         if (!_isReadyToRead())
         {
            goto done;
         }

         if (_currentEntry.getRid() != entry.getRid())
         {
            goto done;
         }
         
         cmp = _currentEntry.getKey().woCompare(entry.getKey(),
                                                *(getOrdering().toBsonOrdering()));
         if (0 != cmp)
         {
            goto done;
         }
      }

      _builder.reset();
      keyObj = entry.getKey().toBson(&_builder);
      rc = upperBoundKeyAndRid(keyObj, entry.getRid());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to upper bound index tuple:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 lsmIndexIterator::lowerBoundKey(const bson::BSONObj &key)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr, "not init yet");
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      memoryBlock mb;

      if (indexIterator::isForward())
      {
         /// set it as min value
         rid = recordID(0, 0);
         ///lsn field in descending order
         lsn = DPS_INVALID_LSN_OFFSET;
      }
      else
      {
         ///invalid rid is max value
         ///lsn field in descending order
         lsn = 0;
      }

      rocksdb::Slice fullKeyToSeek = packFullKey(key, rid, lsn, DPS_TRANS_ID(), mb);
      if (fullKeyToSeek.empty())
      {
         PD_LOG(PDERROR, "failed to pack full key");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _currentEntry.reset();

      if (indexIterator::isForward())
      {
         _itr->Seek(fullKeyToSeek);
      }
      else
      {
         _itr->SeekForPrev(fullKeyToSeek);
      }

      if (_itr->Valid())
      {
         rc = updateCurrentEntry();
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

   INT32 lsmIndexIterator::upperBoundKey(const bson::BSONObj &key)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr, "not init yet");
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      memoryBlock mb;

      if (indexIterator::isForward())
      {
         ///invalid rid is max value
         ///lsn field in descending order
         lsn = 0;
      }
      else
      {
         rid = recordID(0, 0);
         ///lsn field in descending order
         lsn = DPS_INVALID_LSN_OFFSET;
      }

      rocksdb::Slice fullKeyToSeek = packFullKey(key, rid, lsn, DPS_TRANS_ID(), mb);
      if (fullKeyToSeek.empty())
      {
         PD_LOG(PDERROR, "failed to pack full key");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _currentEntry.reset();

      if (indexIterator::isForward())
      {
         _itr->Seek(fullKeyToSeek);
      }
      else
      {
         _itr->SeekForPrev(fullKeyToSeek);
      }

      if (_itr->Valid())
      {
         rc = updateCurrentEntry();
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

   INT32 lsmIndexIterator::lowerBoundKeyAndRid(const bson::BSONObj &key,
                                               const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rid.valid(), "must be valid");
      SDB_ASSERT(NULL != _itr, "not init yet");
      memoryBlock mb;

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (indexIterator::isForward())
      {
         ///lsn field in descending order
         lsn = DPS_INVALID_LSN_OFFSET;
      }
      else
      {
         ///lsn field in descending order
         lsn = 0;
      }

      rocksdb::Slice fullKeyToSeek = packFullKey(key, rid, lsn, DPS_TRANS_ID(), mb);
      if (fullKeyToSeek.empty())
      {
         PD_LOG(PDERROR, "failed to pack full key");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _currentEntry.reset();

      if (indexIterator::isForward())
      {
         _itr->Seek(fullKeyToSeek);
      }
      else
      {
         _itr->SeekForPrev(fullKeyToSeek);
      }

      if (_itr->Valid())
      {
         rc = updateCurrentEntry();
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

   INT32 lsmIndexIterator::upperBoundKeyAndRid(const bson::BSONObj &key,
                                               const recordID &rid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(rid.valid(), "must be valid");
      SDB_ASSERT(NULL != _itr, "not init yet");

      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      memoryBlock mb;

      if (indexIterator::isForward())
      {
         ///lsn field in descending order
         /// The log record with lsn 0 definitely not be 'insert'.
         /// So we can set lsn as 0 here.
         lsn = 0;
      }
      else
      {
         ///lsn field in descending order
         lsn = DPS_INVALID_LSN_OFFSET;
      }

      rocksdb::Slice fullKeyToSeek = packFullKey(key, rid, lsn, DPS_TRANS_ID(), mb);
      if (fullKeyToSeek.empty())
      {
         PD_LOG(PDERROR, "failed to pack full key");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _currentEntry.reset();

      if (indexIterator::isForward())
      {
         _itr->Seek(fullKeyToSeek);
      }
      else
      {
         _itr->SeekForPrev(fullKeyToSeek);
      }

      if (_itr->Valid())
      {
         rc = updateCurrentEntry();
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

   rocksdb::Slice lsmIndexIterator::packFullKey(const bson::BSONObj &key,
                                                const recordID &rid,
                                                DPS_LSN_OFFSET lsn,
                                                const DPS_TRANS_ID &transID,
                                                memoryBlock &mb)
   {
      INT32 rc = SDB_OK;
      rocksdb::Slice fullKey;
      mb.resize(0);
      ixmKeyOwned ownedKey(key);
      UINT32 keySize = ownedKey.dataSize();
      UINT32 bufSize = lsmCalFullDataKeyLen(keySize);
      rc = mb.reserve(bufSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto done;
      }

      rc = lsmPackIndexFullKey(mb.getBuffer(),
                               bufSize,
                               _globalId,
                               indexIterator::getOrdering(),
                               ownedKey, rid, lsn, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to pack full key:%d", rc);
         goto done;
      }

      fullKey = rocksdb::Slice(mb.getBuffer(), bufSize);
      
   done:
      return fullKey;
   }

   INT32 lsmIndexIterator::moveIterator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "must be ready");
      _currentEntry.reset();
      if (indexIterator::isForward())
      {
         _itr->Next();
      }
      else
      {
         _itr->Prev();
      }
      
      if (_itr->Valid())
      {
         rc = updateCurrentEntry();
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

   INT32 lsmIndexIterator::updateCurrentEntry()
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

   BOOLEAN lsmIndexIterator::isMarkedRemoved()const
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
      r = lsmValue->isDelete();
   done:
      return r;
   }

   UINT64 lsmIndexIterator::getLSN()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getDataLsn();
   }

   void lsmIndexIterator::getKey(ixmKey &key)const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      key.assign(_currentEntry.getKey());
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

   slice lsmIndexIterator::getValue()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      rocksdb::Slice v = _itr->value();
      return slice(v.size(), v.data());
   }

   UINT32 lsmIndexIterator::getEntrySize()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _itr->key().size();
   }

   INT32 lsmIndexIterator::copyKeyEntry(UINT32 bufferSize,
                                        CHAR *buffer)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "must be valid");

      if (NULL == buffer || bufferSize < _itr->key().size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(buffer, _itr->key().data(), _itr->key().size());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::copyKeyEntryToBuffer(indexEntryBuffer &buffer) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      rc = buffer.save(INDEX_TYPE_LSM, slice(_itr->key().size(), _itr->key().data()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to save entry to buffer:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lsmIndexIterator::pause()
   {
      return;
   }

   INT32 lsmIndexIterator::resume()
   {
      return SDB_OK;
   }

   INT32 lsmIndexIterator::seek(const slice &entry,
                                BOOLEAN upperBound)
   {
      INT32 rc = SDB_OK;
      globalIndexID indexId;
      orderingWrapper ordering;
      ixmKey key;
      recordID rid;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID;
      rocksdb::Slice seekKey;
      bson::BSONObj keyObj;

      if (OSS_UNLIKELY(!indexIterator::_isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(entry.len() < LSM_MIN_FULL_KEY_SIZE))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lsmUnpackIndexFullKey(entry.data(),
                                 entry.len(),
                                 indexId,
                                 ordering,
                                 key,
                                 rid,
                                 lsn,
                                 transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unpack entry:%d", rc);
         goto error;
      }

      if (indexId != _globalId)
      {
         PD_LOG(PDERROR, "index id[%d,%d,%d] found in entry does not match current",
                indexId.getLogicalCSID(), indexId.getLogicalCLID(), indexId.getLogicalIndexID());
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      _builder.reset();
      keyObj = key.toBson(&_builder);
      rc = seek(keyObj, rid, upperBound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek entry:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::_backupEntry(const lsmKeyEntry &src,
                                        lsmKeyEntry &dst,
                                        memoryBlock &mb)
   {
      INT32 rc = SDB_OK;
      UINT32 keyDataSize = src.getKey().dataSize();
      mb.resize(0);
      rc = mb.reserve(keyDataSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto error;
      }

      ossMemcpy(mb.getBuffer(), src.getKey().data(), keyDataSize);
      dst.shallowCopy(ixmKey(mb.getBuffer()),
                      src.getRid(),
                      src.getDataLsn(),
                      src.getTransID());
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine