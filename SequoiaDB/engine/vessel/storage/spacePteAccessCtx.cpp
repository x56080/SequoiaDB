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

   Source File Name = spacePteAccessCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/spacePteAccessCtx.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   void spacePteAccessCtx::exportDirtySegments(const storageCoreArgs &args,
                                               ossPoolSet<UINT32> &segments)const
   {
      SDB_ASSERT(args.isValid(), "can not be invalid");
      for (auto itr = _pmap.cbegin(); itr != _pmap.cend(); ++itr)
      {
         if (INVALID_PAGE_ID != itr->second)
         {
            segments.insert(itr->second / args.maxPageCountPerSeg);
         }
      }
      return;
   }

   void spacePteAccessCtx::exportDirtyFiles(const storageCoreArgs &args,
                                            ossPoolSet<UINT32> &files)const
   {
      SDB_ASSERT(args.isValid(), "can not be invalid");
      UINT32 pcnt = args.getMaxPageCountInFile();
      for (auto itr = _pmap.cbegin(); itr != _pmap.cend(); ++itr)
      {
         if (INVALID_PAGE_ID != itr->second)
         {
            files.insert(itr->second / pcnt);
         }
      }
      return;
   }

   void spacePteAccessCtx::exportDirtyPids(sparseBitmap32 &pids)const
   {
      for (auto itr = _pmap.cbegin(); itr != _pmap.cend(); ++itr)
      {
         if (INVALID_PAGE_ID != itr->second)
         {
            pids.set(itr->second);
         }
      }

      return;
   }

   void spacePteAccessCtx::set(PAGE_ID lpid, PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      _pmap[lpid] = pid;
   }

   void spacePteAccessCtx::reset(PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      _pmap[lpid] = INVALID_PAGE_ID;
   }

   BOOLEAN spacePteAccessCtx::get(PAGE_ID lpid, PAGE_ID &pid) const
   {
      _P_MAPPING::const_iterator itr = _pmap.find(lpid);
      if (_pmap.cend() == itr)
      {
         return FALSE;
      }
      else
      {
         pid = itr->second;
         return TRUE;
      }
   }

   BOOLEAN spacePteAccessCtx::isPrivate(PAGE_ID lpid) const
   {
      auto itr = _pmap.find(lpid);
      return _pmap.cend() != itr && INVALID_PAGE_ID != itr->second;
   }

   void spacePteAccessCtx::erase(PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      _pmap.erase(lpid);
   }

   void spacePteAccessCtx::obsoleteLpid(PAGE_ID lpid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      BOOLEAN old = FALSE;
      _obsoleteLpids.set(lpid, &old);
      SDB_ASSERT(!old, "duplicated lpid");
   }

   void spacePteAccessCtx::obsoletePid(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      BOOLEAN old = FALSE;
      _obsoletePids.set(pid, &old);
      SDB_ASSERT(!old, "duplicated pid");
   }

   void spacePteAccessCtx::resetObsoleteResources()
   {
      _obsoleteLpids.reset();
      _obsoletePids.reset();
   }
} // namespace vessel

} // namespace engine
