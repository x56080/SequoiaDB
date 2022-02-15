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
   threadContext::threadContext(IExecutor *executor,
                                instanceEnv *env):
   _executor(executor),
   _env(env),
   _sba(_staticBuf, _S_BUF_POOL_SIZE)
   {
      SDB_ASSERT(nullptr != executor && nullptr != env, "can not be null");
   }

   DPS_TRANS_ID threadContext::getTransIDOfExecutor()const
   {
      SDB_ASSERT(nullptr != _executor, "can not be null");
      return _executor->getTransID().getOrigTransID();
   }

/////////////threadContextGuard
   threadContextGuard::threadContextGuard(IExecutor *executor,
                                          instanceEnv *env):
   _context(executor, env)
   {
      SDB_ASSERT(nullptr == _T_CONTEXT, "can not override context");
      _T_CONTEXT = &_context;
   }

   threadContextGuard::~threadContextGuard()
   {
      SDB_ASSERT((&_context) == _T_CONTEXT,
                 "context to be detached does not match our instance");
      _T_CONTEXT = nullptr;
   }
} // namespace vessel

} // namespace engine
