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

   Source File Name = storageUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_UTILS_H_
#define VESSEL_STORAGE_UTILS_H_

#include "vessel/storageFileName.h"
#include "ossMemPool.hpp"
#include "vessel/storageManifest.h"
#include "../bson/bson.hpp"

namespace engine
{
namespace vessel
{
   class storageFile;

   INT32 removeFile(const strSlice &dir,
                    const storageFileName &fn);

   /// rename file between different shadow suffix.
   INT32 renameFileShadowSuffix(const strSlice &dir,
                                BOOLEAN replaceNewFile,
                                const storageFileName &oldFileName,
                                const storageFileName &newFileName);

/*
   INT32 renameToFormalAndReopen(const strSlice &dir,
                                 BOOLEAN replaceNewFile,
                                 UINT32 shadowSuffix,
                                 storageFile *file);*/

   /// file types must be specified.
   INT32 createFileListUnderDir(const strSlice &dir,
                                SPACE_ID sid,
                                SPACE_TYPE spaceType,
                                UINT32 fileTypesSize,
                                const FILE_TYPE *fileTypes,
                                BOOLEAN removeTmpFile,
                                STORAGE_FILE_NAME_LIST &fl);

   BOOLEAN buildSpaceDirName(SPACE_ID sid, UINT32 bufLen, CHAR *buf);
   BOOLEAN parseSpaceDirName(const strSlice &dirName, SPACE_ID *sid);

   bson::BSONObj buildSuManifestObj(const storageUnitManifest &manifest);

   BOOLEAN parseSuManifestObj(const bson::BSONObj &obj,
                              storageUnitManifest &manifest);

}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_UTILS_H_