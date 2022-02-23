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

   Source File Name = indexObjectMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexObjectMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   indexObjectMap::~indexObjectMap()
   {
      fini();
   }

   void indexObjectMap::fini()
   {
      _OBJECT_MAP::const_iterator itr = _objects.begin();
      for (; itr != _objects.end(); ++itr)
      {
         SDB_OSS_DEL itr->second;
      }
      _objects.clear();

      _nextIndexId = 0;
      _freeIndexSlots = OSS_UINT64_MAX;
      return;
   }

   INT32 indexObjectMap::insert(INT32 indexSlot,
                                 UINT32 indexLid,
                                 PAGE_ID lpid,
                                 const indexDescription &desc,
                                 INDEX_STATUS status,
                                 PAGE_ID btreeRoot)
   {
      INT32 rc = SDB_OK;
      indexObject *obj = NULL;
      indexIdentifier id;

      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       INVALID_LOGICAL_INDEX_ID == indexLid ||
                       INVALID_PAGE_ID == lpid ||
                       !desc.isValid() ||
                       INDEX_STATUS_INVALID == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!isIndexSlotFree(indexSlot))
      {
         PD_LOG(PDERROR, "index slot[%d] not free", indexSlot);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      obj = SDB_OSS_NEW indexObject();
      if (OSS_UNLIKELY(NULL == obj))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = obj->init(indexSlot, indexLid, desc, status, lpid, btreeRoot);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index context:%d", rc);
         goto error;
      }

      rc = insert(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert context to map:%d", rc);
         goto error;
      }

      if (_nextIndexId <= indexLid)
      {
         _nextIndexId = indexLid + 1;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   void indexObjectMap::erase(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      _OBJECT_MAP::const_iterator itr = _objects.find(indexSlot);
      if (_objects.end() != itr)
      {
         indexObject *obj = itr->second;
         _objects.erase(itr);
         SDB_OSS_DEL obj;
      }
      freeIndexSlot(indexSlot);
   }

   BOOLEAN indexObjectMap::isIndexSlotFree(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
      BOOLEAN r = (0 != OSS_BIT_TEST(_freeIndexSlots, mask));
#if defined (_DEBUG)
      if (r)
      {
         SDB_ASSERT(0 == _objects.count(indexSlot), "impossible");
      }
      else
      {
         SDB_ASSERT(0 != _objects.count(indexSlot), "impossible");
      }
#endif//_DEBUG
      return r;
   }

   void indexObjectMap::unfreeIndexSlot(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
#if defined (_DEBUG)
      SDB_ASSERT(0 != OSS_BIT_TEST(_freeIndexSlots, mask), "must be free");
#endif//_DEBUG
      OSS_BIT_CLEAR(_freeIndexSlots, mask);
      return;
   }

   void indexObjectMap::freeIndexSlot(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
#if defined (_DEBUG)
      SDB_ASSERT(0 == OSS_BIT_TEST(_freeIndexSlots, mask), "can not be free");
#endif//_DEBUG
      OSS_BIT_SET(_freeIndexSlots, mask);
   }

   INT32 indexObjectMap::findFreeIndexSlot()const
   {
      return ossGetLowestBit1From64Bits(_freeIndexSlots);
   }

   INT32 indexObjectMap::insert(indexObject *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->isValid(), "must be valid");
      if (!_objects.insert(std::make_pair(obj->getIndexId().getIndexSlot(), obj)).second)
      {
         PD_LOG(PDERROR, "duplicated index slot[%d]", obj->getIndexId().getIndexSlot());
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      unfreeIndexSlot(obj->getIndexId().getIndexSlot());
   done:
      return rc;
   error:
      goto done;
   }

   indexObject *indexObjectMap::find(const indexIdentifier &indexId,
                                     INDEX_STATUS filter)const
   {
      SDB_ASSERT(indexId.isValid(), "can not be invalid");
      indexObject *obj = NULL;
      _OBJECT_MAP::const_iterator itr = _objects.find(indexId.getIndexSlot());
      if (_objects.end() != itr)
      {
         if (itr->second->getIndexId() == indexId)
         {
            if (INDEX_STATUS_INVALID == filter ||
                  filter == itr->second->getStatus())
            {
               obj = itr->second;
            }
         }
      }
      return obj;
   }

}//namespace vessel
}//namespace engine