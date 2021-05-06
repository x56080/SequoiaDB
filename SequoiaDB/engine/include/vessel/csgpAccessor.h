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

   Source File Name = csgpAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CSGP_ACCESSOR_H_
#define VESSEL_CSGP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/storageFileDef.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   
   ///collection space global page
   class csgpAccessor : public pageAccessor
   {
      public:
         csgpAccessor();
         virtual ~csgpAccessor();
      
      public:
         INT32 initPage(requestContext *context, const csMetaRecord &record);

         INT32 setOnlineWhenCreating(requestContext *context,
                                     const dataIDMapFileHead &head);

         INT32 readMetaRecord(csMetaRecord &record);

      private:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_CS_META;
         }

      private:
         INT32 prepareUpdateLog(requestContext *context,
                                logRecordContext *lrc,
                                DPS_LOG_TYPE ddlType,
                                BOOLEAN hasOld,
                                const slice &adjuncts);

         INT32 commitUpdateLog(requestContext *context,
                               logRecordContext *lrc,
                               DPS_LOG_TYPE ddlType,
                               UINT64 mask,
                               const csMetaRecord *old,
                               const csMetaRecord &record,
                               const slice &adjuncts);
   };//class csgpAccessor
}//class vessel
}//class engine

#endif//VESSEL_CSGP_ACCESSOR_H_