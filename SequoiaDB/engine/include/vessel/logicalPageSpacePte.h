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

   Source File Name = logicalPageSpacePte.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_SPACE_PTE_H_
#define VESSEL_LOGICAL_PAGE_SPACE_PTE_H_

#include "vessel/logicalPageSpace.h"
#include "ossSharedLatch.hpp"
#include "vessel/lpsPteViewer.h"
#include "vessel/spacePteAccessCtx.h"
#include "vessel/lpsPteWriteBatch.h"
#include "vessel/logicalPageBufferPte.h"

#include <atomic>

namespace engine
{
namespace vessel
{
   class logicalPageSpacePte : public logicalPageSpace
   {
      public:
         logicalPageSpacePte(const storageUnitManifest *manifest);
         virtual ~logicalPageSpacePte() = default;

      public:
         INT32 getPublicPageBuffer(requestContext *context,
                                   PAGE_ID lpid,
                                   logicalPageBufferPte &buffer);

         INT32 getPageBuffer(requestContext *context,
                             spacePteAccessCtx *actx,
                             PAGE_ID lpid,
                             logicalPageBufferPte &buffer);

         INT32 allocatePtePage(requestContext *context,
                               spacePteAccessCtx *actx,
                               pageInitializer *initer,
                               PAGE_ID &lpid);

         INT32 makePrivateBuffer(requestContext *context,
                                 spacePteAccessCtx *actx,
                                 logicalPageBufferPte &buffer);

         INT32 removePage(requestContext *context,
                          spacePteAccessCtx *actx,
                          PAGE_ID lpid);

         INT32 removePages(requestContext *context,
                           spacePteAccessCtx *actx,
                           UINT32 size,
                           const PAGE_ID *lpids);

      public:
         OSS_INLINE UINT32 getPSN()const {return _psn.load(std::memory_order_relaxed);}
         INT32 initViewer(BOOLEAN readonly, lpsPteViewer &v);
         INT32 initWriteBatch(LPS_PTE_WRITE_BATCH &batch);
         INT32 precommit(requestContext *context,
                         lpsPteWriteBatch &batch,
                         PTE_ACCESS_CTX_PTR &&ac);
         INT32 commit(LPS_PTE_WRITE_BATCH &batch);
         void abort(LPS_PTE_WRITE_BATCH &batch);
         void abort(PTE_ACCESS_CTX_PTR &ac);

      protected:
         virtual INT32 _getRuntimePageBuffer(requestContext *context,
                                             PAGE_ID pid,
                                             const ossSharedLatchMode &mode,
                                             runtimePageBuffer &rpb) override;

         virtual INT32 _getRuntimePageBufferToReset(requestContext *context,
                                                    PAGE_ID pid,
                                                    runtimePageBuffer &rpb) override;

         virtual INT32 _copyPageAndReinitBuffer(requestContext *context,
                                                PAGE_SNAPSHOT_VERION psv,
                                                PAGE_ID newPid,
                                                runtimePageBuffer &rpb) override;

      private:
         INT32 _fsyncPrivatePages(spacePteAccessCtx *ctx);
         INT32 _fsyncDirtyClusterFiles(const lpsPteWriteBatch &batch);
         INT32 _commit(lpsPteWriteBatch &batch);
         void _freeObsoleteResources(lpsPteWriteBatch &batch);
         INT32 _getPtePrior(requestContext *context,
                            spacePteAccessCtx *actx,
                            PAGE_ID lpid,
                            lpageDescriptor &desc,
                            BOOLEAN &isPrivate);

      private:
         ossSharedLatch _publishingLocker;
         std::atomic<UINT32> _psn{0};
   };//class logicalPageSpacePte

   using PTE_LPS = logicalPageSpacePte;

} // namespace vessel

} // namespace engine


#endif//VESSEL_LOGICAL_PAGE_SPACE_PTE_H_