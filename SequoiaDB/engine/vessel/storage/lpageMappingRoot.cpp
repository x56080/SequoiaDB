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

   Source File Name = lpageMappingRoot.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lpageMappingRoot.h"
#include "pdTrace.hpp"

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

   BOOLEAN lpageMappingRoot::update(lpmUberBlock *uberBlock)const
   {
      SDB_ASSERT(nullptr != uberBlock, "can not be invalid");
      BOOLEAN r = FALSE;
      for (UINT32 i = 0; i < _entries.size(); ++i)
      {
         if (uberBlock->mappingEntries[i] != _entries[i])
         {
            uberBlock->mappingEntries[i] = _entries[i];
            r = TRUE;
         }
      }
      return r;
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
