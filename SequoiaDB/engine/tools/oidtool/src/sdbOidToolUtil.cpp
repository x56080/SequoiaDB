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

   Source File Name = sdbOidToolUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#include "sdbOidToolUtil.hpp"

INT32 InfoOptions::parse( INT32 argc, CHAR* argv[] )
{
   INT32 rc = SDB_OK ;
   po::variables_map vm ;
   po::options_description desc( "Command options" ) ;

   COMMANDS_ADD_PARAM_OPTIONS_BEGIN ( desc )
         TOOL_GENERAL_OPTIONS
   COMMANDS_ADD_PARAM_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, desc, vm, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( vm.empty() || vm.count( TOOL_OPTIONS_HELP ) )
   {
      // print help information
      cout << desc << endl ;
      cout << endl ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if ( vm.count( TOOL_OPTIONS_VERSION ) )
   {
      ossPrintVersion( "sdboidtool" ) ;
      rc = SDB_SDB_VERSION_ONLY ;
      goto done ;
   }

   if ( !vm.count( TOOL_OPTIONS_ACTION ) )
   {
      rc = SDB_INVALIDARG ;
      cerr << "sdboidtool should specify --action" << endl ;
      goto error ;
   }

   if ( !vm.count( TOOL_OPTIONS_SRC_CLNAME ) )
   {
      rc = SDB_INVALIDARG ;
      cerr << "sdboidtool should specify --srcclfullname" << endl ;
      goto error ;
   }

   // hostname
   if ( vm.count( TOOL_OPTIONS_HOSTNAME ) )
   {
      this->hostName = vm[TOOL_OPTIONS_HOSTNAME].as<string>().c_str() ;
   }
   // svcname
   if ( vm.count( TOOL_OPTIONS_SVCNAME ) )
   {
      this->svcName = vm[TOOL_OPTIONS_SVCNAME].as<string>().c_str() ;
   }
   // user
   if ( vm.count( TOOL_OPTIONS_USER ) )
   {
      this->user = vm[TOOL_OPTIONS_USER].as<string>().c_str() ;
   }
   // password
   if ( vm.count( TOOL_OPTIONS_PASSWORD ) )
   {
      this->passwd = vm[TOOL_OPTIONS_PASSWORD].as<string>().c_str() ;
   }
   // logpath
   if ( vm.count( TOOL_OPTIONS_LOGPATH ) )
   {
      this->logpath = vm[TOOL_OPTIONS_LOGPATH].as<string>().c_str() ;
   }
   // action
   if ( vm.count( TOOL_OPTIONS_ACTION ) )
   {
      this->action = vm[TOOL_OPTIONS_ACTION].as<string>().c_str() ;
      if ( this->action != "check" && this->action != "repair" )
      {
         rc = SDB_INVALIDARG ;
         cerr << "sdboidtool --action should be 'check' or 'repair'" << endl ;
         goto error ;
      }
      if ( this->action == "repair" && !vm.count( TOOL_OPTIONS_DEST_CLNAME ) )
      {
         rc = SDB_INVALIDARG ;
         cerr << "sdboidtool should specify --destclfullname for repair" << endl ;
         goto error ;
      }
   }
   // checkall
   if ( vm.count( TOOL_OPTIONS_CHECKALL ) )
   {
      this->checkAll = vm[TOOL_OPTIONS_CHECKALL].as<bool>() ;
   }
   // srcclfullname
   if ( vm.count( TOOL_OPTIONS_SRC_CLNAME ) )
   {
      this->srcCLFullName = vm[TOOL_OPTIONS_SRC_CLNAME].as<string>().c_str() ;
   }
   // destclfullname
   if ( vm.count( TOOL_OPTIONS_DEST_CLNAME ) )
   {
      this->destCLFullName = vm[TOOL_OPTIONS_DEST_CLNAME].as<string>().c_str() ;
   }
   // tasknum
   if ( vm.count( TOOL_OPTIONS_TASKNUM ) )
   {
      this->taskNum = vm[TOOL_OPTIONS_TASKNUM].as<int>() ;
   }
   // batchnum
   if ( vm.count( TOOL_OPTIONS_LIMIT ) )
   {
      this->limit = vm[TOOL_OPTIONS_LIMIT].as<long>() ;
   }
   // debug
   if ( vm.count( TOOL_OPTIONS_DEBUG ) )
   {
      this->isDebug = vm[TOOL_OPTIONS_DEBUG].as<bool>() ;
   }
   // checking
   if ( this->taskNum < 1 )
   {
      this->taskNum = 10 ;
   }
   else if ( this->taskNum > 100 )
   {
      this->taskNum = 100 ;
   }
   if ( this->logpath.empty() )
   {
      this->logpath = TOOL_LOG_PATH ;
   }
   if ( this->limit <= 0 )
   {
      this->limit = 10000 ;
   }

done:
   return rc ;
error:
   goto done ;
}

InfoOptions* getInfoOptions()
{
   static InfoOptions _infoOptions ;
   return &_infoOptions ;
}
