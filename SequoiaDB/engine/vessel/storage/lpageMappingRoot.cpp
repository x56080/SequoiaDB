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

   Source File Name = lpageMappingRoot.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpageMappingRoot.h"

namespace engine
{
namespace vessel
{
   BOOLEAN lpageMappingRoot::none()const
   {
      BOOLEAN r = TRUE;
      for (UINT32 i = 0; i < _entries.size(); ++i)
      {
         if (INVALID_PAGE_ID != _entries.at(i))
         {
            r = FALSE;
            break;
         }
      }

      return r;
   }

   void lpageMappingRoot::init(const lpmUberBlock *uberBlock)
   {
      reset();
      if (nullptr != uberBlock)
      {
         for (UINT32 i = 0; i < _entries.size(); ++i)
         {
            _entries[i] = uberBlock->mappingEntries[i];
         }
      }
   }

   
   void lpageMappingRoot::merge(const lpageMappingRoot &o)
   {
      for (UINT32 i = 0; i < _entries.size(); ++i)
      {
         if (INVALID_PAGE_ID != o._entries[i])
         {
            _entries[i] = o._entries[i];
         }
      }
      return;
   }
} // namespace vessel

} // namespace engine
