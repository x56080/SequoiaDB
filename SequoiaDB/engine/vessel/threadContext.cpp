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

   Source File Name = threadContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

   DPS_TRANS_ID threadContext::getTransIDOfExecutor()const
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
