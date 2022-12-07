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

   Source File Name = rtnObjectStatAgent.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft
          11/24/2022  ZHY Move to rtn module

   Last Changed =

*******************************************************************************/

#ifndef RTN_OBJECT_STAT_AGENT_HPP__
#define RTN_OBJECT_STAT_AGENT_HPP__
#include "interface/IDataManagementService.h"
#include "sdbInterface.hpp"
#include "rtnObjectStatInfo.hpp"
#include "rtnContext.hpp"
#include "utilUniqueID.hpp"
#include <memory>

namespace engine
{
   class _rtnObjectStatAgent : public SDBObject
   {
   public:
      class contextBase : public SDBObject
      {
         public:
            virtual ~contextBase() = default;

            virtual INT32 fetchOne( IExecutor *executor, RTN_CL_STAT_PTR & ) = 0;

            virtual INT32 fetchBatch( IExecutor *executor,
                                      UINT32 batchSize,
                                      ossPoolVector< RTN_CL_STAT_PTR > & ) = 0;
      };

   public:
      virtual ~_rtnObjectStatAgent() = default;

   public:
      virtual INT32 getCollectionStatInfo( IExecutor *executor,
                                           const CHAR *clFullName,
                                           BOOLEAN withIndexStatInfo,
                                           RTN_CL_STAT_PTR &clStatPtr ) = 0;

      // these functions will open a cursor, then use it to fetch one
      virtual INT32 getCollectionStatInfoOnCS( IExecutor *executor,
                                               const CHAR *csName,
                                               std::unique_ptr< contextBase > &pCtx ) = 0;
      virtual INT32 getAllCollectionStatInfo( IExecutor *executor,
                                              std::unique_ptr< contextBase > &pCtx ) = 0;
   };
   using rtnObjectStatAgent = _rtnObjectStatAgent;

   extern std::unique_ptr< rtnObjectStatAgent > newRtnObjectStatAgentImpl(
      IDataManagementService *dms );
   extern std::unique_ptr< rtnObjectStatAgent > newRtnObjectStatAgentImpl(
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