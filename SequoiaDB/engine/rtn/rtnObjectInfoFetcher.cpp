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
