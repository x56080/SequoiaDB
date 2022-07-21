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
#include "vessel/indexUtils.h"
#include "vessel/lsm/lsmIndexEntryValue.h"
#include "vessel/indexObject.h"
#include "vessel/threadContext.h"

namespace engine
{
namespace vessel
{
   lsmIndexIterator::lsmIndexIterator()
   {
      SDB_ASSERT(nullptr != GET_THREAD_CONTEXT(), "can not be invalid");
   }

   lsmIndexIterator::~lsmIndexIterator()
   {
      if (nullptr != _itr)
      {
         delete _itr;
      }

      if (nullptr != _backwardEntryCache)
      {
         GET_THREAD_CONTEXT()->releaseBuffer(_backwardEntryCache);
      }
   }

   void lsmIndexIterator::reset()
   {
      _o = options();
      _cf = lsmColumnFamily();
      _obj = nullptr;
      _globalId.reset();
      if (nullptr != _itr)
      {
         delete _itr;
         _itr = nullptr;
      }
      _lowKey = rocksdb::Slice();
      _upKey = rocksdb::Slice();
      _currentEntry.reset();
      if (nullptr != _backwardEntryCache)
      {
         GET_THREAD_CONTEXT()->releaseBuffer(_backwardEntryCache);
         _backwardEntryCache = nullptr;
      }
      _backwardEntryCacheSize = 0;
      _backwardEntrySize = 0;
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

   INT32 lsmIndexIterator::init(const lsmColumnFamily &cf,
                                const globalLogicalClId &cl,
                                const indexObject *obj,
                                const options &o)
   {
      INT32 rc = SDB_OK;
      rocksdb::ReadOptions opt;
      globalIndexID indexId;
                        
      reset();

      if (OSS_UNLIKELY(!cf.isValid() ||
                       !cl.isValid() ||
                       nullptr == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _o = o;
      _cf = cf;
      _obj = obj;
      _globalId.reset(cl.getLogicalCSID(),
                      cl.getLogicalCLID(),
                      obj->getLogicalID());
      _initKeyBoundWhenOpen(_globalId);

      opt.iterate_lower_bound = &_lowKey;
      opt.iterate_upper_bound = &_upKey;
      opt.auto_prefix_mode = TRUE;
      _itr = _cf.newIterator(opt);
      if (OSS_UNLIKELY(nullptr == _itr))
      {
         PD_LOG(PDERROR, "failed to allocate new itr");
         rc = SDB_OOM;
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::advance(const bson::BSONObj &prevKey,
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
      seekOptions so;
      so.inclusive = o.inclusive && matchInclusive.allInclusive();
      
      bson::BSONObj keyObj = indexUtils::buildKeyToSeek(prevKey, fieldCountToCmpInPrev,
                                                        matchEles, nullptr);
      rc = seekKey(ixmKeyOwned(keyObj), so);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::seekKey(const ixmKey &key,
                                   const seekOptions &o)
   {
      INT32 rc = SDB_OK;
      recordID rid;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_o.forward)
      {
         rid = o.inclusive ?
               recordID::createMinRid() :
               recordID::createMaxRid();
      }
      else
      {
         rid = o.inclusive ?
               recordID::createMaxRid() :
               recordID::createMinRid();
      }

      rc = packer.packFullKey(key, _globalId,
                              _obj->getProperties().getPattern().getOrdering(),
                              rid, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack full key by packer failed, rc:%d", rc);
         goto error;
      }

      rc = seekFullKey(packer.getFullKeySlice(), !_o.forward);
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
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::locate(const indexEntryLocation *location)
   {
      INT32 rc = SDB_OK;
      keyString target;
      const lsmIndexEntryLocation *lsmLocation = nullptr;

      if (OSS_UNLIKELY(!_isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == location ||
                            IDX_ENTRY_LOCATION_TYPE::LSM != location->getType() ||
                            location->hitTheEnd()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      } 

      _currentEntry.reset();

      lsmLocation = static_cast<const lsmIndexEntryLocation *>(location);
      rc = target.init(lsmLocation->getKeySlice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init key string:%d", rc);
         goto error;
      }

      rc = seekFullKey(target.getDataSlice(), FALSE);
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
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::seekFullKey(const slice &fullKey,
                                       BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _itr, "can not be null");
      SDB_ASSERT(0 < fullKey.getSize(), "can not be empty");
      _currentEntry.reset();

      rocksdb::Slice s(fullKey.getData(), fullKey.getSize());

      if (!forPrev)
      {
         _itr->Seek(s);
      }
      else
      {
         _itr->SeekForPrev(s);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lsmIndexIterator::_isMarkedRemoved(rocksdb::Iterator *itr)const
   {
      BOOLEAN r = FALSE;
      rocksdb::Slice value = itr->value();
      if (!value.empty())
      {
         SDB_ASSERT(LSM_INDEX_ENTRY_VALUE_SIZE <= value.size(), "invalid size");
         const lsmIndexEntryValue * val =
               reinterpret_cast<const lsmIndexEntryValue *>(value.data());
         SDB_ASSERT(val->isValid(), "can not be invalid");
         r = val->isDeleted();
      }
      return r;
   }

   UINT64 lsmIndexIterator::getLSN()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      if (LSM_INDEX_ENTRY_VALUE_SIZE <= _itr->value().size())
      {
         const lsmIndexEntryValue *value =
                  reinterpret_cast<const lsmIndexEntryValue *>(_itr->value().data());
         lsn = value->lsn;
      }
      return lsn;
   }

   DPS_TRANS_ID lsmIndexIterator::getTransID()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      DPS_TRANS_ID transID;
      if (LSM_INDEX_ENTRY_VALUE_SIZE <= _itr->value().size())
      {
         const lsmIndexEntryValue *value = reinterpret_cast<const lsmIndexEntryValue *>
                                  (_itr->value().data());
         transID = value->transID;
      }
      
      return transID;
   }

   recordID lsmIndexIterator::getRid()const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _currentEntry.getRid();
   }

   BOOLEAN lsmIndexIterator::equals(const ixmKey &key)const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      return _isReadyToRead() && _currentEntry.getKey().woEqual(key);
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

   slice lsmIndexIterator::getEncodedKey()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return slice(_currentEntry.getKey().dataSize(),
                   _currentEntry.getKey().data());
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
         if (LSM_IDX_MIN_FULL_KEY_SIZE > _itr->key().size())
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "invalid key:%d", rc);
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
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::ensureBackwardToVisiblePosition()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_itr->Valid(), "can not be invalid");

      _currentEntry.reset();
      _backwardEntrySize = 0;
      while (_itr->Valid())
      {
         lsmPureKeyEntry entryInItr;
         lsmPureKeyEntry entryInCache;
         BOOLEAN removedFlag = _isMarkedRemoved(_itr);

         rc = _cacheBackwardEntry(_itr->key());
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
               _backwardEntrySize = 0;
            }
            break;
         }
         else if (removedFlag)
         {
            _backwardEntrySize = 0;
         }
         else
         {
            rc = entryInCache.shallowCopy(
                     rocksdb::Slice(_backwardEntryCache,
                                    _backwardEntrySize));
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

      if (0 < _backwardEntrySize)
      {
         rc = _currentEntry.shallowCopy(
                  rocksdb::Slice(_backwardEntryCache,
                                 _backwardEntrySize));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update current entry:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;

   }

   INT32 lsmIndexIterator::moveToNextVisiblePosition()
   {
      INT32 rc = SDB_OK;

      if (_itr->Valid())
      {
         if (_o.forward)
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
         _backwardEntrySize = 0;
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
         if (_o.forward)
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
         _backwardEntrySize = 0;
      }

   done:
      return rc;
   error:
      goto done;

   }

   void lsmIndexIterator::_initKeyBoundWhenOpen(const globalIndexID &id)
   {
      SDB_ASSERT(id.isValid(), "can not be invalid");
      _lowBound.init(id);
      _upBound.initAsUpKey(id);
      _lowKey = rocksdb::Slice(_lowBound.getData(), _lowBound.getSize());
      _upKey = rocksdb::Slice((_upBound.getData(), _upBound.getSize());
      return;
   }

   INT32 lsmIndexIterator::_cacheBackwardEntry(const rocksdb::Slice &s)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < s.size(), "can not be invalid");
      _backwardEntrySize = 0;

      rc = _ensureBackwardEntryCache(s.size());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure cache:%d", rc);
         goto error;
      }

      ossMemcpy(_backwardEntryCache, s.data(), s.size());
      _backwardEntrySize = s.size();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::_ensureBackwardEntryCache(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < size, "can not be invalid");
      if (_backwardEntryCacheSize < size)
      {
         THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
         if (nullptr != _backwardEntryCache)
         {
            tc->releaseBuffer(_backwardEntryCache);
            _backwardEntryCache = nullptr;
         }

         _backwardEntryCache = tc->allocateBuffer(size);
         if (OSS_UNLIKELY(nullptr == _backwardEntryCache))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         _backwardEntryCacheSize = size;
      }
      
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine