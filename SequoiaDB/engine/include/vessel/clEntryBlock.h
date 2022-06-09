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

   Source File Name = clEntryBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CL_ENTRY_BLOCK_H_
#define VESSEL_CL_ENTRY_BLOCK_H_

#include "vessel/collectionProperties.h"
#include "vessel/freeSpaceMap.h"
#include "vessel/indexObjectMap.h"
#include "vessel/clMetaBlockPage.h"

#include <array>
#include <atomic>
#include <mutex>

namespace engine
{
namespace vessel
{
   class clEntryBlock : public SDBObject
   {
      friend class collection;
      public:
         clEntryBlock()
         {
            routeMap.fill(INVALID_PAGE_ID);
         }
         ~clEntryBlock() = default;

         clEntryBlock(const clEntryBlock &) = delete;
         clEntryBlock &operator=(const clEntryBlock &) = delete;
      
      public:
         OSS_INLINE const collectionProperties *getProperties()const {return &_properties;}
         void reset();

      private:
         OSS_INLINE ossRWMutex *getOpLock() {return &_oplock;}
         OSS_INLINE std::mutex &getExtLock() {return _extlock;}

      private:
         collectionProperties _properties;
         atomic_uint _lvl0Count = {0};
         atomic_uint _rdpCount = {0};
         std::array<PAGE_ID, COLLECTION_ROUTE_PAGE_SLOT_COUNT> routeMap;
         freeSpaceMap _fsm;
         indexObjectMap _indexes;

         ossRWMutex _oplock;
         std::mutex _extlock;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_CL_ENTRY_BLOCK_H_
