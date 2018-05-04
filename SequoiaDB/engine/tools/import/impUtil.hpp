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

   Source File Name = impUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_UTIL_HPP_
#define IMP_UTIL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

using namespace std;

namespace import
{
   #define IMP_UTIL_TIMEZONE_MAX 720

   UINT32 RC2ShellRC(INT32 rc);
   INT32 parseFileList(const string& fileList, vector<string>& files);
   INT32 checkDateTimeFormat(const string& format) ;
}

#endif /* IMP_UTIL_HPP_ */
