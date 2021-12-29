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

   Source File Name = indexEntryPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_ENTRY_PAGE_ACCESSOR_H_
#define VESSEL_INDEX_ENTRY_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/slice.h"
#include "vessel/indexDef.h"
#include "vessel/indexObject.h"
#include "vessel/indexEntryPage.h"

namespace engine
{
namespace vessel
{
   class indexEntryPageAccessor : public pageAccessor
   {
      public:
         indexEntryPageAccessor(){}
         ~indexEntryPageAccessor(){}

      public:
         INT32 createIndex(requestContext *context,
                           UINT32 indexId,
                           const slice &defObj,
                           logicalPageBuffer *lpb)const;

         INT32 updateIndexStatus(requestContext *context,
                                 UINT32 indexId,
                                 INDEX_STATUS status,
                                 logicalPageBuffer *lpb)const;

         INT32 updateBtreeRoot(requestContext *context,
                               UINT32 indexId,
                               PAGE_ID root,
                               logicalPageBuffer *lpb)const;

         /// WARNING: Do not accesses obj any more after fini lpb if
         /// not owned.
         INT32 getIndexObject(requestContext *context,
                              const logicalPageBuffer *lpb,
                              indexObject &obj,
                              BOOLEAN getOwned = TRUE,
                              indexEntryPageHead *out = NULL)const;

         INT32 dump(requestContext *context,
                    const logicalPageBuffer *lpb,
                    bson::BSONObjBuilder &builder)const;

         /// do not release buffer when accessing head.
         INT32 getIndexDefPageHead(requestContext *context,
                                   UINT32 indexId,
                                   const logicalPageBuffer *lpb,
                                   const indexEntryPageHead **out)const;

         INT32 removeBtreeRoot(requestContext *context,
                               logicalPageBuffer *lpb,
                               PAGE_ID &oldValue);

   };//class indexEntryPageAccessor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_ENTRY_PAGE_ACCESSOR_H_