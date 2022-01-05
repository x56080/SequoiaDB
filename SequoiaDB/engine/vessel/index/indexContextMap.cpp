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

   Source File Name = indexContextMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexContextMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   indexContextMap::~indexContextMap()
   {
      fini();
   }

   void indexContextMap::fini()
   {
      _CONTEXT_MAP::const_iterator itr = _contexts.begin();
      for (; itr != _contexts.end(); ++itr)
      {
         SDB_OSS_DEL itr->second;
      }
      _contexts.clear();

      _nextIndexId = 0;
      _freeIndexSlots = OSS_UINT64_MAX;
      return;
   }

   INT32 indexContextMap::insert(INT32 indexSlot,
                                 PAGE_ID lpid,
                                 const indexObject &obj,
                                 INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      indexContext *ic = NULL;

      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                        INVALID_PAGE_ID == lpid ||
                        !obj.isValid() ||
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

      ic = SDB_OSS_NEW indexContext();
      if (OSS_UNLIKELY(NULL == ic))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = ic->init(indexSlot, lpid, obj, status);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index context:%d", rc);
         goto error;
      }

      rc = insert(ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert context to map:%d", rc);
         goto error;
      }

      if (_nextIndexId <= obj.getIndexID())
      {
         _nextIndexId = obj.getIndexID() + 1;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(ic);
      goto done;
   }

   void indexContextMap::erase(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      _CONTEXT_MAP::const_iterator itr = _contexts.find(indexSlot);
      if (_contexts.end() != itr)
      {
         indexContext *ic = itr->second;
         _contexts.erase(itr);
         SDB_OSS_DEL ic;
      }
      freeIndexSlot(indexSlot);
   }

   BOOLEAN indexContextMap::isIndexSlotFree(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
      BOOLEAN r = (0 != OSS_BIT_TEST(_freeIndexSlots, mask));
#if defined (_DEBUG)
      if (r)
      {
         SDB_ASSERT(0 == _contexts.count(indexSlot), "impossible");
      }
      else
      {
         SDB_ASSERT(0 != _contexts.count(indexSlot), "impossible");
      }
#endif//_DEBUG
      return r;
   }

   void indexContextMap::unfreeIndexSlot(INT32 indexSlot)
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

   void indexContextMap::freeIndexSlot(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
#if defined (_DEBUG)
      SDB_ASSERT(0 == OSS_BIT_TEST(_freeIndexSlots, mask), "can not be free");
#endif//_DEBUG
      OSS_BIT_SET(_freeIndexSlots, mask);
   }

   INT32 indexContextMap::findFreeIndexSlot()const
   {
      return ossGetLowestBit1From64Bits(_freeIndexSlots);
   }

   INT32 indexContextMap::insert(indexContext *ic)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != ic, "can not be null");
      SDB_ASSERT(ic->isValid(), "must be valid");
      if (!_contexts.insert(std::make_pair(ic->getIndexSlot(), ic)).second)
      {
         PD_LOG(PDERROR, "duplicated index slot[%d]", ic->getIndexSlot());
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      unfreeIndexSlot(ic->getIndexSlot());
   done:
      return rc;
   error:
      goto done;
   }

   indexContext *indexContextMap::find(INT32 indexSlot,
                                       INDEX_STATUS filter)const
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      indexContext *ic = NULL;
      _CONTEXT_MAP::const_iterator itr = _contexts.find(indexSlot);
      if (_contexts.end() != itr)
      {
         if (INDEX_STATUS_INVALID == filter ||
             filter == itr->second->getStatus())
         {
            ic = itr->second;
         }
      }
      return ic;
   }

}//namespace vessel
}//namespace engine