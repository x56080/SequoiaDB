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

   Source File Name = sdbOidCatalog.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
// Note: the functions in this file is copied from impCatalogAgent.hpp and impCataInfo.hpp

#ifndef SDB_OID_CATALOG_HPP_
#define SDB_OID_CATALOG_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"
#include <string>
#include <vector>

using namespace std ;
namespace engine
{
   class _clsCatalogAgent;
   class _clsCatalogSet;
}

/**
 * CataInfo
 */
class CataInfo: public SDBObject
{
public:
   CataInfo();
   ~CataInfo();
   INT32 getAllGroupID( vector<UINT32>& list ) ;
   INT32 getGroupByRecord(const char* bsonData, UINT32& groupId);

private:
   string                     _collectionName;
   engine::_clsCatalogSet*    _cataSet;

friend class CatalogAgent;
};

/**
 * CatalogAgent
 */
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

#endif
