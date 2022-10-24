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
#include "sdbInterface.hpp"
#include "clsCLMetaCache.hpp"
#include "rtnContext.hpp"
#include "utilUniqueID.hpp"
#include <memory>

namespace engine
{
   class _clsStorageResourceAgent : public SDBObject
   {
   public:
      virtual ~_clsStorageResourceAgent() {}

   public:
      virtual INT32 getIndexInfo( IExecutor *executor,
                                  const CHAR *clFullName,
                                  const CHAR *indexName,
                                  BOOLEAN withStat,
                                  CLS_INDEX_INFO_PTR &infoPtr ) = 0;

      virtual INT32 getIndexInfo( IExecutor *executor,
                                  utilCLUniqueID cluid,
                                  utilIdxInnerID indexInnerID,
                                  BOOLEAN withStat,
                                  CLS_INDEX_INFO_PTR &infoPtr ) = 0;

      virtual INT32 getIndexInfoSet( IExecutor *executor,
                                     const CHAR *clFullName,
                                     BOOLEAN withStat,
                                     CLS_INDEX_INFO_SET_PTR &indexSetPtr ) = 0;

      virtual INT32 getIndexInfoSet( IExecutor *executor,
                                     utilCLUniqueID cluid,
                                     BOOLEAN withStat,
                                     CLS_INDEX_INFO_SET_PTR &indexSetPtr ) = 0;

      virtual INT32 getCLMetaCache( IExecutor *executor,
                                    const CHAR *clFullName,
                                    BOOLEAN withCLStat,
                                    BOOLEAN withIndexInfoSet,
                                    BOOLEAN withIndexStat,
                                    clsCLMetaCachePtr &clCachePtr ) = 0;

      virtual INT32 getCLMetaCache( IExecutor *executor,
                                    utilCLUniqueID cluid,
                                    BOOLEAN withCLStat,
                                    BOOLEAN withIndexInfoSet,
                                    BOOLEAN withIndexStat,
                                    clsCLMetaCachePtr &clCachePtr ) = 0;

      virtual INT32 getCLStat( IExecutor *executor,
                               const CHAR *clFullName,
                               CLS_CL_STAT_PTR &clStatPtr ) = 0;

      virtual INT32 getCLStat( IExecutor *executor,
                               utilCLUniqueID cluid,
                               CLS_CL_STAT_PTR &clStatPtr ) = 0;
   };
   using clsStorageResourceAgent = _clsStorageResourceAgent;

   extern std::unique_ptr< clsStorageResourceAgent > newClsStorageResourceAgentImpl(
      IDataManagementService *dms );
   extern std::unique_ptr< clsStorageResourceAgent > newClsStorageResourceAgentImpl(
      IDataManagementService *dms,
      const std::function< INT32( const CHAR *,
                                  const BSONObj &,
                                  const BSONObj &,
                                  const BSONObj &,
                                  const BSONObj &,
                                  SINT32,
                                  SINT64,
                                  SINT64,
                                  IDataManagementService *,
                                  SINT64 &,
                                  rtnContextPtr *,
                                  BOOLEAN ) > &queryFunc,
      const std::function< INT32( SINT64, SINT32, rtnContextBuf & ) > &getMoreFunc,
      const std::function< INT32( INT32, const INT64 * ) > &killContextFunc );
} // namespace engine

#endif