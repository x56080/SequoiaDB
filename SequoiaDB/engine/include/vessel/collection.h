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

   Source File Name = collection.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_H_
#define VESSEL_COLLECTION_H_

#include "vessel/collectionRecordPage.h"
#include "vessel/recordID.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/recordID.h"
#include "vessel/listCollectionsDef.h"

namespace engine
{
   class _dpsLogRecord;
namespace vessel
{
   class collectionSpace;
   class requestContext;
   
   class collection: public SDBObject
   {
      public:
         collection();
         ~collection();

      public:
         OSS_INLINE const CHAR *getName()const
         {
            return _record.name;
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _record.logicalCLID;
         }
         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _record.mbID;
         }
         OSS_INLINE utilCLInnerID getInnerID()const
         {
            return _record.innerID;
         }
         OSS_INLINE const collectionRecord &getRecord()const
         {
            return _record;
         }

         INT32 create(requestContext *context,
                      const strSlice &clName,
                      utilCLInnerID innerID,
                      UINT32 logicalID,
                      collectionSpace *cs,
                      const createCLOptions &options);

         /// init when startup
         INT32 initWhenOpen(const collectionRecord &record,
                            collectionSpace *cs);

         void fini();

      public:
         INT32 dump(requestContext *context,
                    listCollectionsRecord &record);

         INT32 insert(requestContext *context,
                      const slice &record,
                      const insertOptions &options);

      private:
         INT32 saveOnDiskWhenCreating(requestContext *context);

         INT32 ensureCLRecordPageAllocated(requestContext *context,
                                           PAGE_ID lpid,
                                           PAGE_ID &pid);

         INT32 allocatePageForCLRecord(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_ID &pid);

         INT32 preallocateCLRecordPage(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_ID &pid);

         INT32 saveCLRecordWhenCreating(requestContext *contex, PAGE_ID pid);
      private:
         ossSpinSLatch _recordLatch;
         collectionRecord _record;
         collectionSpace *_collectionSpace;
   };//class collection
}//namespace vessel
}//namespace engine

#endif // VESSEL_COLLECTION_H_