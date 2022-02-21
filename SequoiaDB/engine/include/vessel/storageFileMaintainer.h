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

   Source File Name = storageFileMaintainer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_MAINTAINER_H_
#define VESSEL_STORAGE_FILE_MAINTAINER_H_

#include "vessel/storageFileName.h"
#include "vessel/slice.h"
#include "vessel/storageFile.h"
#include "vessel/vesselOptions.h"

namespace engine
{
namespace vessel
{
   class storageFileMaintainer : public SDBObject
   {
      public:
         storageFileMaintainer(const storagePathOptions *path,
                             SPACE_ID sid);

         ~storageFileMaintainer(){}

      public:
         INT32 createSpaceDir()const;
         INT32 removeSpaceDir()const;
         INT32 createStorageFile(const storageFileName &fn,
                                 const createStorageFileOptions &o,
                                 const slice &userDefinedHead,
                                 storageFile &file)const;
         INT32 openStorageFile(const storageFileName &fn,
                               storageFile &file)const;
         INT32 removeStorageFile(const storageFileName &fn)const;

      private:
         INT32 testBeforeCreating()const;
         BOOLEAN buildFullDir(SPACE_TYPE type, ossPoolString &path)const;
         ossPoolString buildFullPath(const storageFileName &fn)const;

      private:
         const storagePathOptions *_path = nullptr;
         SPACE_ID _sid = INVALID_SPACE_ID;
         CHAR _subDir[MAX_SPACE_DIR_LEN + 1] = {};
   };//class storageFileMaintainer
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_FILE_MAINTAINER_H_