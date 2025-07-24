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

   Source File Name = storageFileTrashCan.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileTrashCan.h"
#include "vessel/storageFile.h"

namespace engine
{
namespace vessel
{
   storageFileTrashCan::~storageFileTrashCan()
   {
      clear();
   }

   void storageFileTrashCan::push(storageFile *file)
   {
      SDB_ASSERT(nullptr != file && file->isOpen(), "can not be invalid");
      _fl.push_back(file);
   }

   void storageFileTrashCan::clear()
   {
      ossPoolList<storageFile *>::const_iterator itr = _fl.begin();
      for (; itr != _fl.end(); ++itr)
      {
         storageFile *file = *itr;
         if (nullptr != file && file->isOpen())
         {
            PD_LOG(PDINFO, "will remove file[%s]", file->getFullPath());
            file->destroy();
         }
         
         SAFE_OSS_DELETE(file);
      }
      _fl.clear();
   }
} // namespace vesel

} // namespace engine
