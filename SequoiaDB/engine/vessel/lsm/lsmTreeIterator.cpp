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

   Source File Name = lsmTreeIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmTreeIterator.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexObject.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/lsm/lsmTableFilter.h"
#include "vessel/sliceTransfer.h"
#include "vessel/lsm/lsmIndexEntryValue.h"

namespace engine
{
namespace vessel
{
   lsmTreeIterator::~lsmTreeIterator()
   {
      if (nullptr != _itr)
      {
         delete _itr;
      }
   }

   INT32 lsmTreeIterator::init(const lsmColumnFamily &cf,
                               const lsmIteratorBound *bound)
   {
      INT32 rc = SDB_OK;
      globalIndexID indexId;
                        
      reset();

      if (OSS_UNLIKELY(!cf.isValid() ||
                       nullptr == bound ||
                       !bound->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cf = cf;
      _bound = bound;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void lsmTreeIterator::reset()
   {
      _ks.reset();
      _valueRef.reset();
      _bound = nullptr;
      _cf = lsmColumnFamily();
      if (nullptr != _itr)
      {
         delete _itr;
         _itr = nullptr;
      }
      return;
   }
   
   INT32 lsmTreeIterator::seek(const keyString &ks,
                               BOOLEAN pointGetOptimized)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _reinitInternalItr(pointGetOptimized);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = _seek(ks, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      _resetInternalItr();
      goto done;
   }

   INT32 lsmTreeIterator::seekForPrev(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _reinitInternalItr(FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lsm iterator:%d", rc);
         goto error;
      }

      rc = _seek(ks, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to seek key string:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      _resetInternalItr();
      goto done;
   }

   INT32 lsmTreeIterator::next(BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      _resetCurrentEntry();

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
         rc = _initCurrentEntry();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init current entry cache:%d", rc);
            goto error;
         }
      }
      else if (!_itr->status().ok())
      {
         PD_LOG(PDERROR, "failed to move iterator:%s", _itr->status().getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmTreeIterator::advance(const keyString &ks, BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _seek(ks, forPrev);
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

   BOOLEAN lsmTreeIterator::isMarkedRemoved() const
   {
      SDB_ASSERT(_valueRef.isValid(), "can not be invalid");
      return _valueRef.getValuePtr()->isDeleted();
   }

   UINT64 lsmTreeIterator::getLSN()const
   {
      SDB_ASSERT(_valueRef.isValid(), "can not be invalid");
      return _valueRef.getValuePtr()->lsn;
   }

   DPS_TRANS_ID lsmTreeIterator::getTransID()const
   {
      SDB_ASSERT(_valueRef.isValid(), "can not be invalid");
      return _valueRef.getValuePtr()->transID;
   }

   recordID lsmTreeIterator::getRid()const
   {
      SDB_ASSERT(isReadyToRead(), "must be valid");
      return _ks.getRid();
   }

   INT32 lsmTreeIterator::_seek(const keyString &ks, BOOLEAN forPrev)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ks.isValid(), "can not be invalid");
      SDB_ASSERT(_isInternalItrReady(), "can not be invalid");

      rocksdb::Slice s = toRocksdbSlice(ks.getRawData());
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
         rc = _initCurrentEntry();
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

   INT32 lsmTreeIterator::_initCurrentEntry()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _itr && _itr->Valid(), "can not be invalid");
      slice s(_itr->key().size(), _itr->key().data());
      rc = _ks.init(s);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse key string:%d", rc);
         goto error;
      }

      rc = _valueRef.init(_itr->value(), TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init entry value:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      _resetCurrentEntry();
      goto done;
   }

   void lsmTreeIterator::_resetCurrentEntry()
   {
      _ks.reset();
      _valueRef.reset();
   }

   INT32 lsmTreeIterator::_reinitInternalItr(BOOLEAN pointGetOptimized)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      rocksdb::ReadOptions o;

      _ks.reset();

      _resetInternalItr();

      o.iterate_lower_bound = _bound->getLowBound();
      o.iterate_upper_bound = _bound->getUpBound();
      if (pointGetOptimized)
      {
         o.auto_prefix_mode = FALSE;
         o.total_order_seek = FALSE;
         o.prefix_same_as_start = TRUE;
      }
      else
      {
         o.auto_prefix_mode = FALSE;
         o.total_order_seek = TRUE;
         o.prefix_same_as_start = FALSE;
      }

      o.table_filter = lsmTableFilter(rocksdb::Slice(_bound->getLowBound()->data(),
                                                     keyStringCoder::INDEX_ID_ENCODEING_SIZE));

      _itr = _cf.newIterator(o);
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

   void lsmTreeIterator::_resetInternalItr()
   {
      if (nullptr != _itr)
      {
         ///TODO: how about call Refresh()?
         delete _itr;
         _itr = nullptr;
      }
   }
} // namespace vessel

} // namespace engine
