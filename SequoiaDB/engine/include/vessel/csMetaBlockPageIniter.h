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

   Source File Name = csMetaBlockPageIniter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CS_META_BLOCK_PGAE_INITER_H_
#define VESSEL_CS_META_BLOCK_PGAE_INITER_H_

#include "vessel/pageInitializer.h"

namespace engine
{
namespace vessel
{
   struct csMetaBlock;
   
   class csMetaBlockPageIniter : public pageInitializer
   {
      public:
         virtual INT32 initPage(requestContext *context,
                                 PAGE_ID lpid,
                                 PAGE_SNAPSHOT_VERION psv,
                                 runtimePageBuffer *rpb)override;

      public:
         void set(const csMetaBlock *block) {_block = block;}
      private:
         const csMetaBlock *_block = nullptr;
   };//class csMetaBlockPageIniter
} // namespace vessel

} // namespace engine


#endif//VESSEL_CS_META_BLOCK_PGAE_INITER_H_
