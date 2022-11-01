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

   Source File Name = lpsPteWriteBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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