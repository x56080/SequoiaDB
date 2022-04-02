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

   Source File Name = clMetaBlockPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
                                const clMetaBlock &block,
                                logicalPageBuffer *lpb);

         INT32 truncateRouteMap(requestContext *context,
                                logicalPageBuffer *lpb);

      private:
         const clMetaBlockOnDisk *getReadableDiskBlockPtr(const runtimePageBuffer *rpb,
                                                          UINT32 i);

      private:
         INT32 prepareCreateCLLog(requestContext *context,
                                  UINT32 adjunctSize,
                                  const runtimePageBuffer *rpb,
                                  logRecordContext *lrc);

         INT32 commitCreateCLLog(requestContext *context,
                                 const GLOBAL_PAGE_ID &gpid,
                                 const clMetaBlock &block,
                                 const slice &adjunct,
                                 logRecordContext *lrc);
                                  
         INT32 prepareUpdateLog(requestContext *context,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

         INT32 commitUpdateLog(requestContext *context,
                               logRecordContext *lrc,
                               const GLOBAL_PAGE_ID &gpid,
                               PAGE_ID lpid,
                               UINT64 mask,
                               const clMetaBlock &oldBlock,
                               const clMetaBlock &newBlock);

         INT32 commitRemoveLog(requestContext *context,
                               const runtimePageBuffer *rpb,
                               DPS_LSN_OFFSET &lsn);

   };//class clMetaBlockPageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_CL_META_BLOCK_PAGE_ACCESSOR_H_