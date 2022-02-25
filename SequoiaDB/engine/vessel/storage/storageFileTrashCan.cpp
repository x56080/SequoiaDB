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

   Source File Name = storageFileTrashCan.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
