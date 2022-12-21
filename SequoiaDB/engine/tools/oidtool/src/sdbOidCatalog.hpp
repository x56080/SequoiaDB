/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
