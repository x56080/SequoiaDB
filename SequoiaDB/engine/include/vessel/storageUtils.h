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

   Source File Name = storageUtils.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_UTILS_H_
#define VESSEL_STORAGE_UTILS_H_

#include "vessel/storageFileName.h"
#include "ossMemPool.hpp"

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

}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_UTILS_H_