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

   Source File Name = collectionSpace.h

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

#ifndef VESSEL_COLLECTION_SPACE_H_
#define VESSEL_COLLECTION_SPACE_H_

#include "vessel/vesselDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/collectionMap.h"
#include "vessel/stoargeUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/inMemBitMap.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"

namespace engine
{
namespace vessel
{
   class collection;
   class requestContext;
   class extentStorageUnit;
   class listCLCursor;

   class collectionSpace : public SDBObject
   {
      public:
         collectionSpace();
         ~collectionSpace();
         
      public:
         INT32 setup(requestContext *context,
                     extentStorageUnit *su);

         INT32 teardown();
      public:
         OSS_INLINE const CHAR *getCSName()const
         {
         return _recordInMem.name;
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _recordInMem.logicalID;
         }

         OSS_INLINE UINT32 getStatus()const
         {
            return _recordInMem.status;
         }
         OSS_INLINE UINT32 getFlags()const
         {
            return _recordInMem.flags;
         }
         OSS_INLINE UINT32 getVersion()const
         {
            return _recordInMem.version;
         }
         OSS_INLINE extentStorageUnit *getSU()
         {
            return _su;
         }

         OSS_INLINE BOOLEAN isOnline()const
         {
            return _recordInMem.isOnline();
         }

      public:
         SPACE_ID getSpaceID()const;
         INT32 createCL(requestContext *context,
                        const strSlice &clName, 
                        UINT32 clLogicalID,
                        const createCLOptions &options);

         INT32 listCollections(requestContext *context,
                               listCLCursor *cursor);

         INT32 dump(requestContext *context,
                    listCollectionSpaceRecord &record);
      public:
         INT32 getCollectionByLogicalID(requestContext *context,
                                        UINT32 logicalID,
                                        OSS_LATCH_MODE mode,
                                        collection **obj);

         INT32 getCollectionByMBID(requestContext *context,
                                   CL_MB_ID mbID,
                                   UINT32 logicalID,
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

      private:///init functions when start up
         INT32 initMetaRecordFromDisk(requestContext *context,
                                      extentStorageUnit *su);

         INT32 initCollectionsFromDisk(requestContext *context,
                                       extentStorageUnit *su);

         INT32 initCollectionsFromOneDiskPage(requestContext *context,
                                              PAGE_ID pid,
                                              extentStorageUnit *su);

      private:
         csMetaRecord _recordInMem;
         collectionMap _collectionMap;
         extentStorageUnit *_su;
   };//class collectionSpace
}
}

#endif//VESSEL_COLLECTION_SPACE_H_