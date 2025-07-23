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

   Source File Name = qgmOptiDelete.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "qgmOptiDelete.hpp"
#include "authAccessControlList.hpp"
#include "boost/exception/diagnostic_information.hpp"

namespace engine
{
   INT32 qgmOptiDelete::_checkPrivileges( ISession *session ) const
   {
      INT32 rc = SDB_OK;
      if ( !session->privilegeCheckEnabled() )
      {
         goto done;
      }

      {
         authActionSet actions;
         actions.addAction( ACTION_TYPE_remove );
         rc = session->checkPrivilegesForActionsOnExact( _collection.toString().c_str(), actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine