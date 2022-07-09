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

   Source File Name = lpageMappingPteCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPAGE_MAPPING_PTE_CTX_H_
#define VESSEL_LPAGE_MAPPING_PTE_CTX_H_

#include "vessel/lpageMappingRoot.h"
#include "vessel/fixedBitset.hpp"
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
         ~lpageMappingPteCtx();
         lpageMappingPteCtx(const lpageMappingPteCtx &) = delete;
         lpageMappingPteCtx &operator=(const lpageMappingPteCtx &) = delete;

      public:
         BOOLEAN none()const;
         void reset();

      private:

      private:
         std::mutex _privatePathLocker;
         lpageMappingRoot _root;

         /// pids of meta file to be free after publishing.
         /// protected by _privatePathLocker
         ossPoolSet<PAGE_ID> _obsoleteSet; 
   };//class lpageMappingPteCtx
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_PTE_CTX_H_