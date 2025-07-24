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

   Source File Name = lpageMappingRoot.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_MAPPING_ROOT_H_
#define VESSEL_LPAGE_MAPPING_ROOT_H_

#include "vessel/metaDataUberBlock.h"

#include <array>

namespace engine
{
namespace vessel
{
   class lpageMappingRoot : public SDBObject
   {
      public:
         lpageMappingRoot() {reset();}
         ~lpageMappingRoot() = default;
         lpageMappingRoot(const lpageMappingRoot &o)
         {
            _entries = o._entries;
         }
         lpageMappingRoot &operator=(const lpageMappingRoot &o)
         {
            _entries = o._entries;
            return *this;
         }

      private:
         using _ROOT_ENTRIES = std::array<PAGE_ID, lpmUberBlock::MAPPING_ENTRY_SIZE>;

      public:
         OSS_INLINE UINT32 getEntrySize()const {return _entries.max_size();}
         OSS_INLINE void reset() {_entries.fill(INVALID_PAGE_ID);}
         OSS_INLINE PAGE_ID get(UINT32 pos)const {return _entries.at(pos);}
         OSS_INLINE void set(UINT32 pos, PAGE_ID pid) {_entries[pos] = pid;}

      public:
         BOOLEAN none()const;
         void init(const lpmUberBlock *uberBlock);

         /// return false if nothing changed.
         BOOLEAN update(lpmUberBlock *uberBlock)const;
         void merge(const lpageMappingRoot &o);
      private:
         _ROOT_ENTRIES _entries;
   };//class lpageMappingRoot
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_ROOT_H_