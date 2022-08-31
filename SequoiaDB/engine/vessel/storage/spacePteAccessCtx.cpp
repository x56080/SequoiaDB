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

   Source File Name = spacePteAccessCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
      _obsoleteLpids.push(lpid);
   }

   void spacePteAccessCtx::obsoletePid(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      _obsoletePids.push(pid);
   }

   void spacePteAccessCtx::resetObsoleteResources()
   {
      _obsoleteLpids.clear();
      _obsoletePids.clear();
   }
} // namespace vessel

} // namespace engine
