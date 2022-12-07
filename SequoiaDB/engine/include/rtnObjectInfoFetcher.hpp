#ifndef RTN_OBJECT_INFO_FETCHER_HPP__
#define RTN_OBJECT_INFO_FETCHER_HPP__

#include "interface/IObjectInfo.h"
#include "interface/IDataManagementService.h"
#include "rtnCB.hpp"

namespace engine
{
   class _rtnObjectInfoFetcher
   {
   public:
      _rtnObjectInfoFetcher( IDataManagementService *dmsCB, SDB_RTNCB *rtnCB )
      : _dmsCB( dmsCB ), _rtnCB( rtnCB )
      {
      }

   public:
      INT32 getCollectionMetaInfo( IExecutor *executor,
                                   const CHAR *clFullName,
                                   std::shared_ptr< const ICollectionMetaInfo > &meta );

      INT32 getCollectionStatInfo( IExecutor *executor,
                                   const CHAR *clFullName,
                                   std::shared_ptr< const ICollectionStatInfo > &stat );

   private:
      IDataManagementService *_dmsCB;
      SDB_RTNCB *_rtnCB;
   };
   using rtnObjectInfoFetcher = _rtnObjectInfoFetcher;
} // namespace engine

#endif
