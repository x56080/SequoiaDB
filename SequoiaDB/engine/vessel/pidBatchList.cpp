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

   Source File Name = pidBatchList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/pidBatchList.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 RESERVE_SIZE = 64;

   void pidBatchList::push(PAGE_ID pid)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
   
      if (!_bl.empty() && _bl.back().size() < RESERVE_SIZE)
      {
         _bl.back().push_back(pid);
      }
      else
      {
         BATCH batch;
         batch.reserve(RESERVE_SIZE);
         batch.push_back(pid);
         _bl.push_back(std::move(batch));
      }

      return;
   }

   void pidBatchList::transferTo(pidBatchList &o)
   {
      o._bl.merge(std::move(_bl));
   }
} // namespace vessel

} // namespace engine
