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

   Source File Name = sdbOidToolUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_OID_TOOL_UTIL_HPP_
#define SDB_OID_TOOL_UTIL_HPP_

#include "ossVer.h"
#include "core.hpp"
#include "utilParam.hpp"
#include <string>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std ;
using namespace engine ;
namespace pt = boost::posix_time;

#define TOOL_LOG_PATH "sdboidtool.log"

#define TOOL_OPTIONS_HELP         ( "help" )
#define TOOL_OPTIONS_VERSION      ( "version" )
#define TOOL_OPTIONS_HOSTNAME     ( "hostname" )
#define TOOL_OPTIONS_SVCNAME      ( "svcname" )
#define TOOL_OPTIONS_USER         ( "user" )
#define TOOL_OPTIONS_PASSWORD     ( "password" )
#define TOOL_OPTIONS_LOGPATH      ( "logpath" )
#define TOOL_OPTIONS_ACTION       ( "action" )
#define TOOL_OPTIONS_CHECKALL     ( "checkall" )
#define TOOL_OPTIONS_SRC_CLNAME   ( "srcclfullname" )
#define TOOL_OPTIONS_DEST_CLNAME  ( "destclfullname" )
#define TOOL_OPTIONS_TASKNUM      ( "tasknum" )
#define TOOL_OPTIONS_LIMIT        ( "limit" )
#define TOOL_OPTIONS_DEBUG        ( "debug" )

#define COMMANDS_ADD_PARAM_OPTIONS_BEGIN(des)  des.add_options()
#define COMMANDS_ADD_PARAM_OPTIONS_END         ;
#define COMMANDS_STRING(a, b)                  (string(a) + string(b)).c_str()

#define TOOL_GENERAL_OPTIONS \
   (COMMANDS_STRING(TOOL_OPTIONS_HELP,        ",h"), "help") \
   (COMMANDS_STRING(TOOL_OPTIONS_VERSION,     ",v"), "version") \
   (COMMANDS_STRING(TOOL_OPTIONS_HOSTNAME,    ",s"), po::value<string>(), "specify coord hostname" ) \
   (COMMANDS_STRING(TOOL_OPTIONS_SVCNAME,     ",p"), po::value<string>(), "specify coord svcname") \
   (COMMANDS_STRING(TOOL_OPTIONS_USER,        ",u"), po::value<string>(), "specify user" ) \
   (COMMANDS_STRING(TOOL_OPTIONS_PASSWORD,    ",w"), po::value<string>(), "specify password") \
   (COMMANDS_STRING(TOOL_OPTIONS_LOGPATH,     ",a"), po::value<string>(), "specify the log file path") \
   (COMMANDS_STRING(TOOL_OPTIONS_ACTION,      ",b"), po::value<string>(), "specify action: check or repair") \
   (COMMANDS_STRING(TOOL_OPTIONS_CHECKALL,    ",c"), po::value<bool>(), "check all the groups or not: true or false, default is false(for check only)" ) \
   (COMMANDS_STRING(TOOL_OPTIONS_SRC_CLNAME,  ",d"), po::value<string>(), "specify src cl full name(for both check and repair)" ) \
   (COMMANDS_STRING(TOOL_OPTIONS_DEST_CLNAME, ",e"), po::value<string>(), "specify dest cl full name(for repair only)") \
   (COMMANDS_STRING(TOOL_OPTIONS_TASKNUM,     ",f"), po::value<int>(), "importer num: range[1, 100], default is 10(for repair only)") \
   (COMMANDS_STRING(TOOL_OPTIONS_LIMIT,       ",g"), po::value<long>(), "query limit num, default is 10000(for repair only)") \
   (COMMANDS_STRING(TOOL_OPTIONS_DEBUG,       ",i"), po::value<bool>(), "set log level to debug or not: true or false, default: false")

struct InfoOptions
{
   // FIXME: change to immutable
   // input options
   string            hostName ;
   string            svcName ;
   string            user ;
   string            passwd ;
   string            logpath ;
   string            action ;
   BOOLEAN           checkAll ;
   string            srcCLFullName ;
   string            destCLFullName ;
   INT32             taskNum ;
   INT64             limit ;
   BOOLEAN           isDebug ;
   // local options
   INT32             displayPeriod ;

   InfoOptions()
   {
      checkAll = false ;
      isDebug = false ;
      taskNum = 10 ;
      limit = 10000 ;
      displayPeriod = 60 ; // 60 seconds
   }
   ~InfoOptions() {}

   INT32 parse( INT32 argc, CHAR* argv[] ) ;
} ;

InfoOptions* getInfoOptions() ;

#endif
