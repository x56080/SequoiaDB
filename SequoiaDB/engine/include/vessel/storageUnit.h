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

   Source File Name = storageUnit.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_UNIT_H_
#define VESSEL_STORAGE_UNIT_H_

#include "dms.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFile.h"
#include "vessel/storageUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/strSlice.h"
#include "vessel/mainDataSpace.h"
#include "vessel/mmapPagePointer.h"
#include "vessel/indexSpace.h"
#include "vessel/largeObjectSpace.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class logicalPageSpace;
   class storageFileLoader;
   class storageFile;
   class storagePathOptions;

   class storageUnit : public SDBObject
   {
      public:
         storageUnit();
         ~storageUnit();
         storageUnit(const storageUnit &o) = delete;
         storageUnit &operator=(const storageUnit &o) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _manifest.id.isValid();
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _manifest.id.getSpaceId();
         }

         OSS_INLINE mainDataSpace &getMainDataSpace()
         {
            return _mds;
         }
         OSS_INLINE indexSpace &getIndexSpace()
         {
            return _is;
         }
         OSS_INLINE largeObjectSpace &getLobSpace()
         {
            return _los;
         }
         OSS_INLINE UINT32 getLogicalID()const {return _manifest.id.getLid();}
         OSS_INLINE const storageUnitManifest &getManifest()const {return _manifest;}
         OSS_INLINE const collectionSpaceId &getIdentifier()const {return _manifest.id;}

      public:
         INT32 create(const collectionSpaceId &id,
                      const createSUOptions &options);

         INT32 open(SPACE_ID sid);

         INT32 destroy();
                    
         void close();

      public:
         INT32 getMmapPagePointer(SPACE_TYPE spaceType,
                                  FILE_TYPE fileType,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;

         UINT32 getStoragePageSize(SPACE_TYPE spaceType)const;

      private:
         INT32 createManifestFile(const CHAR *fullPath,
                                  const storageUnitManifest &manifest);

         INT32 loadManifestFile(const CHAR *fullPath,
                                storageUnitManifest &manifest);

         void ensureManifestFileRemoved(const CHAR *fullPath);

         INT32 createMainDataSpace();

         INT32 createIndexSpace();
      private:
         storageUnitManifest _manifest;
         mainDataSpace _mds;
         indexSpace _is;
         largeObjectSpace _los;

   };//class storageUnit
}//namespace vessel
}//namespace engine

#endif // VESSEL_STORAGE_UNIT_H_