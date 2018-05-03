/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = fmpMain.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "fmpController.hpp"
#include <string>
#include "pmdOptions.h"
#include "pmdDef.hpp"
#include "utilParam.hpp"
#include "ossVer.h"

using namespace std ;

#define COMMANDS_OPTIONS \
    ( PMD_COMMANDS_STRING (PMD_OPTION_HELP, ",h"), "help" ) \
    ( PMD_OPTION_VERSION, "version" )

// initialize options
void init ( po::options_description &desc )
{
   PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
   PMD_ADD_PARAM_OPTIONS_END
}

void displayArg ( po::options_description &desc )
{
   std::cout << "Usage:  sdbfmp [OPTION]" <<std::endl;
   std::cout << desc << std::endl ;
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   fmpController controller ;
   po::options_description desc ( "Command options" ) ;
   po::variables_map vm ;

   init ( desc ) ;

   // validate arguments
   rc = engine::utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Invalid arguments, rc: %d", rc ) ;
      displayArg ( desc ) ;
      goto done ;
   }
   /// read cmd first
   if ( vm.count( PMD_OPTION_HELP ) )
   {
      displayArg( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   if ( vm.count( PMD_OPTION_VERSION ) )
   {
      ossPrintVersion( "Sdb fmp version" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   rc = controller.run() ;

done:
   return SDB_OK == rc ? 0 : 127 ;
}

