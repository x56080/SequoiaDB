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

   Source File Name = csMetaBlockPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         csMetaBlockPageAccessor();
         virtual ~csMetaBlockPageAccessor();

      public:
         INT32 read(requestContext *context,
                    const logicalPageBuffer *lpb,
                    csMetaBlock &cmb);

         INT32 update(requestContext *context,
                      logicalPageBuffer *lpb,
                      const csMetaBlock &block,
                      UINT64 mask);

   private:
         INT32 prepareUpdateLog(requestContext *context,
                                const runtimePageBuffer *rpb,
                                logRecordContext *lrc);

         INT32 commitUpdateLog(requestContext *context,
                               const GLOBAL_PAGE_ID &gpid,
                               const csMetaBlock &oldBlock,
                               const csMetaBlock &block,
                               UINT64 mask,
                               logRecordContext *lrc);

   };//class csMetaBlockPageAccessor
}//class vessel
}//class engine

#endif//VESSEL_CS_META_BLOCK_PAGE_ACCESSOR_H_