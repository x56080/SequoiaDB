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

   Source File Name = impCoord.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_COORD_HPP_
#define IMP_COORD_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impHosts.hpp"
#include "../client/client.h"
#include <string>
#include <vector>

using namespace std;

namespace import
{
   class Coords: public SDBObject
   {
   public:
      Coords();
      ~Coords();
      INT32 init(const vector<Host>& hosts,
                 const string& user,
                 const string& password,
                 BOOLEAN useSSL);
      INT32 getRandomCoord(string& hostname, string& svcname);
      static INT32 getRandomCoord(vector<Host>& hosts, UINT32& refCount,
                                  string& hostname, string& svcname);

   private:
      INT32 _connect(const string& hostname,
                     const string& svcname,
                     sdbConnectionHandle& conn);
      void  _disconnect(sdbConnectionHandle& conn);
      INT32 _checkCoord(const string& hostname, const string& svcname);

   private:
      string   _user;
      string   _password;
      BOOLEAN  _useSSL;
      BOOLEAN  _inited;
      UINT32   _refCount;

      vector<Host> _coords;
   };
}

#endif /* IMP_COORD_HPP_ */
