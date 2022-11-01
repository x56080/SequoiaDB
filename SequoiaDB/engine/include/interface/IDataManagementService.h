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

   Source File Name = IDataManagementService.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/26/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_DATA_MANAGEMENT_SERVICE_HPP_
#define SDB_I_DATA_MANAGEMENT_SERVICE_HPP_
#include "interface/IDataStorageEngine.h"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "../bson/bson.hpp"
#include "utilUniqueID.hpp"
namespace engine
{
class IDataManagementService : public SDBObject
{
public:
   IDataManagementService(){};
   virtual ~IDataManagementService(){};
   IDataManagementService( const IDataManagementService & ) = delete;
   IDataManagementService &operator=( const IDataManagementService & ) = delete;

public:
   virtual INT32 openCL( IExecutor *executor,
                         const CHAR *fullName,
                         const dmsOpenCLOptions &o,
                         DATA_COLLECTION_PTR &ptr ) = 0;

   virtual INT32 openCL( IExecutor *executor,
                         utilCLUniqueID uniqueId,
                         const dmsOpenCLOptions &o,
                         DATA_COLLECTION_PTR &ptr ) = 0;
};
} // namespace engine
#endif