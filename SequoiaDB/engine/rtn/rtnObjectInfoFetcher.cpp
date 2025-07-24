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

   Source File Name = rtnObjectInfoFetcher.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnObjectInfoFetcher.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"

namespace engine
{
   INT32 _rtnObjectInfoFetcher::getCollectionMetaInfo(
      IExecutor *executor,
      const CHAR *clFullName,
      std::shared_ptr< const ICollectionMetaInfo > &meta )
   {
      INT32 rc = SDB_OK;
      IDataManagementService *dms = sdbGetDMSCB();
      dmsOpenCLOptions options;
      DATA_COLLECTION_PTR cl = nullptr;
      rc = dms->openCL( executor, clFullName, options, cl );
      PD_RC_CHECK( rc, PDERROR, "failed to open collection[%s], rc: %d", clFullName, rc );
      rc = cl->getMetaData( executor, meta );
      PD_RC_CHECK( rc, PDERROR, "failed to get collection[%s] meta info, rc: %d", clFullName, rc );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnObjectInfoFetcher::getCollectionStatInfo(
      IExecutor *executor,
      const CHAR *clFullName,
      std::shared_ptr< const ICollectionStatInfo > &stat )
   {
      INT32 rc = SDB_OK;
      // system collections have no statistics, return nullptr and optimizer will handle it.
      if ( dmsIsSysCSName( clFullName ) )
      {
         stat = nullptr;
      }
      else
      {
         rtnObjectStatCache *statCache = sdbGetRTNCB()->getObjectStatCache();
         CONST_RTN_CL_STAT_PTR clStatPtr = nullptr;
         rc = statCache->getOrUpdateCLStat( executor, clFullName, clStatPtr );
         PD_RC_CHECK( rc, PDERROR, "failed to get collection[%s] statistics, rc: %d", clFullName,
                      rc );
         stat = clStatPtr;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine
