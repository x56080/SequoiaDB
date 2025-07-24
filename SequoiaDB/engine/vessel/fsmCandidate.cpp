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

   Source File Name = fsmCandidate.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/fsmCandidate.h"
#include "ossMemPool.hpp"
#include "pdTrace.hpp"
#include "utilSharedPtrMaker.hpp"

namespace engine
{
namespace vessel
{
   INT32 makeFsmCandidateSharedInfoPtr(PAGE_ID lpid,
                                       INT32 lvl,
                                       fsmCandidate::SHARED_INFO_PTR &sptr)
   {
      INT32 rc = SDB_OK;
      /*
      ossPoolAllocator<fsmCandidate::mutableInfo>::Type alloc;
      sptr.reset();

      /// allocate_shared can avoid twice memory allocating(obj and control block).
      sptr = std::allocate_shared<fsmCandidate::mutableInfo,
                                  typename ossPoolAllocator<fsmCandidate::mutableInfo>::Type>
                                  (alloc, lpid, lvl);
      if (NULL == sptr.get())
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ///For now, allocator is under c++98 standard.
      sptr->_lvl = lvl;
      sptr->_lpid = lpid;
      */

      sptr = makeSharedPtrFromPool<fsmCandidate::mutableInfo>(lpid, lvl);
      if (!sptr)
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