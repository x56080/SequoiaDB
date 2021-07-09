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

#include "vessel/vesselIdDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/collectionAllocator.h"
#include "vessel/strSlice.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/slice.h"
#include "vessel/storageUnit.h"
#include "ossRWMutex.hpp"
#include "vessel/lazyArray.hpp"
#include "vessel/inMemBitmap.h"
#include "vessel/collectionObjHolder.h"

namespace engine
{
namespace vessel
{
   class collection;
   class requestContext;
   class listCLCursor;
   class fsmFile;

   class collectionSpace : public SDBObject
   {
      public:
         collectionSpace();
         ~collectionSpace();
         collectionSpace(const collectionSpace &o) = delete;
         collectionSpace &operator=(const collectionSpace &o) = delete;
   
      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }
         OSS_INLINE SPACE_ID getSpaceId()const
         {
            return NULL == _su ? INVALID_SPACE_ID : _su->getSpaceID();
         }
         OSS_INLINE const CHAR *getCSName()const
         {
            return _recordInMem.name;
         }
         OSS_INLINE UINT32 getUniqueID()const
         {
            return _recordInMem.uniqueID; 
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
         OSS_INLINE BOOLEAN isOnline()const
         {
            return _recordInMem.isOnline();
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _recordInMem.logicalID;
         }
      public:
         INT32 create(requestContext *context,
                      const strSlice &name,
                      UINT32 logicalID,
                      storageUnit *su,
                      const createCSOptions &options);

         INT32 open(requestContext *context,
                    storageUnit *su);

         void close(BOOLEAN closeSU=TRUE);

         INT32 createCL(requestContext *context,
                        const strSlice &clName, 
                        utilCLInnerID clInnerId,
                        const createCLOptions &options);
      public:

         UINT32 getCollectionCount();

         INT32 listCollections(requestContext *context,
                               listCLCursor *cursor);

         INT32 dump(requestContext *context,
                    listCollectionSpaceRecord &record);

         INT32 getCollectionByName(requestContext *context,
                                   const strSlice &clName, 
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 getCollectionByMBID(requestContext *context,
                                   CL_MB_ID mbID,
                                   UINT32 logicalID,
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 ensureFsmFile(requestContext *context,
                             fsmFile **file);

      private:
         INT32 _create(requestContext *context,
                       const csMetaRecord &record,
                       const createCSOptions &options);
      private:
         INT32 precreateCL(const strSlice &clName,
                           utilCLInnerID innerID,
                           UINT32 &logicalID);


         void rollbackPrecreating(const strSlice &clName,
                                  utilCLInnerID innerID,
                                  UINT32 logicalID);

         void moveToFormalIndex(collectionAllocator::collectionHolder *holder);

         BOOLEAN insertIntoFormalIndex(collectionAllocator::collectionHolder *holder);

         void eraseFromIndex(const strSlice &clName,
                             utilCLInnerID innerID);

         BOOLEAN existsInFormalIndex(const strSlice &clName,
                                     utilCLInnerID innerID);

         /// Should always use name in nameBuffer instead of
         /// name in cl obj to do next upper bound.
         BOOLEAN upperBoundCLName(const strSlice &clName,
                                  UINT32 bufferSize,
                                  CHAR *nameBuffer,
                                  UINT32 &logicalID,
                                  collectionAllocator::collectionHolder **holder);

         BOOLEAN findCollection(const strSlice &clName,
                                UINT32 &logicalID,
                                collectionAllocator::collectionHolder **holder);
         BOOLEAN findCollection(utilCLInnerID innerID,
                                UINT32 &logicalID,
                                collectionAllocator::collectionHolder **holder);

      private:
         void fini();

         INT32 initCollectionsFromDisk(requestContext *context);

         INT32 initCollectionsFromOneDiskPage(requestContext *context,
                                              UINT32 capacity,
                                              PAGE_ID pid);

      private:
         struct _NAME_LESS
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrcmp(l, r) < 0;
            }
         };//struct _CS_NAME_LESS

         typedef ossPoolMap<const CHAR *, CL_MB_ID, _NAME_LESS> NAME_INDEX;
         typedef ossPoolMap<utilCLInnerID, CL_MB_ID> ID_INDEX;
         typedef ossPoolSet<const CHAR *, _NAME_LESS> _NAME_SET;
         typedef ossPoolSet<utilCLInnerID> _INNER_ID_SET;         
      private:
         BOOLEAN _isOpen = FALSE;
         storageUnit *_su = NULL;
         ossRWMutex _latch;
         csMetaRecord _recordInMem;

         inMemBitmap _allocator;
         lazyArray<collectionObjHolderGroup> _collections;
         UINT32 _maxCLLogicalID = DMS_INVALID_LOGICCLID;

         ///formal indexes
         NAME_INDEX _clNameIndex;
         ID_INDEX _clIdIndex;

         ///unformal indexes
         _NAME_SET _unformalNameIndex;
         _INNER_ID_SET _unformalInnerIdIndex;    
   };//class collectionSpace
}
}

#endif//VESSEL_COLLECTION_SPACE_H_