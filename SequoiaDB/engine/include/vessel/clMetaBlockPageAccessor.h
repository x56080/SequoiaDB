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

   Source File Name = clMetaBlockPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CL_META_BLOCK_PAGE_ACCESSOR_H_
#define VESSEL_CL_META_BLOCK_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/clMetaBlockPage.h"
#include "vessel/strSlice.h"
#include "vessel/slice.h"
#include "vessel/collectionOptions.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;

   class clMetaBlockPageAccessor : public pageAccessor
   {
      public:
         clMetaBlockPageAccessor();
         virtual ~clMetaBlockPageAccessor();
      public:

         INT32 createCL(requestContext *context,
                        const clMetaBlock &block,
                        const createCLOptions &options,
                        logicalPageBuffer *lpb);

         INT32 removeCL(requestContext *context,
                        logicalPageBuffer *lpb);

         /// record on disk must be valid.
         INT32 updateRoutePages(requestContext *context,
                                const PAGE_ID *pages,
                                UINT32 count,
                                logicalPageBuffer *lpb);

         INT32 truncateRouteMap(requestContext *context,
                                logicalPageBuffer *lpb);

      private:
         const clMetaBlockOnDisk *getReadableDiskBlockPtr(const runtimePageBuffer *rpb,
                                                          UINT32 i);

      private:
         INT32 writeCreateCLJournal(requestContext *context,
                                    const GLOBAL_PAGE_ID &gpid,
                                    const clMetaBlock &block,
                                    const slice &adjunct,
                                    DPS_LSN_OFFSET &lsn);

         INT32 writeUpdateJournal(requestContext *context,
                                  const GLOBAL_PAGE_ID &gpid,
                                  UINT64 mask,
                                  const clMetaBlock &oldBlock,
                                  const clMetaBlock &newBlock,
                                  DPS_LSN_OFFSET &lsn);

         INT32 writeRemoveJournal(requestContext *context,
                                  const GLOBAL_PAGE_ID &gpid,
                                  DPS_LSN_OFFSET &lsn);

   };//class clMetaBlockPageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_CL_META_BLOCK_PAGE_ACCESSOR_H_