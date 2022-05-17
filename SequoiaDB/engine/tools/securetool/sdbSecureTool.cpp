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

   Source File Name = sdbPasswd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#include <iostream>
#include "ossVer.hpp"
#include "utilParam.hpp"
#include "pdSecure.hpp"
#include "utilCommon.hpp"

using namespace std ;

#define SECURE_OPTIONS_HELP         ( "help" )
#define SECURE_OPTIONS_VERSION      ( "version" )
#define SECURE_OPTIONS_DECRYPT      ( "decrypt" )

#define COMMANDS_ADD_PARAM_OPTIONS_BEGIN(des)  des.add_options()
#define COMMANDS_ADD_PARAM_OPTIONS_END         ;
#define COMMANDS_STRING(a, b)                  (string(a) + string(b)).c_str()

#define SECURE_GENERAL_OPTIONS \
   (COMMANDS_STRING(SECURE_OPTIONS_HELP,    ",h"), "help") \
   (COMMANDS_STRING(SECURE_OPTIONS_VERSION, ",v"), "version") \
   (COMMANDS_STRING(SECURE_OPTIONS_DECRYPT, ",d"), po::value<string>(), "decrypt data")

void displayArg ( po::options_description &desc )
{
   cout << desc << endl ;
   cout << "Example:" << endl ;
   cout << "  ./bin/sdbsecuretool -d \"SDBSECURE0000(hfIcCqboSWK2)\"" << endl ;
   cout << endl ;
}

INT32 resolveArgument( INT32 argc, CHAR* argv[],
                       po::options_description &desc,
                       po::variables_map &vm,
                       BOOLEAN &hasDecrypt,
                       ossPoolString &decryptData )
{
   INT32 rc = SDB_OK ;
   hasDecrypt = FALSE ;
   decryptData.clear() ;

   COMMANDS_ADD_PARAM_OPTIONS_BEGIN ( desc )
        SECURE_GENERAL_OPTIONS
   COMMANDS_ADD_PARAM_OPTIONS_END

   rc = engine::utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( vm.empty() || vm.count( SECURE_OPTIONS_HELP ) )
   {
      displayArg( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count( SECURE_OPTIONS_VERSION ) )
   {
      ossPrintVersion( "sdbsecuretool" ) ;
      rc = SDB_SDB_VERSION_ONLY ;
      goto done ;
   }

   if ( vm.count( SECURE_OPTIONS_DECRYPT ) )
   {
      hasDecrypt = TRUE ;
      decryptData = vm[SECURE_OPTIONS_DECRYPT].as<string>().c_str() ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   BOOLEAN hasDecrypt = FALSE ;
   ossPoolString decryptData ;
   po::variables_map vm ;
   po::options_description desc( "Command options" ) ;

   rc = resolveArgument( argc, argv, desc, vm, hasDecrypt, decryptData ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( hasDecrypt )
   {
      ossPoolString plainText ;
      rc = utilSecureDecrypt( decryptData, plainText ) ;
      if ( rc )
      {
         if ( SDB_INVALIDARG == rc )
         {
            cerr << "Invalid data format: " << decryptData << endl ;
         }
         else
         {
            cerr << "Failed to decrypt data, rc: " << rc << endl ;
         }
      }
      else
      {
         cout << plainText.c_str() << endl ;
      }
   }

done:
   return engine::utilRC2ShellRC( rc ) ;
error:
   goto done ;
}
