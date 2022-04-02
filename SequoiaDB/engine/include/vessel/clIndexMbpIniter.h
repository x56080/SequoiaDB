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

   Source File Name = clIndexMbpIniter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CL_INDEX_META_BLOCK_PAGE_INITER_
#define VESSEL_CL_INDEX_META_BLOCK_PAGE_INITER_

#include "vessel/pageInitializer.h"

namespace engine
{
namespace vessel
{
   class clIndexMbpIniter : public pageInitializer
   {
      public:
         virtual INT32 initPage(requestContext *context,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb);
   }; // class clIndexMbpIniter

} // namespace vessel
} // namespace engine

#endif //VESSEL_CL_INDEX_META_BLOCK_PAGE_INITER_