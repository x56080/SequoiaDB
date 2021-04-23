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

   Source File Name = collectionSpaceContainer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_SPACE_CONTAINER_H_
#define VESSEL_COLLECTION_SPACE_CONTAINER_H_

#include "vessel/vesselIdDef.h"
#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/vesselOptions.h"
#include "ossMemPool.hpp"
#include "vessel/objectSlots.h"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class requestContext;
   class listCSCursor;
   class storageUnit;
   class IQueryFilter;

   class collectionSpaceContainer : public SDBObject
   {
      public:
         collectionSpaceContainer();
         ~collectionSpaceContainer();
         collectionSpaceContainer(const collectionSpaceContainer &) = delete;
         collectionSpaceContainer &operator=(const collectionSpaceContainer &) = delete;

      public:
         INT32 openStorageUnits(requestContext *context);
         INT32 openCollectionSpaces(requestContext *context);
         void close();

         INT32 createCS(requestContext *context,
                        const strSlice &csName,
                        utilCSUniqueID uniqueID,
                        const createCSOptions &options,
                        SPACE_ID *sid = NULL,
                        UINT32 *logicalID = NULL);

         INT32 listCollectionSpaces(requestContext *context,
                                    listCSCursor *cursor);

         UINT32 getCSCount();

         INT32 getCLCount(requestContext *context,
                          const CHAR *csName,
                          UINT32 &cnt);

      public:
         INT32 testCS(requestContext *context,
                      const strSlice &nameSlice,
                      utilCSUniqueID uniqueID,
                      UINT32 &logicalID,
                      SPACE_ID &sid);
         INT32 getCSByName(requestContext *context,
                           const strSlice &nameSlice,
                           OSS_LATCH_MODE mode,
                           collectionSpace **obj);
         INT32 getCSByUniqueID(requestContext *context,
                               utilCSUniqueID uniqueID,
                               OSS_LATCH_MODE mode,
                               collectionSpace **obj);
         /// if logicalID set as valid value,
         /// will return error when the actual id of the object does not match the parameter
         INT32 getCSByLockedSpaceID(requestContext *context,
                                    UINT32 logicalID,
                                    collectionSpace **obj);

         /// WRANING: you need to make sure that su has been created and will not be deleted.
         INT32 getStorageUnit(SPACE_ID sid,
                              storageUnit **su);

      private:
         void fini();
         void finiOpenCS();

      private:
         INT32 precreateCS(requestContext *context,
                           const strSlice &csName,
                           utilCSUniqueID uniqueID,
                           UINT32 &logicalID,
                           SPACE_ID &sid);

         void endToCreateCS(requestContext *context,
                            const strSlice &csName,
                            utilCSUniqueID uniqueID,
                            UINT32 logicalID,
                            SPACE_ID sid);

         INT32 createSU(requestContext *context,
                        UINT32 logicalID,
                        const createCSOptions &options,
                        storageUnit **su);

         INT32 createCS(requestContext *context,
                        storageUnit *su,
                        const strSlice &csName,
                        utilCSUniqueID uniqueID,
                        const createCSOptions &options);

         void rollbackPrecreating(requestContext *context,
                                  const strSlice &csName,
                                  utilCSUniqueID uniqueID,
                                  UINT32 logicalID,
                                  SPACE_ID sid);
         INT32 loadStorageUnitsOnDisk(requestContext *context);
         
         BOOLEAN allocateSpaceID(SPACE_ID &sid);
         void releaseSpaceID(SPACE_ID sid);

         INT32 addToIndex(const strSlice &csName,
                          utilCSUniqueID uniqueID,
                          SPACE_ID sid,
                          UINT32 logicalID);

         void upsertToIndex(const strSlice &csName,
                            utilCSUniqueID uniqueID,
                            SPACE_ID sid,
                            UINT32 logicalID);

         BOOLEAN exists(const strSlice &csName,
                        utilCSUniqueID uniqueID);

         void removeFromIndex(const strSlice &csName,
                              utilCSUniqueID uniqueID);

         BOOLEAN getLIdAndSid(const strSlice &csName,
                              BOOLEAN mustBeValid,
                              UINT32 &logicalID,
                              SPACE_ID &sid);
         BOOLEAN getLIdAndSid(utilCSUniqueID uniqueID,
                              BOOLEAN mustBeValid,
                              UINT32 &logicalID,
                              SPACE_ID &sid);

         INT32 upperBoundCSName(const strSlice &name,
                                UINT32 bufferSize,
                                CHAR *nextName,
                                UINT32 &nextLId,
                                SPACE_ID &nextSid);

      private:
         struct _LID_SID_PAIR
         {
            OSS_INLINE _LID_SID_PAIR():
            logicalID(DMS_INVALID_LOGICCSID),
            sid(INVALID_SPACE_ID)
            {}

            OSS_INLINE _LID_SID_PAIR(UINT32 l, SPACE_ID s):
            logicalID(l),
            sid(s)
            {}

            OSS_INLINE ~_LID_SID_PAIR()
            {}

            OSS_INLINE _LID_SID_PAIR &operator=(const _LID_SID_PAIR &o)
            {
               logicalID = o.logicalID;
               sid = o.sid;
               return *this;
            }

            UINT32 logicalID;
            SPACE_ID sid;
         };//struct _LID_SID_PAIR

         typedef ossPoolMap<ossPoolString, _LID_SID_PAIR> NAME_INDEX;
         typedef ossPoolMap<utilCSUniqueID, _LID_SID_PAIR> UID_INDEX;
         typedef ossPoolList<SPACE_ID> _SPACE_ID_POOL;

         enum _CONTAINER_STATUS
         {
            CLOSED = 0,
            SU_LOADED = 1,
            OPEN = 2,
         };

      private:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return OPEN == _status;
         }
         OSS_INLINE BOOLEAN isSULoaded()const
         {
            return OPEN == _status || SU_LOADED == _status;
         }
         OSS_INLINE BOOLEAN isClosed()const
         {
            return CLOSED == _status;
         }

      private:
         ossSpinSLatch _latch;
         _CONTAINER_STATUS _status = CLOSED;
         UINT32 _nextLogicalID = VESSEL_MIN_CS_LID;
         _SPACE_ID_POOL _freeStorageUnits;
         objectSlots<storageUnit> _storageUnits;
         objectSlots<collectionSpace> _collectionSpaces;
         NAME_INDEX _nameIndex;
         UID_INDEX _uidIndex;
         UINT32 _creatingCount = 0;
   };//class collectionSpaceContainer

   typedef class collectionSpaceContainer CS_CONTAINER;
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_SPACE_CONTAINER_H_