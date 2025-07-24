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

   Source File Name = hybridTreeIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/hybridTreeIterator.h"
#include "vessel/indexSpace.h"
#include "vessel/indexObject.h"
#include "vessel/requestContext.h"
#include "vessel/collectionProperties.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/indexObject.h"
#include "vessel/indexUtils.h"

namespace engine
{
namespace vessel
{
   hybridTreeIterator::~hybridTreeIterator()
   {

   }

   INT32 hybridTreeIterator::init(requestContext *context,
                                  indexSpace *is,
                                  const lsmColumnFamily &cf,
                                  indexObject *obj)
   {
      INT32 rc = SDB_OK;
      globalIndexID indexId;

      reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isClPropertiesSet() ||
                       !cf.isValid() ||
                       nullptr == obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _obj = obj;
      _is = is;
      _context = context;
      _cf = cf;
      indexId.reset(context->getClProperties()->csproperties->csid.getLid(),
                    context->getLogicalClId(),
                    obj->getLogicalID());
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      rc = _bound.init(indexId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init key bound:%d", rc);
         goto error;
      }

      rc = _reinitInternalItrs();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init internal itrs:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void hybridTreeIterator::reset()
   {
      _resetRing();
      _obj = nullptr;
      _is = nullptr;
      _context = nullptr;
      _cf = lsmColumnFamily();
      _lsm.reset();
      _bound.reset();
      _btree.reset();
      _o = options();
      return;
   }

   INT32 hybridTreeIterator::seek(const VEC_ELE_CMP &eles,
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
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = builder.buildPredicate(eles,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, o.forward, _bound.getEncodedIndexId());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      _o = o;
      if (_o.pointGetOptimized && !iv.allInclusive())
      {
         _o.pointGetOptimized = FALSE;
      }

      if (_o.forward)
      {
         rc = _seek(builder.getShallowKeyString());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _seekForPrev(builder.getShallowKeyString());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek for prev:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 hybridTreeIterator::seek(const bson::BSONObj &key,
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
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = builder.buildPredicate(key,
                                  _obj->getProperties().getPattern().getOrdering(),
                                  iv, o.forward, _bound.getEncodedIndexId());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      _o = o;
      if (_o.pointGetOptimized && !iv.allInclusive())
      {
         _o.pointGetOptimized = FALSE;
      }

      if (_o.forward)
      {
         rc = _seek(builder.getShallowKeyString());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _seekForPrev(builder.getShallowKeyString());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek for prev:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   // INT32 hybridTreeIterator::equal(const bson::BSONObj &key)
   // {
   //    INT32 rc = SDB_OK;
   //    STACK_KEY_STRING_BUILDER builder;

   //    if (OSS_UNLIKELY(!key.isValid()))
   //    {
   //       rc = SDB_INVALIDARG;
   //       goto error;
   //    }
   //    else if (OSS_UNLIKELY(!isValid()))
   //    {
   //       rc = SDB_VESSEL_RESOURCES_NOT_INIT;
   //       goto error;
   //    }

   //    rc = builder.buildPredicate(key,
   //                                _obj->getProperties().getPattern().getOrdering(),
   //                                inclusiveVec(), TRUE, _bound.getEncodedIndexId());
   //    if (SDB_OK != rc)
   //    {
   //       PD_LOG(PDERROR, "failed to build predicate:%d", rc);
   //       goto error;
   //    }

   //    _o = options();
   //    _o.pointGetOptimized = TRUE;
   //    rc = _seek(builder.getShallowKeyString());
   //    if (SDB_OK != rc)
   //    {
   //       PD_LOG(PDERROR, "failed to seek:%d", rc);
   //       goto error;
   //    }

   // done:
   //    return rc;
   // error:
   //    reset();
   //    goto done;
   // }

   // INT32 hybridTreeIterator::equal(const VEC_ELE_CMP &matchEles)
   // {
   //    INT32 rc = SDB_OK;
   //    STACK_KEY_STRING_BUILDER builder;

   //    if (OSS_UNLIKELY(matchEles.empty()))
   //    {
   //       rc = SDB_INVALIDARG;
   //       goto error;
   //    }
   //    else if (OSS_UNLIKELY(!isValid()))
   //    {
   //       rc = SDB_VESSEL_RESOURCES_NOT_INIT;
   //       goto error;
   //    }

   //    rc = builder.buildPredicate(matchEles,
   //                                _obj->getProperties().getPattern().getOrdering(),
   //                                inclusiveVec(), TRUE, _bound.getEncodedIndexId());
   //    if (SDB_OK != rc)
   //    {
   //       PD_LOG(PDERROR, "failed to build predicate:%d", rc);
   //       goto error;
   //    }

   //    _o = options();
   //    _o.pointGetOptimized = TRUE;
   //    rc = _seek(builder.getShallowKeyString());
   //    if (SDB_OK != rc)
   //    {
   //       PD_LOG(PDERROR, "failed to seek:%d", rc);
   //       goto error;
   //    }
      
   // done:
   //    return rc;
   // error:
   //    reset();
   //    goto done;
   // }

   INT32 hybridTreeIterator::locateNext(const indexEntryLocation *location,
                                        const options &o)
   {
      INT32 rc = SDB_OK;
      const _location *l = nullptr;

      if (OSS_UNLIKELY(nullptr == location ||
                       !location->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _o = o;
      SDB_ASSERT(IDX_ENTRY_LOCATION_TYPE::HIT == location->getType(), "can not be invalid");
      l = static_cast<const _location *>(location);
      rc = _locateNext(l);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate next:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 hybridTreeIterator::next()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!_isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _refillRingAndPick(TRUE);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
         goto done;
      }
      else
      {
         PD_LOG(PDERROR, "failed to fetch next:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 hybridTreeIterator::advance(const bson::BSONObj &prevKey,
                                     INT32 fieldCountToCmpInPrev,
                                     const VEC_ELE_CMP &matchEles,
                                     const inclusiveVec &iv)
   {
      INT32 rc = SDB_OK;
      inclusiveVec niv;
      bson::BSONObj obj;
      STACK_KEY_STRING_BUILDER builder;

      if (OSS_UNLIKELY(!_isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(fieldCountToCmpInPrev < 0 ||
                            matchEles.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!_o.pointGetOptimized, "can not be point get");
      obj = indexUtils::buildKeyToSeek(prevKey, fieldCountToCmpInPrev, matchEles);
      niv = iv;
      niv.setBatch(0, fieldCountToCmpInPrev - 1, TRUE);
      rc = builder.buildPredicate(obj,
                                 _obj->getOrderingWrapper(),
                                 niv, _o.forward, _bound.getEncodedIndexId());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build predicate:%d", rc);
         goto error;
      }

      rc = _advance(builder.getShallowKeyString());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance iterator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::pause()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _resetRing();
      _resetInternalItrs();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::resume()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_isPaused())
      {
         rc = _reinitInternalItrs();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reinit itrs:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   bson::BSONObj hybridTreeIterator::getKeyObj(BOOLEAN withFieldName,
                                               bson::BufBuilder *buf)const
   {
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      const bson::BSONObj pattern = _obj->getProperties().getPattern().getPattern();
      if (nullptr == buf)
      {
         return _getCurrent().toBSON(pattern, withFieldName);
      }
      else
      {
         return _getCurrent().toBSON(pattern, *buf, withFieldName);
      }
   }

   DPS_LSN_OFFSET hybridTreeIterator::getLSN()const
   {
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      return (_RING_POS::BTREE == _pos) ?
             _btree.getLSN() : _lsm.getLSN();
   }

   DPS_TRANS_ID hybridTreeIterator::getTransID()const
   {
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      return (_RING_POS::BTREE == _pos) ?
             _btree.getTransID() : _lsm.getTransID();
   }

   recordID hybridTreeIterator::getRid()const
   {
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      return _getCurrent().getRid();
   }

   keyString hybridTreeIterator::getCurrentKeyString()const
   {
      SDB_ASSERT(_isReadyToRead(), "can not be invalid");
      return _getCurrent();
   }

   INT32 hybridTreeIterator::initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location)const
   {
      INT32 rc = SDB_OK;
      _location *l = nullptr;
      slice s;

      if (OSS_UNLIKELY(!_isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!location)
      {
         location.reset(SDB_OSS_NEW _location());
         if (OSS_UNLIKELY(!location))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
      }

      l = static_cast<_location *>(location.get());
      l->reset();
      s = _getCurrent().getRawData();
      l->entry.insert(0, s.data(), s.size());
      l->current = _pos;
      if (_btree.isReadyToRead())
      {
         l->bl = std::move(_btree.getLocation());
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::_seek(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      SDB_ASSERT(_o.forward, "must be forward");
      _resetRing();

      rc = _lsm.seek(ks, _o.pointGetOptimized);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek in lsm:%d", rc);
         goto error;
      }

      rc = _btree.seek(ks);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek in btree:%d", rc);
         goto error;
      }

      rc = _refillRingAndPick(FALSE);
      if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to refill ring and pick:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::_seekForPrev(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      _resetRing();

      rc = _lsm.seekForPrev(ks);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek for prev in lsm:%d", rc);
         goto error;
      }

      rc = _btree.seekForPrev(ks);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek for perv in btree:%d", rc);
         goto error;
      }

      rc = _refillRingAndPick(FALSE);
      if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to refill ring and pick:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::_locateNext(const _location *l)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != l && l->isValid(), "can not be invalid");
      STACK_KEY_STRING_BUILDER builder;
      keyString ks(l->entry.size(), l->entry.data());
      keyString target;
      _resetRing();

      if (!ks.isValid())
      {
         PD_LOG(PDERROR, "invalid key string to parse:%d", rc);
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }

      if (_o.forward || !ks.hasKeyHead())
      {
         recordID rid = ks.getRid();
         if (_o.forward)
         {
            rid.setPos(rid.getPos() + 1);
         }
         rc = builder.rebuildEntryKey(ks, rid, _bound.getEncodedIndexId());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to rebuild entry key:%d", rc);
            goto error;
         }
         target = builder.getShallowKeyString();
      }
      else
      {
         target = ks;
      }

      rc = _resumeBtreeLocation(*l, target);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to resume btree location:%d", rc);
         goto error;
      }

      rc = _o.forward ? _lsm.seek(target, _o.pointGetOptimized) : _lsm.seekForPrev(target);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to resume lsm location:%d", rc);
         goto error;
      }

      rc = _refillRingAndPick(FALSE);
      if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to refill ring and pick:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::_resumeBtreeLocation(const _location &l,
                                                  const keyString &ks)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_btree.isValid(), "can not be invalid");

      if (l.bl.isValid() && l.bl.getTransferTick() == _btree.getTransferTick())
      {
         rc = _btree.locate(l.bl);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to relocate btree:%d", rc);
            goto error;
         }
         
         if (_RING_POS::BTREE == l.current)
         {
            rc = _btree.next(_o.forward);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to move btree iterator:%d", rc);
               goto error;
            }
         }

         goto done;
      }

      rc = _o.forward ? _btree.seek(ks) : _btree.seekForPrev(ks);
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

   void hybridTreeIterator::_resetRing()
   {
      _status = _FILLING_STATUS::BOTH_EXPECTED;
      _pos = _RING_POS::INVALID;
      for (UINT32 i = 0; i < _ring.size(); ++i)
      {
         _ring[i].reset();
      }
      return;
   }

   INT32 hybridTreeIterator::_refillRingAndPick(BOOLEAN fetchNext)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      BOOLEAN next = fetchNext;

      do
      {
         rc = _refillRing(next);
         if (SDB_OK == rc)
         {
            _RING_PICK_RES res = _pickFromRing();
            if (res.isPicked())
            {
               _status = res.status;
               _pos = res.pos;
               break;
            }
            else if (res.needRefill())
            {
               next = TRUE;
               continue;
            }
            else
            {
               PD_LOG(PDERROR, "failed to pick from ring");
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
         else
         {
            goto error;
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      _resetRing();
      goto done;
   }

   INT32 hybridTreeIterator::_refillRing(BOOLEAN fetchNext)
   {
      INT32 rc = SDB_OK;
      
      if (0 != OSS_BIT_TEST(_status, _FILLING_STATUS::LSM_EXPECTED))
      {
         _ring[_RING_POS::LSM].reset();
         if (fetchNext)
         {
            rc = _lsm.next(_o.forward);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch next from lsm tree:%d", rc);
               goto error;
            }
         }

         if (_lsm.isReadyToRead())
         {
            _ring[_RING_POS::LSM] = _lsm.getCurrentEntry();
         }
         else
         {
            OSS_BIT_CLEAR(_status, _FILLING_STATUS::LSM_EXPECTED);
         }
      }

      if (0 != OSS_BIT_TEST(_status, _FILLING_STATUS::BTREE_EXPECTED))
      {
         _ring[_RING_POS::BTREE].reset();
         if (fetchNext)
         {
            rc = _btree.next(_o.forward);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch next from btree:%d", rc);
               goto error;
            }
         }
         if (_btree.isReadyToRead())
         {
            _ring[_RING_POS::BTREE] = _btree.getEntry();
            // bson::BSONObj obj = _btree.getEntry().toBSON(_obj->getProperties().getPattern().getPattern(), FALSE);
            // PD_LOG(PDDEBUG, "btree entry:%s", obj.toPoolString(false, true, true).c_str());
         }
         else
         {
            OSS_BIT_CLEAR(_status, _FILLING_STATUS::BTREE_EXPECTED);
            
         }
      }

      if (!_ring[_RING_POS::LSM].isValid() &&
          !_ring[_RING_POS::BTREE].isValid())
      {
         rc = SDB_IXM_EOC;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   hybridTreeIterator::_RING_PICK_RES hybridTreeIterator::_pickFromRing() const
   {
      _RING_PICK_RES res;
      if (_ring[_RING_POS::LSM].isValid() &&
          _ring[_RING_POS::BTREE].isValid())
      {
         slice lkey = _ring[_RING_POS::LSM].getKeySliceAfterHeader();
         slice bkey = _ring[_RING_POS::BTREE].getKeySlice();
         INT32 cmp = lkey.compare(bkey);
         if (0 == cmp)
         {
            BOOLEAN removed = _lsm.isMarkedRemoved();
            res.pos = removed ? _RING_POS::INVALID : _RING_POS::LSM;
            res.status = _FILLING_STATUS::BOTH_EXPECTED;
         }
         else
         {
            const INT32 direction = _o.getDirection();
            if (direction * cmp < 0)
            {
               res.pos = _lsm.isMarkedRemoved() ?
                         _RING_POS::INVALID : _RING_POS::LSM;
               res.status = _FILLING_STATUS::LSM_EXPECTED;
            }
            else
            {
               res.pos = _RING_POS::BTREE;
               res.status = _FILLING_STATUS::BTREE_EXPECTED;
            }
         }
      }
      else if (_ring[_RING_POS::LSM].isValid())
      {
         res.pos = _lsm.isMarkedRemoved() ?
                   _RING_POS::INVALID : _RING_POS::LSM;
         res.status = _FILLING_STATUS::LSM_EXPECTED;
      }
      else if (_ring[_RING_POS::BTREE].isValid())
      {
         res.pos = _RING_POS::BTREE;
         res.status = _FILLING_STATUS::BTREE_EXPECTED;
      }
      else
      {
         res.pos = _RING_POS::INVALID;
         res.status = _FILLING_STATUS::NONE_EXPECTED;
         SDB_ASSERT(FALSE, "ring can not be empty");
      }

      return res;
   }

   INT32 hybridTreeIterator::_advance(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      _resetRing();

      rc = _lsm.advance(ks, !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance lsm itr:%d", rc);
         goto error;
      }

      rc = _btree.advance(ks, !_o.forward);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to advance btree itr:%d", rc);
         goto error;
      }

      rc = _refillRingAndPick(FALSE);
      if (SDB_IXM_EOC == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to refill ring and pick:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 hybridTreeIterator::_reinitInternalItrs()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _obj, "can not be invalid");
      SDB_ASSERT(nullptr != _is, "can not be invalid");
      SDB_ASSERT(nullptr != _context, "can not be invalid");
      SDB_ASSERT(_cf.isValid(), "can not be invalid");
      SDB_ASSERT(_bound.isValid(), "can not be invalid");

      _resetInternalItrs();

      /// always init btree iterator first, to lock btree view.
      rc = _is->initViewer(TRUE, _viewer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init viewer:%d", rc);
         goto error;
      }

      rc = _btree.init(_context, _is, _obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree iterator:%d", rc);
         goto error;
      }

      rc = _lsm.init(_cf, &_bound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      _lsm.reset();
      goto done;
   }

   void hybridTreeIterator::_resetInternalItrs()
   {
      SDB_ASSERT(!_isReadyToRead(), "reset ring first");
      _lsm.reset();
      _btree.reset();
      _viewer.reset();
   }

   INT32 hybridTreeIterator::seek(const keyString &key, const options &o)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      _o = o;
      if (_o.forward)
      {
         rc = _seek(key);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _seekForPrev(key);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to seek for prev:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }
} // namespace vessel

} // namespace engine
