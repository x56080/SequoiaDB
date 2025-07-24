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

   Source File Name = lpsPteWriteBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPS_PTE_WRITE_BATCH_H_
#define VESSEL_LPS_PTE_WRITE_BATCH_H_

#include "vessel/lpsPteViewer.h"
#include "vessel/lpageMappingPteCtx.h"
#include "vessel/spacePteAccessCtx.h"
#include "ossMemPool.hpp"
#include "vessel/storageFileDef.h"
#include <mutex>

namespace engine
{
namespace vessel
{
   class lpsPteWriteBatch : public SDBObject
   {
      friend class logicalPageSpacePte;
      public:
         lpsPteWriteBatch() = default;
         ~lpsPteWriteBatch() = default;
         lpsPteWriteBatch(const lpsPteWriteBatch &) = delete;
         lpsPteWriteBatch &operator=(const lpsPteWriteBatch &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid() const {return _viewer.isWritable();}
         OSS_INLINE UINT32 getPSN()const {return _viewer.getPSN();}
         OSS_INLINE UINT32 getWritingPSN() const {return _viewer.getPSN() + 1;}
         OSS_INLINE BOOLEAN hasPteMapping()const {return !_mctx.isEmpty();}
         void reset();
         ossPoolSet<UINT32> exportDirtyFiles(const storageCoreArgs &args)const;

      private:
         lpsPteWriteBatch(lpsPteViewer &&viewer);
         void precommit(PTE_ACCESS_CTX_PTR &&ctx);
      
      private:
         lpsPteViewer _viewer;
         lpageMappingPteCtx _mctx;

         std::mutex _mutex;
         ossPoolMap<UINT32, PTE_ACCESS_CTX_PTR> _committing;

   };//class lpsPteWriteBatch

   using LPS_PTE_WRITE_BATCH = std::unique_ptr<lpsPteWriteBatch>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPS_PTE_WRITE_BATCH_H_