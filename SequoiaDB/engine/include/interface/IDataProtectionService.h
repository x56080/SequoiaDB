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

   Source File Name = IDataProtectionService.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_DATA_PROTECTION_SERVICE_H_
#define SDB_I_DATA_PROTECTION_SERVICE_H_

#include "interface/IDataJournal.h"
#include "sdbIPersistence.hpp"
#include "dpsRequestContext.hpp"
#include "dpsOperationList.hpp"

namespace engine
{
   class _IDataProtectionService : public IDataJournal,
                                   public IDataSyncBase
   {
      public:
         _IDataProtectionService() = default;
         virtual ~_IDataProtectionService() = default;
         _IDataProtectionService(const _IDataProtectionService &) = delete;
         _IDataProtectionService &operator=(const _IDataProtectionService &) = delete;
      
      public:
         virtual void regEventHandler(dpsEventHandler *handler) = 0;
         virtual void unregEventHandler(dpsEventHandler *handler) = 0;
         virtual INT32 completeOpr(IExecutor *executor, INT32 w) = 0;
         virtual INT32 archive() = 0;
         virtual INT32 process(IExecutor *executor, dpsRequestContext &ctx) = 0 ;
         virtual INT32 loadOpl( const DPS_LSN &lastNodeLSN,
                                dpsOperationList &opl ) = 0 ;

   };//class _IDataProtectionService
   using IDataProtectionService = _IDataProtectionService;

} // namespace engine


#endif//SDB_I_DATA_PROTECTION_SERVICE_H_