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

   Source File Name = collectionIndexContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionIndexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/unstableIndexContext.h"

namespace engine
{
namespace vessel
{
   collectionIndexContext::~collectionIndexContext()
   {
      fini();
   }

   void collectionIndexContext::fini()
   {
      _UNSTABLE_INDEX_MAP::const_iterator itr = _unstatbleIndexMap.begin();
      for (; itr != _unstatbleIndexMap.end(); ++itr)
      {
         SDB_OSS_DEL itr->second;
      }
      _unstatbleIndexMap.clear();
      _nextIndexId = 0;
      _uniqueIndexes = 0;
      _nonUniqueIdexes = 0;
      return;
   }

   void collectionIndexContext::incNextIndexId()
   {
      if (OSS_LIKELY(INVALID_LOGICAL_INDEX_ID != _nextIndexId))
      {
         ++_nextIndexId;
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
      }
      return;
   }

   INT32 collectionIndexContext::findFreeIndexSlot()const
   {
      SDB_ASSERT(64 == MAX_INDEX_COUNT_PER_CL, "must be 64");
      INT32 indexSlot = -1;
      UINT64 bitmap = getIndexSlotBitmap();
      UINT64 mask = 1;
      
      if (OSS_UINT64_MAX == bitmap)
      {
         goto done;
      }

      for (INT32 i = 0; i < (INT32)MAX_INDEX_COUNT_PER_CL; ++i)
      {
         if (0 == OSS_BIT_TEST(bitmap, mask))
         {
            indexSlot = i;
            break;
         }
         mask <<= 1;
      }

   done:
      return indexSlot;
   }

   void collectionIndexContext::unfreeIndexSlot(INT32 indexSlot, BOOLEAN isUnique)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UINT64 mask = 1;
      mask <<= indexSlot;
      if (isUnique)
      {
         OSS_BIT_SET(_uniqueIndexes, mask);
      }
      else
      {
         OSS_BIT_SET(_nonUniqueIdexes, mask);
      }
      return;
   }

   INT32 collectionIndexContext::insertUnstableIndex(INT32 indexSlot,
                                                     INDEX_STATUS status,
                                                     const indexObject &obj,
                                                     unstableIndexContext **out)
   {
      INT32 rc = SDB_OK;
      unstableIndexContext *uic = NULL;
      UINT64 mask = 1;

      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       !obj.isValid() ||
                       INDEX_STATUS_INVALID == status ||
                       INDEX_STATUS_NORMAL == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      mask <<= indexSlot;

      uic = SDB_OSS_NEW unstableIndexContext();
      if (NULL == uic)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = uic->init(indexSlot, obj, status);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init context:%d", rc);
         goto error;
      }

      if (!_unstatbleIndexMap.insert(std::make_pair(indexSlot, uic)).second)
      {
         PD_LOG(PDERROR, "failed to insert index slot[%d] into map");
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      if (NULL != out)
      {
         *out = uic;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(uic);
      goto done;
   }

   BOOLEAN collectionIndexContext::eraseUnstableIndex(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      BOOLEAN r = FALSE;
      _UNSTABLE_INDEX_MAP::const_iterator itr = _unstatbleIndexMap.find(indexSlot);
      if (_unstatbleIndexMap.end() != itr)
      {
         SDB_OSS_DEL itr->second;
         _unstatbleIndexMap.erase(itr);
         r = TRUE;
      }
      return r;
   }

   unstableIndexContext *collectionIndexContext::findUnstableIndex(INT32 indexSlot)const
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      unstableIndexContext *uic = NULL;
      _UNSTABLE_INDEX_MAP::const_iterator itr = _unstatbleIndexMap.find(indexSlot);
      if (_unstatbleIndexMap.end() != itr)
      {
         uic = itr->second;
      }
      return uic;
   }

   INT32 collectionIndexContext::unfreeSlotAndSetBuilding(INT32 indexSlot,
                                                          const indexObject &obj,
                                                          unstableIndexContext **out)
   {
      INT32 rc = SDB_OK;
      UINT64 mask = 1;
      UINT64 bitmap = 0;

      if (!isValidIndexSlot(indexSlot) ||
          !obj.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      mask <<= indexSlot;
      bitmap = getIndexSlotBitmap();
      if (0 != OSS_BIT_TEST(bitmap, mask))
      {
         PD_LOG(PDERROR, "index slot[%d] is unfree", indexSlot);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = insertUnstableIndex(indexSlot, INDEX_STATUS_BUILDING, obj, out);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert unstable index[%d]:%d", indexSlot, rc);
         goto error;
      }

      if (obj.getParams().isUnique)
      {
         OSS_BIT_SET(_uniqueIndexes, mask);
      }
      else
      {
         OSS_BIT_SET(_nonUniqueIdexes, mask);
      }

   done:
      return rc;
   error:
      if (NULL != out)
      {
         *out = NULL;
      }
      goto done;
   }

   BOOLEAN collectionIndexContext::freeSlotAndEraseUnstableIndex(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      BOOLEAN r = FALSE;
      _UNSTABLE_INDEX_MAP::const_iterator itr = _unstatbleIndexMap.find(indexSlot);
      if (_unstatbleIndexMap.end() != itr)
      {
         UINT64 mask = 1;
         mask <<= indexSlot;
         BOOLEAN isUnique = itr->second->getIndexObj().getParams().isUnique;
         SDB_OSS_DEL itr->second;
         _unstatbleIndexMap.erase(itr);

         if (isUnique)
         {
            OSS_BIT_CLEAR(_uniqueIndexes, mask);
         }
         else
         {
            OSS_BIT_CLEAR(_nonUniqueIdexes, mask);
         }
         r = TRUE;
      }
      return r;
   }

   UINT32 collectionIndexContext::getUnfreeIndexSlotCount()const
   {
      return ossGetNonZeroBitCount64(getIndexSlotBitmap());
   }

   UINT32 collectionIndexContext::getNormalIndexCount()const
   {
      UINT32 count = ossGetNonZeroBitCount64(getIndexSlotBitmap());
      SDB_ASSERT(_unstatbleIndexMap.size() <= count, "impossible");
      return count - _unstatbleIndexMap.size();
   }
}//namespace vessel
}//namespace engine