/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = clsStorageResourceAgent.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_STORAGE_RESOURCE_AGENT_HPP__
#define CLS_STORAGE_RESOURCE_AGENT_HPP__
#include "interface/IDataManagementService.h"
#include "interface/IDataCollection.h"
#include "clsCLMetaCache.hpp"
#include "ossMemPool.hpp"
#include "sdbInterface.hpp"
#include "utilUniqueID.hpp"
#include <memory>

namespace engine
{
class _clsStorageResourceAgent : public SDBObject
{
public:
   virtual ~_clsStorageResourceAgent()
   {
   }

public:
   virtual INT32 getIndexes( IExecutor *executor,
                             const CHAR *clFullName,
                             clsIndexInfoSetPtr &indexSetPtr ) = 0;

   virtual INT32 getIndexes( IExecutor *executor,
                             utilCLUniqueID cluid,
                             clsIndexInfoSetPtr &indexSetPtr ) = 0;

   virtual INT32 getCLMetaCache( IExecutor *executor, 
                                 const CHAR *clFullName,
                                 clsCLMetaCachePtr &clCachePtr) = 0;

   virtual INT32 getCLMetaCache( IExecutor *executor,
                                 utilCLUniqueID cluid,
                                 clsCLMetaCachePtr &clCachePtr ) = 0;
};
using clsStorageResourceAgent = _clsStorageResourceAgent;

extern std::unique_ptr< clsStorageResourceAgent >
newClsStorageResourceAgentImpl( IDataManagementService *dms );
} // namespace engine

#endif