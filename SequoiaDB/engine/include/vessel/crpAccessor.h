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

   Source File Name = crpAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CRP_ACCESSOR_H_
#define VESSEL_CRP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/strSlice.h"
#include "vessel/slice.h"
#include "vessel/collectionOptions.h"

namespace engine
{
namespace vessel
{
   class logicalPageBuffer;

   class crpAccessor : public pageAccessor
   {
      public:
         crpAccessor();
         virtual ~crpAccessor();
      public:

         INT32 createCL(requestContext *context,
                        const collectionRecord &record,
                        const createCLOptions &options,
                        logicalPageBuffer *lpb);

         /// record on disk must be valid.
         INT32 updateRoutePages(requestContext *context,
                                const collectionRecord &record,
                                logicalPageBuffer *lpb);
/*
         INT32 updateIndexInfo(requestContext *context,
                               CL_MB_ID mbID,
                               UINT64 uniqueIndexes,
                               UINT64 nonuniqueIndexes,
                               logicalPageBuffer *lpb);
                               */

      private:
         const collectionRecordOnDisk *getReadableDiskRecordPtr(const runtimePageBuffer *rpb,
                                                                 UINT32 i);

      private:
         INT32 prepareCreateCLLog(requestContext *context,
                                  UINT32 adjunctSize,
                                  const runtimePageBuffer *rpb,
                                  logRecordContext *lrc);

         INT32 commitCreateCLLog(requestContext *context,
                                 const GLOBAL_PAGE_ID &gpid,
                                 const collectionRecord &record,
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
                               const collectionRecord &oldRecord,
                               const collectionRecord &newRecord);

   };//class crpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_CRP_ACCESSOR_H_