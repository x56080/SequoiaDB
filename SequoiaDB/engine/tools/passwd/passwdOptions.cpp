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

   Source File Name = passwdOptions.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "ossUtil.hpp"
#include "ossMem.h"
#include "ossIO.hpp"
#include "ossTypes.h"
#include "utilPasswdTool.hpp"
#include "pd.hpp"
#include "utilParam.hpp"
#include "passwdOptions.hpp"

namespace passwd
{
   using namespace engine;
   #define DEFAULT_FILE         ( "passwd" )
   #define OPTIONS_HELP         ( "help" )
   #define OPTIONS_ADDUSER      ( "adduser" )
   #define OPTIONS_RMUSER       ( "removeuser" )
   #define OPTIONS_CLUSTER      ( "cluster" )
   #define OPTIONS_TOKEN        ( "token" )
   #define OPTIONS_FILE         ( "file" )
   #define OPTIONS_PASSWD       ( "password" )

   #define COMMANDS_ADD_PARAM_OPTIONS_BEGIN(des)  des.add_options()
   #define COMMANDS_ADD_PARAM_OPTIONS_END ;
   #define COMMANDS_STRING(a, b) (string(a) + string(b)).c_str()

   #define PASSWD_GENERAL_OPTIONS \
   (COMMANDS_STRING(OPTIONS_HELP, ",h"), "help") \
   (COMMANDS_STRING(OPTIONS_ADDUSER,",a"), po::value<string>(), "add a user")\
   (COMMANDS_STRING(OPTIONS_RMUSER,",r"), po::value<string>(), "remove a user")\
   (COMMANDS_STRING(OPTIONS_TOKEN,",t"), po::value<string>(), "password encryption token")\
   (COMMANDS_STRING(OPTIONS_FILE,",f"), po::value<string>(), "cipherfile location, default ./passwd")\
   (COMMANDS_STRING(OPTIONS_PASSWD,",p"), implicit_value<string>(""), "password")

   passwdOptions::passwdOptions(): _cmdParsed( FALSE ), _file( DEFAULT_FILE )
   {
   }

   passwdOptions::~passwdOptions()
   {
   }

   BOOLEAN passwdOptions::hasHelp() const
   {
      return _cmdHas( OPTIONS_HELP ) ;
   }

   BOOLEAN passwdOptions::hasPassword() const
   {
      return _cmdHas( OPTIONS_PASSWD ) ;
   }

   INT32 passwdOptions::printHelpInfo() const
   {
      po::options_description general( "command Options" ) ;
      general.add_options()PASSWD_GENERAL_OPTIONS ;
      cout << general << endl ;
      return SDB_OK ;
   }

   INT32 passwdOptions::parseCmd( INT32 argc,CHAR* argv [ ] )
   {
      INT32 rc = SDB_OK ;

      COMMANDS_ADD_PARAM_OPTIONS_BEGIN (_cmdDesc)
           PASSWD_GENERAL_OPTIONS
      COMMANDS_ADD_PARAM_OPTIONS_END

      rc = utilReadCommandLine( argc, argv, _cmdDesc, _cmdVm, FALSE ) ;
      if ( SDB_OK != rc )
      {
         cerr << "fail to read command line, rc = " << rc << endl ;
         goto error ;
      }
      _cmdParsed = true ;

      if ( !_cmdHas( OPTIONS_ADDUSER ) && !_cmdHas( OPTIONS_RMUSER ) )
      {
         if ( _cmdHas( OPTIONS_HELP ) )
         {
            rc = SDB_PMD_HELP_ONLY ;
            goto done ;
         }
         rc = SDB_INVALIDARG ;
         cerr << "must specify adduser/removeuser" << endl ;
         goto error ;
      }

      if ( _cmdHas( OPTIONS_ADDUSER ) )
      {
         _mode = OpAddUser ;
         _user = _cmdVm[OPTIONS_ADDUSER].as<string>() ;
      }
      else if ( _cmdHas( OPTIONS_RMUSER ) )
      {
         _mode = OpRemoveUser ;
         _user = _cmdVm[OPTIONS_RMUSER].as<string>() ;
      }

      if ( OpAddUser == _mode )
      {
         if ( _cmdHas( OPTIONS_PASSWD ) )
         {
            string passwd = _cmdVm[OPTIONS_PASSWD].as<string>() ;
            if ( "" ==  passwd )
            {
               _password = utilPasswordTool::interactivePasswdInput() ;
            }
            else
            {
               _password = passwd ;
            }
         }
      }

      if ( _cmdHas(OPTIONS_TOKEN ) )
      {
         _token = _cmdVm[OPTIONS_TOKEN].as<string>() ;
      }

      if ( _cmdHas(OPTIONS_FILE ) )
      {
         _file = _cmdVm[OPTIONS_FILE].as<string>() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN passwdOptions::_cmdHas(const CHAR * option) const
   {
      SDB_ASSERT(option, "") ;
      return _cmdParsed && _cmdVm.count(option) > 0 ;
   }

}
