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

   Source File Name = clEntryBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
