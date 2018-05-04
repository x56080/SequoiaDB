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

   Source File Name = impHosts.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_HOSTS_HPP_
#define IMP_HOSTS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

using namespace std;

namespace import
{
   struct Host
   {
      string hostname;
      string svcname;
      INT32  refCount;

      Host()
      {
         refCount = 0;
      }
   };

   class Hosts: public SDBObject
   {
   public:
      static INT32 parse(const string& hostList, vector<Host>& hosts);
      static void  removeDuplicate(vector<Host>& hosts);
   };
}

#endif /* IMP_HOSTS_HPP_ */
