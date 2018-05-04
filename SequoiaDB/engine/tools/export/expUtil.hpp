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

   Source File Name = expUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_UTIL_HPP_
#define EXP_UTIL_HPP_

#include "oss.hpp"
#include <string>
#include <vector>

namespace exprt
{
   using namespace std ;
   UINT32 RC2ShellRC(INT32 rc) ;
   void cutStr( const string &str, vector<string>& subs, const string &cutBy ) ;
   void trimBoth ( string &str ) ;
   void trimLeft ( string &str ) ;
   void trimRight( string &str ) ;
}

#endif