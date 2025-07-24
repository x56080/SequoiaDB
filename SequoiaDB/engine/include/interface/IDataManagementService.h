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
         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 utilCSUniqueID uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct ) = 0;

         virtual INT32 dropCS( IExecutor *executor,
                               const CHAR *name,
                               const dmsRemoveCSOptions &options ) = 0;

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *oldName,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite ) = 0;

         virtual INT32 openCL( IExecutor *executor,
                               const CHAR *fullName,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) = 0;

         virtual INT32 openCL( IExecutor *executor,
                               utilCLUniqueID uniqueId,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) = 0;

         virtual INT32 createCL( IExecutor *executor,
                                 const CHAR *clFullName,
                                 utilCLUniqueID clUniqueID,
                                 const dmsCreateCLOptions &o,
                                 const bson::BSONObj &adjunct ) = 0;

         virtual INT32 dropCL( IExecutor *executor,
                               const CHAR *clFullName,
                               const dmsRemoveCLOptions &o ) = 0;

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) = 0;

         virtual UINT32 getNullCSUniqueIDCnt() const = 0;
   };
} // namespace engine
#endif