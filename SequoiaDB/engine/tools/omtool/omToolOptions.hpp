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

   Source File Name = omToolOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_OPTIONS_HPP_
#define OMTOOL_OPTIONS_HPP_

#include "utilOptions.hpp"

namespace omTool
{
   class omToolOptions: public engine::utilOptions
   {
   public:
      omToolOptions() ;
      ~omToolOptions() ;

      BOOLEAN hasHelp();

      BOOLEAN hasVersion();

      INT32 parse( INT32 argc, CHAR* argv[] ) ;

      /* General */
      inline const string& mode() const { return _mode ; }

      /* HostFiles */
      inline const string& hostname() const { return _hostname ; }
      inline const string& ip() const { return _ip ; }

      /* Directories */
      inline const string& path() const { return _path ; }
      inline const string& user() const { return _user ; }

   private:
      INT32 _checkOptions();

   private:
      string _mode ;

      /* HostFiles */
      string _hostname ;
      string _ip ;

      /* Directories */
      string _path ;
      string _user ;
   } ;
}

#endif /* OMTOOL_OPTIONS_HPP_ */