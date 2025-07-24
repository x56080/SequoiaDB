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

   Source File Name = baseMetaDataFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/baseMetaDataFile.h"
#include "pdTrace.hpp"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   INT32 baseMetaDataFile::_open(BOOLEAN isCreating)
   {
      INT32 rc = SDB_OK;
      UINT32 pcnt = getCommonHeadInMem().maxPageCountPerSeg;
      UINT64 maxSegCnt = getCommonHeadInMem().maxSegmentCountPerFile;
      UINT64 maxSmeSize = (maxSegCnt * pcnt) >> 3;
      UINT64 smeSize = getCommonHeadInMem().reservedAreaSize;
      
      if (smeSize < maxSmeSize)
      {
         PD_LOG(PDERROR, "invalid sme size[%lld] reserved", smeSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (0 < storageFile::getSegmentCount())
      {
         _loadSme();
      }

   done:
      return rc;
   error:
      goto done;
   }

   void baseMetaDataFile::_close()
   {
      _scanner.reset();
   }

   void baseMetaDataFile::_loadSme()
   {
      SDB_ASSERT(!_scanner.isReady(), "do not reinit");
      UINT32 segmentCount = storageFile::getSegmentCount();
      SDB_ASSERT(0 < segmentCount, "can not be zero");
      UINT32 pcnt = getCommonHeadInMem().maxPageCountPerSeg; 
      UINT64 *sme = reinterpret_cast<UINT64 *>(getReservedAreaPtr());
      SDB_ASSERT(nullptr != sme, "can not be null");
      _scanner.load(sme, pcnt * segmentCount);
   }

   INT32 baseMetaDataFile::_initSme()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == storageFile::getSegmentCount(), "must be zero");
      SDB_ASSERT(!_scanner.isReady(), "do not reinit");
      UINT64 *sme = reinterpret_cast<UINT64 *>(getReservedAreaPtr());
      SDB_ASSERT(nullptr != sme, "can not be null");

      rc = ensureSegmentCount(1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file:%d", rc);
         goto error;
      }

      _scanner.init(sme, getCommonHeadInMem().maxPageCountPerSeg, FALSE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 baseMetaDataFile::reservePid(PAGE_ID &pid, mmapPagePointer *ptr)
   {
      INT32 rc = SDB_OK;
      pid = INVALID_PAGE_ID;
      if (nullptr != ptr)
      {
         ptr->reset();
      }

      std::unique_lock<std::mutex> guard(_mutex);
      if (OSS_UNLIKELY(!_scanner.isReady()))
      {
         rc = _initSme();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init sme:%d", rc);
            goto error;
         }
      }

      do
      {
         INT32 val = 0;
         if (_scanner.findAndClearNext(val))
         {
            pid = static_cast<PAGE_ID>(val);
            break;
         }
         else
         {
            rc = storageFile::allocateNewSegment();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to extend file:%d", rc);
               goto error;
            }

            _scanner.extend(getCommonHeadInMem().maxPageCountPerSeg, FALSE);
         }
      } while (TRUE);

      if (nullptr != ptr)
      {
         rc = storageFile::getPagePtr(pid, *ptr);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get page[%d] ptr:%d", pid, rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _scanner.setBit(pid, TRUE);
      }
      goto done;
   }

   void baseMetaDataFile::freePid(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(_scanner.isReady(), "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);
      _scanner.setBit(pid, FALSE);
   }

   void baseMetaDataFile::freePids(UINT32 size, const PAGE_ID *pids)
   {
      SDB_ASSERT(_scanner.isReady(), "can not be invalid");
      SDB_ASSERT(0 != size && nullptr != pids, "can not be invalid");

      if (1 < size)
      {
         ossPoolVector<PAGE_ID> v;
         v.reserve(size);
         for (UINT32 i = 0; i < size; ++i)
         {
            if (INVALID_PAGE_ID != pids[i])
            {
               v.push_back(pids[i]);
            }
            else
            {
               SDB_ASSERT(FALSE, "can not release invalid pid");
            }
         }

         if (OSS_LIKELY(!v.empty()))
         {
            std::sort(v.begin(), v.end());
            std::unique_lock<std::mutex> guard(_mutex);
            for (UINT32 i = 0; i < v.size(); ++i)
            {
               _scanner.setBit(v.at(i), FALSE);
            }
         }
      }
      else
      {
         freePid(pids[0]);
      }
   }

   void baseMetaDataFile::freePids(const ossPoolSet<PAGE_ID> &set)
   {
      SDB_ASSERT(_scanner.isReady(), "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);
      for (auto itr = set.cbegin(); itr != set.cend(); ++itr)
      {
         _scanner.setBit(*itr, FALSE);
      }
      return;
   }  
} // namespace vessel

} // namespace engine
