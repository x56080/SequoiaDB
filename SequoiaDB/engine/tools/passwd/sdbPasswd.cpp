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

   Source File Name = sdbPasswd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include "ossVer.h"
#include "ossUtil.hpp"
#include "passwdOptions.hpp"
#include "utilPasswdTool.hpp"
#include "utilCipherFile.hpp"
#include "utilCipherMgr.hpp"
#include "pd.hpp"
#include <iostream>

using namespace engine;
using namespace passwd;
using namespace std;

#define SDBPASSWD_LOG "sdbpasswd.log"

INT32 main(INT32 argc, char* argv[])
{
   INT32 rc = SDB_OK ;
   passwdOptions options ;

   sdbEnablePD( SDBPASSWD_LOG ) ;
   setPDLevel( PDINFO ) ;
   utilCipherMgr mgr ;
   utilCipherFile cipherfile ;

   try
   {
      rc = options.parseCmd( argc, argv ) ;
      if ( rc )
      {
         options.printHelpInfo();
         if ( SDB_PMD_HELP_ONLY == rc )
         {
            rc = SDB_OK;
            goto done;
         }
         goto error;
      }

      cipherfile.initFile( options.file(), utilCipherFile::WRole ) ;
      if ( SDB_OK != rc )
      {
         std::cerr << "init cipherfile failed" << std::endl ;
         PD_LOG( PDERROR, "cipherfFile initialize error [%d]", rc ) ;
         goto error;
      }

      rc = mgr.init( &cipherfile );
      if ( SDB_OK != rc )
      {
         std::cerr << "init utilCipherMgr failed" << std::endl ;
         PD_LOG( PDERROR, "utilCipherMgr initialize error [%d]", rc ) ;
         goto error;
      }
      if ( passwdOptions::OpAddUser == options.mode() )
      {
         string password ;
         if ( !options.hasPassword() )
         {
            password = utilPasswordTool::interactivePasswdInput() ;
         }
         else
         {
            password = options.password() ;
         }

         rc = mgr.addUser( options.user(), options.token(), password );
         if ( SDB_OK != rc )
         {
            std::cerr << "adduser failed" << std::endl ;
            PD_LOG( PDINFO, "utilCipherMgr adduser [%s], error [%d]",
                    options.user().c_str(), rc ) ;
         }
      }
      else if ( passwdOptions::OpRemoveUser == options.mode() )
      {
         rc = mgr.removeUser( options.user() );
         if ( SDB_OK != rc )
         {
            std::cerr << "removeuser failed" << std::endl ;
            PD_LOG( PDINFO, "utilCipherMgr removeuser [%s], error [%d]",
                    options.user().c_str(), rc ) ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         std::cerr << "unexpected error" << std::endl ;
         PD_LOG( PDERROR, "utilCipherMgr illegal method" ) ;
         goto error;
      }
   }
   catch( exception &e )
   {
      std::cerr << "unexpected error" << endl ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}
