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

   Source File Name = clIndexMbpAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_CL_INDEX_META_BLOCK_PAGE_ACCESSOR_H_
#define VESSEL_CL_INDEX_META_BLOCK_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/clIndexMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;
   class requestContext;

   class clIndexMbpAccessor : public pageAccessor
   {
      public:
         clIndexMbpAccessor(){}
         ~clIndexMbpAccessor(){}
         clIndexMbpAccessor(const clIndexMbpAccessor &) = delete;
         clIndexMbpAccessor &operator=(const clIndexMbpAccessor &) = delete;

      public:
         INT32 init(logicalPageBuffer *lpb);
         
         void fini(BOOLEAN lpbNeedClose = TRUE);
      
      public:
         INT32 initIndexMetaBlock(requestContext *context,
                                  INT32 blockPos);

         INT32 resetIndexMetaBlock(requestContext *context,
                                   INT32 blockPos);

         INT32 getIndexMetaBlock(requestContext *context,
                                 INT32 blockPos,
                                 clIndexMetaBlock &block);

      public:
         INT32 setIndexEntryPageLpid(requestContext *context,
                                     INT32 blockPos,
                                     INT32 slot,
                                     PAGE_ID lpid,
                                     UINT32 indexLid);

         INT32 resetIndexEntryPageLpid(requestContext *context,
                                       INT32 blockPos,
                                       INT32 slot);

      private:
         logicalPageBuffer *_lpb = nullptr;

   }; // class clIndexMbpAccessor

} // namespace vessel
} // namespace engine

#endif // VESSEL_CL_INDEX_META_BLOCK_PAGE_ACCESSOR_H_