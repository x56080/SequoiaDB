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

   Source File Name = atomicOperationList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/atomicOperationList.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   atomicOperationList::atomicOperationList()
   {}

   atomicOperationList::~atomicOperationList()
   {
      fini();
   }

   void atomicOperationList::fini()
   {
      _list.clear();
      _waitingTail = FALSE;
      _nomorePushing = FALSE;
      return;
   }

   DPS_LSN_OFFSET atomicOperationList::getOplistLsn()const
   {
      return isEmpty() ? DPS_INVALID_LSN_OFFSET : _list.front();
   }

   void atomicOperationList::abort(requestContext *context)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      fini();
      return;
   }

   INT32 atomicOperationList::push(DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      
      if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == lsn))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isReadonly())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      _list.push_back(lsn);

      if (isWatingHead())
      {
         _nomorePushing = TRUE;
         _waitingTail = FALSE;
      }
   done:
      return rc;
   error:
      goto done;
   }

   DPS_LSN_OFFSET atomicOperationList::getLastLsn()const
   {
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;
      if (!_list.empty())
      {
         lsn = _list.back();
      }
      return lsn;
   }

   void atomicOperationList::setWaitingTail()
   {
      //SDB_ASSERT(!isWatingHead(), "impossible");
      SDB_ASSERT(!isReadonly(), "impossible");
      _waitingTail = TRUE;
      return;
   }

}//namespace vessel
}//namespace engine
