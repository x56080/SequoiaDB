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

   Source File Name = IDataProtectionService.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_I_DATA_PROTECTION_SERVICE_H_
#define SDB_I_DATA_PROTECTION_SERVICE_H_

#include "interface/IDataJournal.h"
#include "sdbIPersistence.hpp"

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

   };//class _IDataProtectionService
   using IDataProtectionService = _IDataProtectionService;

} // namespace engine


#endif//SDB_I_DATA_PROTECTION_SERVICE_H_