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

   Source File Name = storageFileMaintainer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_FILE_MAINTAINER_H_
#define VESSEL_STORAGE_FILE_MAINTAINER_H_

#include "vessel/storageFileName.h"
#include "vessel/slice.h"
#include "vessel/storageFile.h"
#include "vessel/vesselOptions.h"
#include "vessel/storageFileLoader.h"
#include "vessel/invalidFileReason.h"

namespace engine
{
namespace vessel
{
   class storageFileMaintainer : public SDBObject
   {
      public:
         storageFileMaintainer(){}
         storageFileMaintainer(const storagePathOptions *path,
                               SPACE_ID sid);

         ~storageFileMaintainer(){}

      public:
         OSS_INLINE BOOLEAN isValid()const {return INVALID_SPACE_ID != _sid;}
         INT32 init(const storagePathOptions *path,
                    SPACE_ID sid);
         void reset();
         INT32 createSpaceDirs()const;
         INT32 removeSpaceDirs()const;
         INT32 createStorageFile(const storageFileName &fn,
                                 const createStorageFileOptions &o,
                                 storageFile &file)const;
         INT32 openStorageFile(const storageFileName &fn,
                               UINT32 flags,
                               storageFile &file,
                               invalidFileReason *reason=nullptr)const;
         INT32 removeStorageFile(const storageFileName &fn)const;

         ossPoolString buildFullPath(const storageFileName &fn)const;
         ossPoolString buildFullPath(SPACE_TYPE type, const CHAR *fileName)const;

         INT32 load(storageFileLoader &loader)const;
         INT32 testBeforeOpenning()const;

         ///always build lobd dir when type is lob
         BOOLEAN buildFullDir(SPACE_TYPE type,
                              ossPoolString &path)const;
         
      private:
         ossPoolString _build(const CHAR *l, const CHAR *r)const;
      private:
         const storagePathOptions *_path = nullptr;
         SPACE_ID _sid = INVALID_SPACE_ID;
         CHAR _subDir[MAX_SPACE_DIR_LEN + 1] = {};
   };//class storageFileMaintainer
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_FILE_MAINTAINER_H_