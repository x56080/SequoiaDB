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
      _lsmDB = NULL;
      if (NULL != _itr)
      {
         delete _itr;
      }
      _lowKey = rocksdb::Slice();
      _upKey = rocksdb::Slice();
   }

   void lsmIndexIterator::close()
   {
      _close();
      indexIteratorKernal::_close();
      return;
   }

   INT32 lsmIndexIterator::open(requestContext *context,
                                 const indexHandle &handle,
                                 const orderingWrapper &ordering,
                                 INT32 direction,
                                 rtnPredicateListIterator *predicate,
                                 memoryBlock &entryBuffer)
   {
      INT32 rc = SDB_OK;
      static const UINT8 minKey = (1 | 64);
      rocksdb::ReadOptions o;
      globalIndexID indexId(context->getLogicalCSID(),
                                 context->getLogicalCLID(),
                                 handle.getIndexId());
      globalIndexID upperIndexId(context->getLogicalCSID(),
                                 context->getLogicalCLID(),
                                 handle.getIndexId() + 1);

      recordID minRid(0, 0);
                        
      close();
      rc = indexIteratorKernal::_open(context, handle, ordering,
                                      direction, predicate, entryBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open iterator kernal:%d", rc);
         goto error;
      }

      rc = lsmPackIndexFullKey(_lowBoundKey,
                               LSM_MIN_FULL_KEY_SIZE,
                               indexId,
                               ordering,
                               ixmKey((const CHAR *)(&minKey)),
                               minRid,
                               0, DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build low bound key:%d", rc);
         goto error;
      }

      rc = lsmPackIndexFullKey(_upperBoundKey,
                               LSM_MIN_FULL_KEY_SIZE,
                               upperIndexId,
                               ordering,
                               ixmKey((const CHAR *)(&minKey)),
                               minRid,
                               0, DPS_TRANS_ID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build upper bound key:%d", rc);
         goto error;
      }

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

   rocksdb::Slice lsmIndexIterator::getLastEntry()const
   {
      rocksdb::Slice entry;
      memoryBlock &buffer = indexIteratorKernal::getEntryBuffer();
      if (!buffer.isEmpty())
      {
         SDB_ASSERT(LSM_MIN_FULL_KEY_SIZE <= buffer.getSize(), "impossible");
         entry = rocksdb::Slice(buffer.getBuffer(), buffer.getSize());
      }

      return entry;
   }
}//namespace vessel
}//namespace engine