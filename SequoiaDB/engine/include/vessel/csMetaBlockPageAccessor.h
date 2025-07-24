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

   Source File Name = csMetaBlockPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CS_META_BLOCK_PAGE_ACCESSOR_H_
#define VESSEL_CS_META_BLOCK_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/csMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   ///collection space global page
   class csMetaBlockPageAccessor : public pageAccessor
   {
      public:
         csMetaBlockPageAccessor() = default;
         virtual ~csMetaBlockPageAccessor() = default;

      public:
         INT32 read(requestContext *context,
                    const logicalPageBuffer *lpb,
                    csMetaBlock &cmb);

         INT32 update(requestContext *context,
                      logicalPageBuffer *lpb,
                      const csMetaBlock &block,
                      UINT64 mask);

   private:
         INT32 writeJournal(requestContext *context,
                            const GLOBAL_PAGE_ID &gpid,
                            const csMetaBlock &oldBlock,
                            const csMetaBlock &block,
                            UINT64 mask,
                            DPS_LSN_OFFSET &lsn);

   };//class csMetaBlockPageAccessor
}//class vessel
}//class engine

#endif//VESSEL_CS_META_BLOCK_PAGE_ACCESSOR_H_