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

   Source File Name = sdbOmTool.cpp

   Descriptive Name = sdbOmTool Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbstop,
   which is used to stop SequoiaDB engine.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/27/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "omToolCmdBase.hpp"
#include "utilCommon.hpp"

namespace omTool
{
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      omToolOptions options ;
      omToolCmdBase *cmd = NULL ;

      rc = options.parse( argc, argv ) ;
      if( rc )
      {
         goto error ;
      }

      if( options.hasHelp() )
      {
         options.print() ;
         goto done ;
      }

      if( options.hasVersion() )
      {
         ossPrintVersion( "sdbomtool" ) ;
         goto done ;
      }

      cmd = getOmToolCmdBuilder()->create( options.mode().c_str() ) ;
      if( NULL == cmd )
      {
         rc = SDB_INVALIDARG ;
         cout << "unreconigzed mode: " << options.mode() << endl ;
         goto error ;
      }

      cmd->setOptions( &options ) ;

      rc = cmd->doCommand() ;
      if( rc )
      {
         goto error ;
      }

   done:
      if( cmd )
      {
         getOmToolCmdBuilder()->release( cmd ) ;
         cmd = NULL ;
      }
      return SDB_OK == rc ? 0 : engine::utilRC2ShellRC( rc ) ;
   error:
      goto done ;
   }
}

INT32 main( INT32 argc, CHAR **argv )
{
   return omTool::mainEntry( argc, argv ) ;
}



