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

   Source File Name = lpsPteWriteBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpsPteWriteBatch.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lpsPteWriteBatch::lpsPteWriteBatch(lpsPteViewer &&viewer):
   _viewer(std::move(viewer))
   {
      SDB_ASSERT(_viewer.isWritable(), "can not be invalid");
   }

   void lpsPteWriteBatch::reset()
   {
      _committing.clear();
      _mctx.reset();
      _viewer.reset();
   }

   ossPoolSet<UINT32> lpsPteWriteBatch::exportDirtyFiles(const storageCoreArgs &args)const
   {
      ossPoolSet<UINT32> s;

      for (auto itr = _committing.cbegin(); itr != _committing.cend(); ++itr)
      {
         itr->second->exportDirtyFiles(args, s);
      }

      return std::move(s);
   }

   void lpsPteWriteBatch::precommit(PTE_ACCESS_CTX_PTR &&ctx)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      spacePteAccessCtx *obj = ctx.get();
      SDB_ASSERT(nullptr != obj && ! obj->isEmpty(), "can not be invalid");
      std::unique_lock<std::mutex> guard(_mutex);
      SDB_ASSERT(0 == _committing.count(obj->getId()), "duplidated id");
      _committing[obj->getId()] = std::move(ctx);
   }
} // namespace vessel

} // namespace engine
