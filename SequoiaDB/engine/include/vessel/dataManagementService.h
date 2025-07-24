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

   Source File Name = dataManagementService.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_DATA_MANAGEMENT_SERVICE_H_
#define VESSEL_DATA_MANAGEMENT_SERVICE_H_

#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/vesselOptions.h"
#include "ossMemPool.hpp"
#include "vessel/storageUnit.h"
#include "vessel/lazyArray.hpp"
#include "ossRWMutex.hpp"
#include "vessel/objectIdentifier.h"
#include "dmsEngineOptions.hpp"

#include <boost/dynamic_bitset.hpp>

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class requestContext;
   class listCSCursor;
   class storageUnit;
   class storageFileLoader;
   class storageFileCluster;

   class dataManagementService : public SDBObject
   {
      public:
         dataManagementService();
         ~dataManagementService();
         dataManagementService(const dataManagementService &) = delete;
         dataManagementService &operator=(const dataManagementService &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }

      public:
         INT32 open(requestContext *context);
         void close();

         INT32 createCS(requestContext *context,
                        const strSlice &csName,
                        utilCSUniqueID uniqueId,
                        const dmsCreateCSOptions &options,
                        collectionSpaceId &identifier);

         INT32 removeCS(requestContext *context,
                        const collectionSpaceId &identifier);

         INT32 listCollectionSpaces(requestContext *context,
                                    listCSCursor *cursor);

         UINT32 getCSCount();

      public:
         INT32 testCS(const strSlice &nameSlice,
                      collectionSpaceId &identifier);

         INT32 testCS(utilCSUniqueID uniqueID,
                      collectionSpaceId &identifier);

         INT32 testCSByLid(UINT32 lid,
                           collectionSpaceId &identifier);

         INT32 getCSByName(requestContext *context,
                           const strSlice &nameSlice,
                           OSS_LATCH_MODE mode,
                           collectionSpace **out);

         INT32 getCSByUniqueID(requestContext *context,
                               utilCSUniqueID uniqueID,
                               OSS_LATCH_MODE mode,
                               collectionSpace **out);

         INT32 getCSByLogicalID(requestContext *context,
                                UINT32 logicalID,
                                OSS_LATCH_MODE mode,
                                collectionSpace **out);

         INT32 getCSByCollectionSpaceId(requestContext *context,
                                        const collectionSpaceId &id,
                                        OSS_LATCH_MODE mode,
                                        collectionSpace **out);

      public:/// snapshot
         PAGE_SNAPSHOT_VERION getOnlinePageSnapshotVersion();
         INT32 isSnapshotEffective(SPACE_ID sid,
                                   PAGE_SNAPSHOT_VERION psv,
                                   BOOLEAN &effective);

      public:/// storage
         ///WARNING: User must be sure that space exists and will
         /// not be released during accessing.
         /// validate page size if pageSize is not null
         INT32 getMmapPagePtr(const GLOBAL_PAGE_ID &gpid,
                              mmapPagePointer &ptr,
                              const UINT32 *pageSize=nullptr)const;

         INT32 getLogicalPageSpace(SPACE_ID sid,
                                   SPACE_TYPE type,
                                   logicalPageSpace **lps)const;

         INT32 getLogicalPageSpace(SPACE_ID sid,
                                   SPACE_TYPE type,
                                   LPS_OBJ_PTR &out)const;

         storageUnit *getStorageUnit(SPACE_ID sid);

         storageFileCluster *getLobdFileCluster(SPACE_ID sid);

         storageFileCluster *getStorageFileClsuter(SPACE_ID sid, SPACE_TYPE type);

      private:
         INT32 loadStorageUnits(requestContext *context,
                                ossPoolList<SPACE_ID> &sidList);

         INT32 loadCollectionSpaces(requestContext *context,
                                    const ossPoolList<SPACE_ID> &sidList);

      private:
         INT32 occupySpaceId(SPACE_ID sid);
         
         INT32 reserveCSForCreating(const strSlice &csName,
                                    utilCSUniqueID uniqueID,
                                    UINT32 &logicalID,
                                    SPACE_ID &sid);

         void clearReservedCSInfo(const strSlice &csName,
                                  utilCSUniqueID uniqueID,
                                  UINT32 logicalID,
                                  SPACE_ID sid); 

         void _endToCreateCS(std::unique_ptr<collectionSpace> &&obj);

         /// obj will not managed by obj map!
         /// user must release it outside.
         void _prepareToDropCS(collectionSpace *obj);

         INT32 createSU(requestContext *context,
                        const collectionSpaceId &id,
                        const dmsCreateCSOptions &options,
                        storageUnit *su);

         INT32 _createCS(requestContext *context,
                         const strSlice &csName,
                         const collectionSpaceId &id,
                         const dmsCreateCSOptions &options,
                         std::unique_ptr<collectionSpace> &out);

         INT32 _lockAndGetCSByName(requestContext *context,
                                   const strSlice &nameSlice,
                                   OSS_LATCH_MODE mode,
                                   collectionSpace **out);

         INT32 _lockAndGetCSByUid(requestContext *context,
                                  utilCSUniqueID uniqueId,
                                  OSS_LATCH_MODE mode,
                                  collectionSpace **out);

         INT32 _lockAndGetCSByLid(requestContext *context,
                                  UINT32 logicalId,
                                  OSS_LATCH_MODE mode,
                                  collectionSpace **out);

         collectionSpace *_getCSByName(const strSlice &csName);
         collectionSpace *_getCSByUid(utilCSUniqueID uniqueID);
         collectionSpace *_getCSByLid(UINT32 lid);

         collectionSpace *_upperBound(UINT32 logicalId); 

         INT32 _insertIntoFormalIndex(std::unique_ptr<collectionSpace> &&obj);


      private:

         struct _NAME_LESS
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrncmp(l, r, DMS_COLLECTION_SPACE_NAME_SZ) < 0;
            }
         };//struct _CS_NAME_LESS
         
         using _CS_UNIQUE_PTR = std::unique_ptr<collectionSpace>;
         using _CS_UPTR_INDEX = std::map<UINT32, _CS_UNIQUE_PTR>;
         using _CS_UID_INDEX = std::map<utilCSUniqueID, collectionSpace*>;
         using _CS_NAME_INDEX = std::map<const CHAR *, collectionSpace *, _NAME_LESS>;

         using _TMP_UID_SET = ossPoolSet<utilCSUniqueID>;
         using _TMP_NAME_SET = ossPoolSet<ossPoolString>;

      private:
         BOOLEAN _isOpen = FALSE;
         ossRWMutex _latch;
         UINT32 _nextLogicalID = 0;

         /// formal indexes
         _CS_UPTR_INDEX _mainIndex;
         /// Name should be readonly when obj exists in name index.
         _CS_NAME_INDEX _nameIndex;  
         _CS_UID_INDEX _uidIndex;
         
         ///unformal indexes, used when creating/removing.
         _TMP_NAME_SET _unformalNameIndex;
         _TMP_UID_SET _unformalUidIndex;

         boost::dynamic_bitset<> _suAllocator;
         lazyArray<storageUnit> _sus;
   };//class dataManagementService
}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_MANAGEMENT_SERVICE_H_