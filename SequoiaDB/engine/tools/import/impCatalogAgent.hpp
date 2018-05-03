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

   Source File Name = impCatalogAgent.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CATALOG_AGENT_HPP_
#define IMP_CATALOG_AGENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impCataInfo.hpp"
#include <string>

using namespace std;

namespace engine
{
   class _clsCatalogAgent;
}

namespace import
{
   // define CatalogAgent to avoid bson namespace conflict between c and C++ bson API
   class CatalogAgent: public SDBObject
   {
   public:
      CatalogAgent();
      ~CatalogAgent();
      INT32 updateCatalog(const char* bsonData);
      INT32 getCataInfo(const string& collectionName, CataInfo& cataInfo);

   private:
      engine::_clsCatalogAgent*     _cataAgent;
   };
}

#endif /* IMP_CATALOG_AGENT_HPP_ */
