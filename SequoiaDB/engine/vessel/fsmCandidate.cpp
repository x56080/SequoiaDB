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

   Source File Name = fsmCandidate.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/fsmCandidate.h"
#include "ossMemPool.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 makeFsmCandidateSharedInfoPtr(PAGE_ID lpid,
                                       INT32 lvl,
                                       fsmCandidate::SHARED_INFO_PTR &sptr)
   {
      INT32 rc = SDB_OK;
      ossPoolAllocator<fsmCandidate::mutableInfo> alloc;
      sptr.reset();

      /// allocate_shared can avoid twice memory allocating(obj and control block).
      sptr = std::allocate_shared<fsmCandidate::mutableInfo>(alloc, lpid, lvl);
      if (NULL == sptr.get())
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine