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

   Source File Name = indexSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SPACE_H_
#define VESSEL_INDEX_SPACE_H_

#include "vessel/logicalPageSpace.h"
#include "ossRWMutex.hpp"
#include "vessel/lpageMappingPteCtx.h"
#include "vessel/sparsePidBitmap.h"
#include "vessel/indexSpaceAccessCtx.h"

#include <chrono>
#include <atomic>

namespace engine
{
namespace vessel
{  
   class indexSpace : public logicalPageSpace
   {
      public:
         indexSpace(const storageUnitManifest *manifest):
         logicalPageSpace(manifest){}
         virtual ~indexSpace(){}

      public:
         virtual SPACE_TYPE getSpaceType()const override
         {
            return SPACE_TYPE_IDX;
         }

      protected:
         virtual INT32 _getRuntimePageBuffer(requestContext *context,
                                             PAGE_ID pid,
                                             const ossSharedLatchMode &mode,
                                             runtimePageBuffer &rpb);

         virtual INT32 _getRuntimePageBufferToReset(requestContext *context,
                                                    PAGE_ID pid,
                                                    runtimePageBuffer &rpb);

         virtual INT32 _copyPageAndReinitBuffer(requestContext *context,
                                                PAGE_SNAPSHOT_VERION psv,
                                                PAGE_ID newPid,
                                                runtimePageBuffer &rpb);
                                                
      public:
         INT32 openAccessCtx(requestContext *context,
                             indexSpaceAccessCtx &ac);

         void abort(indexSpaceAccessCtx &ctx);

         INT32 getLogicalPageBuffer(indexSpaceAccessCtx &ctx,
                                    PAGE_ID lpid,
                                    BOOLEAN pteEnabled,
                                    logicalPageBuffer &buffer);

         INT32 allocate(indexSpaceAccessCtx &ctx,
                        pageInitializer *initer,
                        PAGE_ID &lpid);

         INT32 makePrivateBuffer(indexSpaceAccessCtx &ctx,
                                 logicalPageBuffer &lpb);

         INT32 removePage(indexSpaceAccessCtx &ctx,
                          PAGE_ID lpid);

         INT32 removePages(indexSpaceAccessCtx &ctx,
                           UINT32 size,
                           const PAGE_ID *lpids);

      public:
         UINT64 getPSN()const
         {
            return _psn.load(std::memory_order_relaxed);
         }

         void incPtePageNum()
         {
            _totalPtePageNum.fetch_add(1, std::memory_order_relaxed);
         }
         void resetPtePageNum()
         {
            _totalPtePageNum.store(0, std::memory_order_relaxed);
         }
         UINT32 getPtePageNum()const
         {
            return _totalPtePageNum.load(std::memory_order_relaxed);
         }

      private:
         void _rollbackPagesReserved(indexSpaceAccessCtx &ctx);
         void _rollbackPageRemapped(indexSpaceAccessCtx &ctx);

      private:
         ossRWMutex _mutex;
         std::atomic<UINT64> _psn{0};
         std::atomic_uint _totalPtePageNum{0};
         std::chrono::steady_clock _lastPublishTime;
         lpageMappingPteCtx _mappingCtx;

         ossSpinSLatchPOSIX _lpidsToFreeLatch;
         sparsePidBitmap _lpidsToFree{&_lpidsToFreeLatch};
         ossSpinSLatchPOSIX _ppidsToFreeLatch;
         sparsePidBitmap _ppidsToFree{&_ppidsToFreeLatch};
   };//class indexSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_SPACE_H_