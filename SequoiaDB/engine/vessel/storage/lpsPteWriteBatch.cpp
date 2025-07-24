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

   Source File Name = lpsPteWriteBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
