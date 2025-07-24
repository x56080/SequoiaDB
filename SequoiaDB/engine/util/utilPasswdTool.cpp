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

   Source File Name = utilPasswdTool.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilPasswdTool.hpp"
#include "linenoise.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>

BOOLEAN _utilPasswordTool::interactivePasswdInput( string &passwd )
{
   CHAR*   line = NULL ;
   BOOLEAN isNormalInput = FALSE ;
   setEchoOff() ;
   if ( ( line = linenoise( "password: " ) ) != NULL )
   {
      // If we enter Ctrl + c, we will not enter this branch.
      passwd = line ;
      SDB_OSS_ORIGINAL_FREE( line ) ;
      isNormalInput = TRUE ;
   }
   setEchoOn() ;
   return isNormalInput ;
}

INT32 _utilPasswordTool::getPasswdByCipherFile( const string &userFullName,
                                                const string &token,
                                                string &filePath,
                                                string &password )
{
   INT32  rc = SDB_OK ;

   rc = _cipherfile.init( filePath, R_ROLE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _cipherMgr.init( &_cipherfile ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _cipherMgr.getPasswd( filePath, userFullName, token, password ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

