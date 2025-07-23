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

   Source File Name = passwdOptions.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef   PASSWD_OPTOPNS_H_
#define   PASSWD_OPTOPNS_H_

#include "oss.hpp"
#include "ossTypes.h"
#include <iostream>
#include <string>
#include <boost/program_options.hpp>

using namespace std ;

namespace passwd
{
   namespace po = boost::program_options ;

   class passwdOptions : public SDBObject
   {
      public:
         enum OpMode { OpAddUser, OpRemoveUser } ;

         passwdOptions() ;
         ~passwdOptions() ;

         INT32 parseCmd(INT32 argc, CHAR* argv[]) ;
         INT32 printHelpInfo() const ;

         BOOLEAN  hasHelp() const ;
         BOOLEAN  hasPassword() const ;

         inline const string &user()        const { return _user ; }
         inline const string &token()       const { return _token ; }
         inline const string &file()        const { return _file ; }
         inline const string &password()    const { return _password ; }
         inline const OpMode &mode()        const { return _mode ; }

      private:
         BOOLEAN _cmdHas(const CHAR* option) const;

      private:
         BOOLEAN                   _cmdParsed ;
         po::options_description   _cmdDesc ;
         po::variables_map         _cmdVm ;
         string                    _user ;
         string                    _token ;
         OpMode                    _mode ;
         string                    _password ;
         string                    _file ;
   } ;

}
#endif
