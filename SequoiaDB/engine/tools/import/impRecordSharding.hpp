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

   Source File Name = impRecordSharding.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_SHARDING_HPP_
#define IMP_RECORD_SHARDING_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCataInfo.hpp"
#include "impCatalogAgent.hpp"
#include "impHosts.hpp"
#include "../client/bson/bson.h"
#include <string>
#include <map>

using namespace std;

namespace import
{
   class RecordSharding: public SDBObject
   {
   public:
      RecordSharding();
      ~RecordSharding();
      INT32 init(const vector<Host>& hosts,
                 const string& user,
                 const string& password,
                 const string& csname,
                 const string& clname,
                 BOOLEAN useSSL);
      INT32 getGroupNum() const { return _groupNum; }
      INT32 getGroupByRecord(bson* record, string& collection, UINT32& groupId);

   private:
      const vector<Host>*     _hosts;
      string                  _user;
      string                  _password;
      string                  _csname;
      string                  _clname;
      BOOLEAN                 _useSSL;
      BOOLEAN                 _inited;

      CatalogAgent            _cataAgent;
      CataInfo                _cataInfo;
      string                  _collectionName;
      BOOLEAN                 _isMainCL;
      INT32                   _groupNum;
      map<string, CataInfo>   _subCataInfo;
   };
}

#endif /* IMP_RECORD_SHARDING_HPP_ */
