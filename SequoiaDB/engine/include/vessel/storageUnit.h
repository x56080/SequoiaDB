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

   Source File Name = storageUnit.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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