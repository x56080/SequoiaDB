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

   Source File Name = options.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <string>
#include <iostream>
using namespace std ;

class cmdOptions
{
public:
   cmdOptions() ;
   ~cmdOptions() ;

   int parse( int argc, char *argv[] ) ;

   int getLevel(){ return _level ; }
   bool getForce(){ return _force ; }
   string &getLang(){ return _lang ; }

private:
   /*
      0: SERVER
      1: ERROR
      2: WARNING
      3: INFO
   */
   int      _level ;
   bool     _force ;
   string   _lang ;
} ;

cmdOptions *getCmdOptions() ;


#endif