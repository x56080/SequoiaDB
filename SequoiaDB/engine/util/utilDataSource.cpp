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

   Source File Name = utilDataSource.cpp

   Descriptive Name = Data source utilities

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/13/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilDataSource.hpp"

namespace engine
{
   const CHAR *sdbDSTransModeDesc( SDB_DS_TRANS_PROPAGATE_MODE mode )
   {
      const CHAR *desc = NULL ;
      switch ( mode )
      {
         case DS_TRANS_PROPAGATE_NEVER:
            desc = VALUE_NAME_NEVER ;
            break ;
         case DS_TRANS_PROPAGATE_NOT_SUPPORT:
            desc = VALUE_NAME_NOT_SUPPORT ;
            break ;
         default:
            desc = "" ;
            break ;
      }
      return desc ;
   }

   SDB_DS_TRANS_PROPAGATE_MODE sdbDSTransModeFromDesc( const CHAR *descStr )
   {
      SDB_ASSERT( descStr, "Propagate mode string is NULL" ) ;
      if ( 0 == ossStrcasecmp( descStr, VALUE_NAME_NEVER ) )
      {
         return DS_TRANS_PROPAGATE_NEVER ;
      }
      else if ( 0 == ossStrcasecmp( descStr, VALUE_NAME_NOT_SUPPORT ) )
      {
         return DS_TRANS_PROPAGATE_NOT_SUPPORT ;
      }
      else
      {
         return DS_TRANS_PROPAGATE_INVALID ;
      }
   }
}
