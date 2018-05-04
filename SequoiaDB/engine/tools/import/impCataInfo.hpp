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

   Source File Name = impCataInfo.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CATA_INFO_HPP_
#define IMP_CATA_INFO_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

using namespace std;

namespace engine
{
   class _clsCatalogSet;
}

namespace import
{
   // use CataInfo to avoid bson namespace conflict between c and C++ bson API
   class CataInfo: public SDBObject
   {
   public:
      CataInfo();
      ~CataInfo();
      INT32 getGroupNum();
      BOOLEAN isMainCL();
      INT32 getSubCLList(vector<string>& list);
      INT32 getGroupByRecord(const char* bsonData, UINT32& groupId);
      INT32 getSubCLNameByRecord(const char* bsonData, string &subCLName);

   private:
      string                     _collectionName;
      engine::_clsCatalogSet*    _cataSet;

   friend class CatalogAgent;
   };
}

#endif /* IMP_CATA_INFO_HPP_ */
