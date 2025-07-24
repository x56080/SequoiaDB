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

   Source File Name = lpageMappingPteCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_MAPPING_PTE_CTX_H_
#define VESSEL_LPAGE_MAPPING_PTE_CTX_H_

#include "vessel/lpageMappingRoot.h"
#include "ossMemPool.hpp"

#include <mutex>

namespace engine
{
namespace vessel
{
   class lpageMappingPteCtx : public SDBObject
   {
      friend class lpageMapping;
      public:
         lpageMappingPteCtx() = default;
         ~lpageMappingPteCtx() = default;
         lpageMappingPteCtx(const lpageMappingPteCtx &) = delete;
         lpageMappingPteCtx &operator=(const lpageMappingPteCtx &) = delete;

      public:
         void reset();
         BOOLEAN isBrandNewPid(PAGE_ID pid, BOOLEAN lock=TRUE);
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _root.none();
         }
      private:
         std::mutex _pathLock;
         lpageMappingRoot _root;

         /// pids of meta file to be free after publishing.
         /// protected by _pathLock
         ossPoolSet<PAGE_ID> _obsoleteSet; 

         /// pids of meta file allocated.
         /// protected by _pathLock
         ossPoolSet<PAGE_ID> _brandNewSet;
   };//class lpageMappingPteCtx
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_PTE_CTX_H_