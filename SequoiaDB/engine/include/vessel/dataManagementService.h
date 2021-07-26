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

   Source File Name = dataManagementService.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class requestContext;
   class listCSCursor;
   class storageUnit;
   class IQueryFilter;

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
                        const createCSOptions &options,
                        SPACE_ID *sid = NULL,
                        UINT32 *logicalID = NULL);

         INT32 listCollectionSpaces(requestContext *context,
                                    listCSCursor *cursor);

         UINT32 getCSCount();

      public:
         INT32 testCS(requestContext *context,
                      const strSlice &nameSlice,
                      UINT32 &logicalID,
                      SPACE_ID &sid);

         INT32 testCS(requestContext *context,
                      utilCSUniqueID uniqueID,
                      UINT32 &logicalID,
                      SPACE_ID &sid);

         INT32 getCSByName(requestContext *context,
                           const strSlice &nameSlice,
                           OSS_LATCH_MODE mode,
                           collectionSpace **out);
         INT32 getCSByUniqueID(requestContext *context,
                               utilCSUniqueID uniqueID,
                               OSS_LATCH_MODE mode,
                               collectionSpace **out);

         /// if logicalID set as valid value,
         /// will return error when the actual id of the object
         /// does not match the parameter
         INT32 getCSBySpaceID(requestContext *context,
                              SPACE_ID sid,
                              UINT32 logicalID,
                              OSS_LATCH_MODE mode,
                              collectionSpace **out);

         /// if logicalID set as valid value,
         /// will return error when the actual id of the object
         /// does not match the parameter
         INT32 getCSByLockedSpaceID(requestContext *context,
                                    UINT32 logicalID,
                                    collectionSpace **out);

      public:/// snapshot
         PAGE_SNAPSHOT_VERION getOnlinePageSnapshotVersion();
         INT32 isSnapshotEffective(SPACE_ID sid,
                                   PAGE_SNAPSHOT_VERION psv,
                                   BOOLEAN &effective);

      public:/// storage
         ///WARNING: User must be sure that space exists and will
         /// not be released during accessing.
         INT32 getMmapPagePtr(const GLOBAL_PAGE_ID &gpid,
                              mmapPagePointer &ptr)const;

         INT32 getPageSize(SPACE_ID sid,
                           SPACE_TYPE spaceType,
                           FILE_TYPE fileType,
                           UINT32 &pageSize)const;

         INT32 getLogicalPageSpace(SPACE_ID sid,
                                   SPACE_TYPE type,
                                   logicalPageSpace **lps);

      private:
         INT32 loadStorageUnits(requestContext *context,
                                ossPoolList<SPACE_ID> &sidList);

         INT32 loadCollectionSpaces(requestContext *context,
                                    const ossPoolList<SPACE_ID> &sidList);

      private:
         INT32 occupySpaceId(SPACE_ID sid);
         
         INT32 precreateCS(const strSlice &csName,
                           utilCSUniqueID uniqueID,
                           UINT32 &logicalID,
                           SPACE_ID &sid);

         void rollbackPrecreating(const strSlice &csName,
                                  utilCSUniqueID uniqueID,
                                  UINT32 logicalID,
                                  SPACE_ID sid); 

         void endToCreateCS(collectionSpace *obj);

         INT32 createSU(requestContext *context,
                        const createCSOptions &options,
                        storageUnit **out);

         INT32 createCS(requestContext *context,
                        const strSlice &csName,
                        utilCSUniqueID uniqueId,
                        UINT32 logicalID,
                        const createCSOptions &options,
                        collectionSpace **out);

         void rollbackPrecreating(requestContext *context,
                                  const strSlice &csName,
                                  utilCSUniqueID uniqueID,
                                  UINT32 logicalID,
                                  SPACE_ID sid);

         INT32 _getCSByName(requestContext *context,
                            const strSlice &nameSlice,
                            OSS_LATCH_MODE mode,
                            collectionSpace **out);

         INT32 _getCSByUniqueId(requestContext *context,
                                utilCSUniqueID uniqueId,
                                OSS_LATCH_MODE mode,
                                collectionSpace **out);

         BOOLEAN testCS(const strSlice &csName,
                        UINT32 &logicalID,
                        SPACE_ID &sid,
                        collectionSpace **obj)const;

         BOOLEAN testCS(utilCSUniqueID uniqueID,
                        UINT32 &logicalID,
                        SPACE_ID &sid,
                        collectionSpace **obj)const;

         BOOLEAN upperBoundCS(const strSlice &name,
                              UINT32 &logicalID,
                              SPACE_ID &sid,
                              collectionSpace **obj)const;

         collectionSpace *getCS(SPACE_ID sid)const;

         INT32 insertIntoFormalIndex(collectionSpace *obj);
         void removeFromFormalIndex(collectionSpace *obj);


      private:

         struct _NAME_LESS
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrcmp(l, r) < 0;
            }
         };//struct _CS_NAME_LESS
         typedef ossPoolMap<const CHAR *, collectionSpace *, _NAME_LESS> _NAME_INDEX;
         typedef ossPoolMap<utilCSUniqueID, collectionSpace *> _UID_INDEX;
         typedef ossPoolMap<SPACE_ID, collectionSpace *> _SPACE_ID_INDEX;

         typedef ossPoolSet<utilCSUniqueID> _UID_SET;
         typedef ossPoolSet<const CHAR *, _NAME_LESS> _NAME_SET;


      private:
         BOOLEAN _isOpen = FALSE;
         ossRWMutex _latch;
         UINT32 _nextLogicalID = VESSEL_MIN_CS_LID;

         /// formal indexes
         _SPACE_ID_INDEX _mainIndex;
         /// Name should be readonly when obj exists in name index.
         _NAME_INDEX _nameIndex;  
         _UID_INDEX _uidIndex;
         
         ///unformal indexes, used when creating/removing.
         _NAME_SET _unformalNameIndex;
         _UID_SET _unformalUidIndex;

         inMemBitmap _suAllocator;
         lazyArray<storageUnit> _sus;
   };//class dataManagementService

   typedef class dataManagementService CS_CONTAINER;
}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_MANAGEMENT_SERVICE_H_