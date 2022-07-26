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
#include "ossLikely.hpp"
#include "vessel/keyStringBuilder.h"
#include "vessel/keyStringModifier.h"

namespace engine
{
namespace vessel
{
   lsmIndexIterator::~lsmIndexIterator()
   {
      if (nullptr != _itr)
      {
         delete _itr;
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
      _ks.reset();
      return;
   }

   BOOLEAN lsmIndexIterator::isReadyToRead()const
   {
      return _isReadyToRead();
   }

   BOOLEAN lsmIndexIterator::_isReadyToRead()const
   {
      return _ks.isValid();
   }

   INT32 lsmIndexIterator::init(const lsmColumnFamily &cf,
                                const globalLogicalClId &cl,
                                const indexObject *obj)
   {
      INT32 rc = SDB_OK;
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

      _cf = cf;
      _obj = obj;
      _globalId.reset(cl.getLogicalCSID(),
                      cl.getLogicalCLID(),
                      obj->getLogicalID());
      _initKeyBoundWhenOpen(_globalId);
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::advance(const bson::BSONObj &prevKey,
                                   INT32 fieldCountToCmpInPrev,
                                   const VEC_ELE_CMP &matchEles,
                                   const inclusiveVec &matchInclusive)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 <= fieldCountToCmpInPrev, "can not be invalid");
      inclusiveVec iv;
      bson::BSONObj obj;
      STACK_KEY_STRING_BUILDER builder;

      if (matchEles.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(!_o.pointGetOnly, "can not be point get");
      obj = indexUtils::buildKeyToSeek(prevKey, fieldCountToCmpInPrev, matchEles);
      iv = matchInclusive;
      iv.setBatch(0, fieldCountToCmpInPrev, TRUE);

      rc = builder.buildPredicate(obj,
                                 _obj->getOrderingWrapper(),
                                 iv, _o.forward, &_globalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _seekKeyString(builder.getShallowKeyString(), !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::seek(const VEC_ELE_CMP &eles,
                                const inclusiveVec &iv,
                                const options &o)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;

      if (OSS_UNLIKELY(eles.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _reinitIterator(o, iv.allInclusive());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = builder.buildPredicate(eles,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, _o.forward, &_globalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _seekKeyString(builder.getShallowKeyString(), !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &key,
                                const inclusiveVec &iv,
                                const options &o)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _reinitIterator(o, iv.allInclusive());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = builder.buildPredicate(key,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, _o.forward, &_globalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _seekKeyString(builder.getShallowKeyString(), !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::locateNext(const indexEntryLocation *location,
                                      const options &o)
   {
      INT32 rc = SDB_OK;
      
      if (OSS_UNLIKELY(nullptr == location ||
                       IDX_ENTRY_LOCATION_TYPE::LSM != location->getType() ||
                       !location->isValidToLocate()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         const lsmIndexEntryLocation *lsmLocation = static_cast<const lsmIndexEntryLocation *>(location);
         keyString ks(lsmLocation->getKeySlice());
         if (!ks.isValid())
         {
            PD_LOG(PDERROR, "failed to load key string");
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _reinitIterator(o, o.pointGetOnly);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
            goto error;
         }

         rc = _locateNextEntry(ks);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to locate entry key:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::equal(const bson::BSONObj &key)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;
      options opt;
      inclusiveVec iv;
      SDB_ASSERT(iv.allInclusive(), "can not be invalid");

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      opt.forward = TRUE;
      opt.pointGetOnly = TRUE;

      rc = _reinitIterator(opt, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = builder.buildPredicate(key,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, _o.forward, &_globalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _seekKeyString(builder.getShallowKeyString(), !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::equal(const VEC_ELE_CMP &matchEles)
   {
      INT32 rc = SDB_OK;
      STACK_KEY_STRING_BUILDER builder;
      options opt;
      inclusiveVec iv;
      SDB_ASSERT(iv.allInclusive(), "can not be invalid");

      if (OSS_UNLIKELY(matchEles.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      opt.forward = TRUE;
      opt.pointGetOnly = TRUE;

      rc = _reinitIterator(opt, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = builder.buildPredicate(matchEles,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, _o.forward, &_globalId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _seekKeyString(builder.getShallowKeyString(), !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexIterator::_locateNextEntry(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      keyStringModifier modifier(ks);
      keyString target;

      if (_o.forward)
      {
         rc = modifier.incRid();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to inc rid of entry:%d", rc);
            goto error;
         }

         target = modifier.getShallowKeyString();
      }
      else
      {
         target = ks;
      }

      rc = _seekKeyString(target, !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lsmIndexIterator::isMarkedRemoved()const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      rocksdb::Slice value = _itr->value();
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
      return _ks.getRid();
   }

   INT32 lsmIndexIterator::next()
   {
      INT32 rc = SDB_OK;
      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _moveIterator();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to move iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::pause()
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!_isInited()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (nullptr != _itr)
      {
         _resetCurrentEntry();
         delete _itr;
         _itr = nullptr;
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   bson::BSONObj lsmIndexIterator::getKeyObj(BOOLEAN withFieldName,
                                             bson::BufBuilder *buf)const
   {
      SDB_ASSERT(_isReadyToRead(), "must be valid");
      const bson::BSONObj &pattern = _obj->getProperties().getPattern().getPattern();
      return nullptr == buf ?
             _ks.toBSON(pattern, withFieldName):
             _ks.toBSON(pattern, *buf, withFieldName);
   }

   INT32 lsmIndexIterator::initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const
   {
      INT32 rc = SDB_OK;
      lsmIndexEntryLocation *l = nullptr;
      
      if (OSS_UNLIKELY(!_isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!location)
      {
         location.reset(SDB_OSS_NEW lsmIndexEntryLocation());
         if (OSS_UNLIKELY(!location))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
      }

      l = static_cast<lsmIndexEntryLocation *>(location.get());
      l->assign(_ks.getDataSlice());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::_moveIterator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");

      _resetCurrentEntry();

      if (_o.forward)
      {
         _itr->Next();
      }
      else
      {
         _itr->Prev();
      }

      if (!_itr->Valid() && _itr->status().ok())
      {
         PD_LOG(PDERROR, "failed to move iterator:%s", _itr->status().getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (_itr->Valid())
      {
         rc = _initCurrentEntry(_itr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init current entry cache:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void lsmIndexIterator::_initKeyBoundWhenOpen(const globalIndexID &id)
   {
      SDB_ASSERT(id.isValid(), "can not be invalid");      
      UINT32 size = 0;
      INT32 rc = STACK_KEY_STRING_BUILDER::buildBoundaryKey(id, FALSE, sizeof(_lowBound), _lowBound, size);
      SDB_ASSERT(SDB_OK == rc, "must be ok");
      SDB_ASSERT(sizeof(_lowBound) == size, "must be same");

      rc = STACK_KEY_STRING_BUILDER::buildBoundaryKey(id, TRUE, sizeof(_upBound), _upBound, size);
      SDB_ASSERT(SDB_OK == rc, "must be ok");
      SDB_ASSERT(sizeof(_upBound) == size, "must be same");

      _lowKey = rocksdb::Slice(_lowBound, size);
      _upKey = rocksdb::Slice(_upBound, size);
      return;
   }

   INT32 lsmIndexIterator::_seekKeyString(const keyString &ks,
                                          BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != _itr, "can not be invalid");

      rocksdb::Slice s(ks.getDataSlice().getData(), ks.getDataSlice().getSize());
      _resetCurrentEntry();

      if (!forPrev)
      {
         _itr->Seek(s);
      }
      else
      {
         _itr->SeekForPrev(s);
      }

      if (!_itr->Valid() && !_itr->status().ok())
      {
         PD_LOG(PDERROR, "failed to seek key:%s", _itr->status().getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (_itr->Valid())
      {
         rc = _initCurrentEntry(_itr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init current entry cache:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lsmIndexIterator::_resetCurrentEntry()
   {
      _ks.reset();
   }

   INT32 lsmIndexIterator::_initCurrentEntry(const rocksdb::Iterator *itr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != itr && itr->Valid(), "can not be invalid");
      slice s(itr->key().size(), itr->key().data());
      rc = _ks.init(s);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse key string:%d", rc);
         goto error;
      }

      ///TODO: validate value
   done:
      return rc;
   error:
      _ks.reset();
      goto done;
   }

   INT32 lsmIndexIterator::_reinitIterator(const options &o, BOOLEAN allInclusive)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_isInited(), "can not be invalid");
      rocksdb::ReadOptions options;

      _ks.reset();

      if (nullptr != _itr)
      {
         delete _itr;
         _itr = nullptr;
      }

      _o.forward = o.forward;
      _o.pointGetOnly = o.pointGetOnly && o.forward && allInclusive;

      options.iterate_lower_bound = &_lowKey;
      options.iterate_upper_bound = &_upKey;

      if (_o.pointGetOnly)
      {
         options.auto_prefix_mode = TRUE;
         options.prefix_same_as_start = TRUE;
      }
      else
      {
         options.auto_prefix_mode = FALSE;
         options.total_order_seek = TRUE;
         options.prefix_same_as_start = FALSE;
      }

      _itr = _cf.newIterator(options);
      if (OSS_UNLIKELY(nullptr == _itr))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine