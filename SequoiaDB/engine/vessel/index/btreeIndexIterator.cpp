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

   Source File Name = btreeIndexIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexIterator.h"
#include "vessel/indexUtils.h"
#include "ixmKey.hpp"
#include "vessel/btreeScanEntryParser.h"

namespace engine
{
namespace vessel
{
   btreeIndexIterator::btreeIndexIterator()
   {}

   btreeIndexIterator::~btreeIndexIterator()
   {}

   void btreeIndexIterator::close()
   {
      _item.fini();
      _node = btreeNode();
      _bac.fini();
      _accessor.fini();
      _builder.reset();
   }

   INT32 btreeIndexIterator::open(requestContext *context,
                                  indexContext *ic)
   {
      INT32 rc = SDB_OK;
      close();
      if (OSS_UNLIKELY(NULL == context ||
                       DMS_INVALID_LOGICCSID == context->getLogicalCSID() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       NULL == ic ||
                       !ic->isValid() ||
                       INDEX_TYPE_BTREE != ic->getIndexType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _accessor.init(context, ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init btree accessor:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   BOOLEAN btreeIndexIterator::isReadyToRead()const
   {
      return _item.isValid();
   }

   INT32 btreeIndexIterator::advanceTo(const bson::BSONObj &prevKey,
                                       INT32 fieldCountToCmpInPrev,
                                       const VEC_ELE_CMP &matchEles,
                                       const inclusiveVec &matchInclusive,
                                       const options &o)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj keyObj;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::seekKey(const ixmKey &key,
                                     const options &o)
   {
      INT32 rc = SDB_OK;

      
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::seek(const bson::BSONObj &prevKey,
                                  INT32 fieldCountToCmpInPrev,
                                  const VEC_ELE_CMP &matchEles,
                                  const inclusiveVec &matchInclusive,
                                  const options &o)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 btreeIndexIterator::seekEntry(const slice &entry,
                                       const options &o)
   {
      return SDB_OK;
   }

   void btreeIndexIterator::resetToSeek()
   {
      _item.fini();
      _node = btreeNode();
      _bac.fini();
      _builder.reset();
      return;
   }

   INT32 btreeIndexIterator::next()
   {
      INT32 rc = SDB_OK;

      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 btreeIndexIterator::prev()
   {
      INT32 rc = SDB_OK;

      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   UINT64 btreeIndexIterator::getLSN()const
   {
      SDB_ASSERT(_node.isValid(), "can not be invalid");
      return _node.getBuffer()->getRuntimeBuffer().getPageHead()->lsn;
   }
   slice btreeIndexIterator::getKey()const
   {
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return slice();
   }
   DPS_TRANS_ID btreeIndexIterator::getTransID()const
   {
      return DPS_TRANS_ID();
   }
   recordID btreeIndexIterator::getRid()const
   {
      return recordID();
   }

   INT32 btreeIndexIterator::pushCurrentEntryToBatch(indexScanEntryBatch &batch)const
   {
      INT32 rc = SDB_OK;
      btreeScanEntryParser parser;


      if (!isReadyToRead())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN btreeIndexIterator::equalToCurrentKey(const ixmKey &key)const
   {
      return FALSE;
   }

   INT32 btreeIndexIterator::cacheSeekResult(const btreeItemLocation &location)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(location.isValid(), "can not be invalid");

      _node = _bac.getEndNodeInPath();
      SDB_ASSERT(_node.isValid(), "impossible");
      rc = _node.getItem(location.slotPos, _item);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index item:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      resetToSeek();
      goto done;
   }

   void btreeIndexIterator::pause()
   {

   }

   indexScanEntry btreeIndexIterator::getCurrentEntry()const
   {
      return indexScanEntry();
   }
} // namespace vessel

} // namespace engine
