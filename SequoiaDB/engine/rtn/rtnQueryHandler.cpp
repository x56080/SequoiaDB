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

   Source File Name = rtnQueryHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnQueryHandler.hpp"

namespace engine
{
   INT32 rtnQueryHandler::launch(pmdEDUCB *cb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _options, "can not be null");
      SDB_ASSERT(NULL != cb, "can not be null");
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
