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

namespace engine
{

   string _utilPasswordTool::interactivePasswdInput()
   {
      CHAR* line = NULL ;
      string passwd ;
      setEchoOff() ;
      if ( ( line = linenoise( "password:" ) ) != NULL )
      {
         passwd = line ;
         SDB_OSS_ORIGINAL_FREE( line ) ;
      }
      setEchoOn() ;
      return passwd ;
   }

   INT32 _utilPasswordTool::getPasswdByCipherFile( const string &user,
                                                   const string &token,
                                                   const string &cipherFile,
                                                   string &connectionUserName,
                                                   string &password )
   {
      INT32  rc = SDB_OK ;

      string filePath = cipherFile ;

      if ( filePath.empty() )
      {
         filePath = "./passwd" ;
      }

      _cipherfile.initFile( filePath, utilCipherFile::RRole ) ;

      rc = _cipherMgr.init( &_cipherfile ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _cipherMgr.getConnectionUserName( user, connectionUserName ) ;

      rc = _cipherMgr.getPasswd( user, token, password ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}