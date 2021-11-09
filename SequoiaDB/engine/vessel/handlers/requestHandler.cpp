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

   Source File Name = requestHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/requestHandler.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   void requestHandler::init(instanceEnv *env,
                               IExecutor *executor,
                               outerResource *resource)
   {
      SDB_ASSERT(NULL != env, "can not be null");
      SDB_ASSERT(NULL != executor, "can not be null");
      SDB_ASSERT(NULL != resource && resource->isValid(), "can not be null");
      
      _env = env;
      _executor = executor;
      _outerResource = resource;
      return;
   }

   BOOLEAN requestHandler::isInitialized()const
   {         
      return NULL != _env &&
             NULL != _executor &&
             NULL != _outerResource &&
             _outerResource->isValid();
         
   }
}//namespace vessel
}//namespace engine