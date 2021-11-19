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
            return INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }

         OSS_INLINE mainDataSpace &getMainDataSpace()
         {
            return _mds;
         }
         OSS_INLINE indexSpace &getIndexSpace()
         {
            return _is;
         }

      public:
         INT32 create(requestContext *context,
                      const createSUOptions &options);

         INT32 open(requestContext *context);

         INT32 destroy(requestContext *context);
                    
         void close();

      public:
         INT32 getMmapPagePointer(SPACE_TYPE spaceType,
                                  FILE_TYPE fileType,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;

         INT32 getCoreArgs(SPACE_TYPE spaceType,
                           FILE_TYPE fileType,
                           storageCoreArgs &args);

      public:
         INT32 openStorageFile(const vesselFileName &fn,
                               storageFile *file)const;

         INT32 createStorageFile(const vesselFileName &fn,
                                 const createStorageFileOptions &o,
                                 const slice &userDefinedHead,
                                 storageFile *file)const;

         INT32 getDirPathOfType(SPACE_TYPE type,
                                ossPoolString &dir)const;

         INT32 destroyStorageFile(const vesselFileName &fn);

      private:

         INT32 testAllDirsBeforeCreating(const storagePathOptions &path,
                                         const strSlice &dir)const;

         INT32 testAllDirsBeforeOpenning(const storagePathOptions &path,
                                         const strSlice &dir)const;

         INT32 createMainDataDir(const storagePathOptions &path,
                                 const strSlice &dir);


         INT32 ensureMainDataDirRemoved(const storagePathOptions &path,
                                        const strSlice &dir);

         INT32 createStatusFile(const storagePathOptions &path,
                                const strSlice &dir,
                                SPACE_ID sid);

         INT32 removeStatusFile(const storagePathOptions &path,
                                const strSlice &dir,
                                SPACE_ID sid);

         INT32 testStatusFile(const strSlice &fullDir,
                              SPACE_ID sid,
                              BOOLEAN &exists)const;

         INT32 testDir(const CHAR *fullPath,
                       UINT32 &subCount);
         INT32 createOtherDirs(const storagePathOptions &path,
                               const strSlice &dir);

         INT32 ensureOtherDirRemoved(const storagePathOptions &path,
                                     const strSlice &dir);

         INT32 rollbackCreating(requestContext *context,
                                const storagePathOptions &path,
                                const strSlice &dir);

      private:
         INT32 createMainDataSpace(requestContext *context,
                                   UINT32 secretValue,
                                   const storageCoreArgs &args);

         INT32 createIndexSpace(requestContext *context,
                                UINT32 secretValue,
                                const storageCoreArgs &args);
      private:
         void buildFullDir(const storagePathOptions *path,
                           SPACE_TYPE type,
                           FILE_TYPE ftype,
                           ossPoolString &dir)const;
      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         ossPoolString _unitEntryDir;
         const storagePathOptions *_path = NULL;
         mainDataSpace _mds;
         indexSpace _is;

   };//class storageUnit
}//namespace vessel
}//namespace engine

#endif // VESSEL_STORAGE_UNIT_H_