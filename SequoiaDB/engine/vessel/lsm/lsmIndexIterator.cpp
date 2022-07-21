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
      opt.auto_prefix_mode = FALSE;
      opt.total_order_seek = TRUE;
      ///TODO: table filter

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
                                   const inclusiveVec &matchInclusive)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 lsmIndexIterator::seek(const VEC_ELE_CMP &matchEles,
                                const inclusiveVec &matchInclusive)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(matchEles.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::seek(const bson::BSONObj &key,
                                const inclusiveVec &matchInclusive)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 lsmIndexIterator::locate(const indexEntryLocation *location)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
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
      SDB_ASSERT(FALSE, "TODO"); 
      return recordID();
   }

   INT32 lsmIndexIterator::next()
   {
      INT32 rc = SDB_OK;
      if (!_isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lsmIndexIterator::pause(IDX_ENTRY_LOCATION_UPTR &location)
   {
      SDB_ASSERT(FALSE, "TODO"); 
      return SDB_OK;
   }

   INT32 lsmIndexIterator::resume(const indexEntryLocation *location)
   {
      SDB_ASSERT(FALSE, "TODO"); 
      return SDB_OK;
   }

   bson::BSONObj lsmIndexIterator::getKeyObj(BOOLEAN withFieldName,
                                             bson::BufBuilder *buf)const
   {
      SDB_ASSERT(FALSE, "TODO");
      return bson::BSONObj();
   }

   INT32 lsmIndexIterator::initOrUpdateLocation(IDX_ENTRY_LOCATION_UPTR &location) const
   {
      SDB_ASSERT(FALSE, "TODO"); 
      return SDB_OK;
   }

   void lsmIndexIterator::_moveIterator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _itr && _itr->Valid(), "can not be invalid");
      if (_o.forward)
      {
         _itr->Next();
      }
      else
      {
         _itr->Prev();
      }
   }

   void lsmIndexIterator::_initKeyBoundWhenOpen(const globalIndexID &id)
   {
      SDB_ASSERT(id.isValid(), "can not be invalid");
      _lowBound.init(id);
      _upBound.initAsUpKey(id);
      _lowKey = rocksdb::Slice(_lowBound.getData(), _lowBound.getSize());
      _upKey = rocksdb::Slice(_upBound.getData(), _upBound.getSize());
      return;
   }

}//namespace vessel
}//namespace engine