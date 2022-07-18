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

   Source File Name = lpageMappingRoot.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         void merge(const lpageMappingRoot &o);
      private:
         _ROOT_ENTRIES _entries;
   };//class lpageMappingRoot
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_ROOT_H_