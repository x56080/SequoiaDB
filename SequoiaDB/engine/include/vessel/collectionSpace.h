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
#include "vessel/csMetaBlockPage.h"
#include "vessel/strSlice.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/slice.h"
#include "vessel/storageUnit.h"
#include "ossRWMutex.hpp"
#include "vessel/lazyArray.hpp"
#include "vessel/collectionOptions.h"
#include "vessel/objectIdentifier.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/fixedBitset.hpp"
#include "vessel/objectHolder.hpp"
#include "vessel/csProperties.h"

namespace engine
{
namespace vessel
{
   class collection;
   class requestContext;
   class listCLCursor;

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
            return _properties.csid.getSpaceId();
         }
         OSS_INLINE const CHAR *getCSName()const
         {
            return _properties.name.c_str();
         }
         OSS_INLINE strSlice getCSNameSlice()const
         {
            return strSlice(_properties.name.c_str(), _properties.name.size());
         }
         OSS_INLINE UINT32 getUniqueID()const
         {
            return _properties.csid.getUniqueId();
         }

         OSS_INLINE UINT32 getStatus()const
         {
            return _properties.status;
         }
         OSS_INLINE UINT32 getFlags()const
         {
            return _properties.flags;
         }
         OSS_INLINE BOOLEAN isOnline()const
         {
            return CS_STATUS_ONLINE == getStatus();
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _properties.csid.getLid();
         }
         OSS_INLINE storageUnit *getSU()const
         {
            return _su;
         }
         OSS_INLINE const collectionSpaceId &getIdentifier()const
         {
            return _properties.csid;
         }

         OSS_INLINE const csProperties *getProperties()const {return &_properties;}
      public:
         INT32 create(requestContext *context,
                      const strSlice &name,
                      storageUnit *su,
                      DPS_LSN_OFFSET lsn);

         INT32 open(requestContext *context,
                    storageUnit *su);

         void close();

      public:
         INT32 createCL(requestContext *context,
                        const strSlice &clName, 
                        utilCLInnerID clInnerId,
                        const createCLOptions &options);

         INT32 removeCL(requestContext *context,
                        const collectionId &identifier);
      public:

         UINT32 getCollectionCount();

         INT32 listCollections(requestContext *context,
                               listCLCursor *cursor);

         INT32 dump(requestContext *context,
                    bson::BSONObj &record);

         INT32 getCollectionByName(requestContext *context,
                                   const strSlice &clName, 
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 getCollectionByMBID(requestContext *context,
                                   CL_MB_ID mbID,
                                   UINT32 logicalID,
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 getCollectionById(requestContext *context,
                                 const collectionId &id,
                                 OSS_LATCH_MODE mode,
                                 collection **obj);

         INT32 getCollectionByCLInnerID(requestContext *context,
                                        utilCLInnerID innerID,
                                        OSS_LATCH_MODE mode,
                                        collection **obj);
      private:
         struct _NAME_LESS
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrncmp(l, r, DMS_COLLECTION_NAME_SZ) < 0;
            }
         };//struct _CS_NAME_LESS

         typedef ossPoolMap<const CHAR *, CL_MB_ID, _NAME_LESS> NAME_INDEX;
         typedef ossPoolMap<utilCLInnerID, CL_MB_ID> ID_INDEX;
         typedef ossPoolSet<ossPoolString> _NAME_SET;
         typedef ossPoolSet<utilCLInnerID> _INNER_ID_SET; 

         typedef objectHolder<collection> _CL_HOLDER;
         typedef objectHolderGroup<collection, 64> _CL_HOLDER_GROUP;

         static_assert(65535 == MAX_CL_MB_COUNT, "msut be 65535");
         static constexpr UINT32 _ALLOCATOR_SIZE = 65536 / _CL_HOLDER_GROUP::CAPACITY;
      private:
         void fini();
         INT32 initInMemStructures();

         INT32 createCSNameFile()const;

         INT32 removeCSNameFile()const;

         INT32 _initMetaBlock(requestContext *context,
                              const csMetaBlock &block,
                              DPS_LSN_OFFSET lsn);
         INT32 _updateMetaBlock(requestContext *context,
                                const csMetaBlock &block,
                                UINT64 mask);

         INT32 _loadMetaBlock(requestContext *context,
                              csMetaBlock &block);

         void _initProperties(const csMetaBlock &block);

         void _exportMetaBlock(csMetaBlock &block);

      private:

         INT32 initCollectionsFromDisk(requestContext *context);
         INT32 initCollection(requestContext *context,
                              const clMetaBlock *block);

         INT32 getCollectionHolder(CL_MB_ID mbID, _CL_HOLDER **holder);

         INT32 ensureCLMetaBlockPage(requestContext *context,
                                     CL_MB_ID mbID);

         INT32 ensureCLIndexMetaBlockPage(requestContext *context,
                                          CL_MB_ID mbID);

         INT32 reserveCL(requestContext *context,
                         const strSlice &clName,
                         utilCLInnerID innerID,
                         CL_MB_ID &mbID,
                         UINT32 &logicalID);

         void releaseCL(const strSlice &clName,
                        utilCLInnerID innerID,
                        CL_MB_ID mbID);

         void endCreatingCL(collection *obj);

         void prepareToRemoveCL(const ossPoolString &clName,
                                utilCLInnerID innerId);

         INT32 reserveCLObj(CL_MB_ID &mbID);
         INT32 ensureCLObj(CL_MB_ID mbID, _CL_HOLDER **holder);
         void releaseCLObj(CL_MB_ID mbID);

      private:
         BOOLEAN upperBoundCLName(const strSlice &clName,
                                  CL_MB_ID &mbID)const;

         INT32 insertIntoFormalIndexes(collection *obj);
         BOOLEAN existsInFormalIndexes(const strSlice &clName,
                                       utilCLInnerID innerID)const;
         BOOLEAN existsInUnformalIndexes(const strSlice &clName,
                                         utilCLInnerID innerID)const;

         BOOLEAN find(const strSlice &clName,
                      CL_MB_ID &mbID)const;
         BOOLEAN find(utilCLInnerID innerID,
                      CL_MB_ID &mbID)const;
        
      private:
         BOOLEAN _isOpen = FALSE;
         storageUnit *_su = NULL;
         ossSpinSLatchPOSIX _latch;
         csProperties _properties;

         fixedBitset<_ALLOCATOR_SIZE> _allocator;
         lazyArray<_CL_HOLDER_GROUP> _collections;

         ///formal indexes
         NAME_INDEX _clNameIndex;
         ID_INDEX _innerIdIndex;

         ///unformal indexes
         _NAME_SET _unformalNameIndex;
         _INNER_ID_SET _unformalInnerIdIndex;
   };//class collectionSpace

   typedef shallowPointer<collectionSpace> CS_OBJ_PTR;
}
}

#endif//VESSEL_COLLECTION_SPACE_H_