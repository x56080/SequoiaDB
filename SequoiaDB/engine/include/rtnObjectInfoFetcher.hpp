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

   Source File Name = rtnObjectInfoFetcher.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
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
