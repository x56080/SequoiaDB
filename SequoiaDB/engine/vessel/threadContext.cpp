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

   Source File Name = threadContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   threadContext::threadContext():
   _sba(_staticBuf, _S_BUF_POOL_SIZE)
   {
      
   }

   DPS_TRANS_ID threadContext::getTransID()const
   {
      SDB_ASSERT(nullptr != _executor, "can not be null");
      return _executor->getTransID().getOrigTransID();
   }

   CHAR *threadContext::allocateBuffer(UINT32 size)
   {
      return _sba.allocate(size);
   }

   void threadContext::releaseBuffer(void *buffer)
   {
      if (nullptr != buffer)
      {
         _sba.release((CHAR *)buffer);
      }
   }

   BOOLEAN threadContext::hasUnfreeBuffer()const
   {
      return _sba.hasUnfreeBuffer();
   }


/////////////threadContextOnwer
   OSS_THREAD_LOCAL threadContext *threadContextOnwer::_T_CONTEXT = nullptr;

   threadContextOnwer::threadContextOnwer(IExecutor *executor,
                                          instanceEnv *env)
   {
      SDB_ASSERT(nullptr == _T_CONTEXT, "can not override context");
      SDB_ASSERT(nullptr != executor, "can not be null");
      SDB_ASSERT(nullptr != env, "can not be null");
      _context._executor = executor;
      _context._env = env;
      _T_CONTEXT = &_context;
   }

   threadContextOnwer::~threadContextOnwer()
   {
      SDB_ASSERT((&_context) == _T_CONTEXT,
                  "context to be detached does not match our instance");
      _T_CONTEXT = nullptr;
   }
} // namespace vessel

} // namespace engine
