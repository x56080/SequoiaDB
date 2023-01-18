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

   Source File Name = IDataStorageEngine.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_DATA_STORAGE_ENGINE_HPP_
#define SDB_I_DATA_STORAGE_ENGINE_HPP_

#include "utilUniqueID.hpp"
#include "sdbInterface.hpp"
#include "../bson/bson.hpp"
#include "dmsSuDescriptor.hpp"
#include "dmsEngineOptions.hpp"
#include "utilPooledObject.hpp"
#include "utilInsertResult.hpp"
#include "ossMemPool.hpp"
#include "interface/IRecordFilter.h"
#include "interface/IRecordUpdater.h"
#include "interface/IDataCollection.h"
#include "interface/IDataCursor.h"

namespace engine
{
// 20 minutes
#define DMS_DFT_BLOCKWRITE_TIMEOUT ( 20 * 60 * OSS_ONE_SEC )

   class IDataStorageEngine : public SDBObject
   {
      public:
         IDataStorageEngine() {}
         virtual ~IDataStorageEngine() = default;
         IDataStorageEngine( const IDataStorageEngine &o ) = delete;
         IDataStorageEngine &operator=( const IDataStorageEngine & ) = delete;

      public:
         virtual INT32 close( IExecutor *executor, const dmsCloseDBOptions &options ) = 0;

         virtual DMS_ENGINE_TYPE getEngineType() const = 0;

         // virtual INT32 openCS( IExecutor *executor,
         //                       const CHAR *name,
         //                       const dmsOpenCSOptions &o,
         //                       COLLECTION_SPACE_PTR &ptr ) = 0;

         // virtual INT32 openCS( IExecutor *executor,
         //                       utilCSUniqueID csUID,
         //                       const dmsOpenCSOptions &o,
         //                       COLLECTION_SPACE_PTR &ptr ) = 0;

         // virtual INT32 verifyAndOpenCS( IExecutor *executor,
         //                                const dmsEventSUItem *pSUItem,
         //                                const dmsOpenCSOptions &o,
         //                                COLLECTION_SPACE_PTR &ptr ) = 0;

      public:
         // temp for vessel
         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct ) = 0;

         virtual INT32 removeCS( IExecutor *executor, const CHAR *csName ) = 0;

      public:
         virtual INT32 createCS( IExecutor *executor,
                                 const CHAR *name,
                                 const utilCSUniqueID &uniqueId,
                                 const dmsCreateCSOptions &o,
                                 const bson::BSONObj &adjunct,
                                 DMS_SU_DESCRIPTOR &desc ) = 0;

         virtual INT32 testCS( IExecutor *executor,
                               const CHAR *name,
                               utilCSUniqueID &uniqueId ) = 0;

         virtual INT32 testCS( IExecutor *executor, utilCSUniqueID uniqueId ) = 0;

         virtual INT32 listCS( IExecutor *executor, DATA_CURSOR_PTR &cursor ) = 0;

         virtual INT32 getCSCount( IExecutor *executor, UINT32 &count ) = 0;

         virtual INT32 removeCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &options ) = 0;

         virtual INT32 removeEmptyCS( IExecutor *executor,
                                      const CHAR *name,
                                      const dmsRemoveCSOptions &options ) = 0;

         virtual INT32 renameCS( IExecutor *executor,
                                 const CHAR *name,
                                 const CHAR *newName,
                                 BOOLEAN blockWrite,
                                 DMS_SU_DESCRIPTOR &desc ) = 0;

         virtual INT32 unloadCS( IExecutor *executor,
                                 const CHAR *name,
                                 const dmsRemoveCSOptions &delOptions ) = 0;

         virtual INT32 restoreCS( IExecutor *executor, const CHAR *name ) = 0;

         virtual INT32 returnCS( IExecutor *executor,
                                 dmsReturnOptions &options,
                                 BOOLEAN blockWrite ) = 0;

         virtual INT32 nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc ) = 0;

      public:
         virtual INT32 createCL( IExecutor *executor,
                                 const CHAR *fullName,
                                 utilCLUniqueID uniqueId,
                                 const dmsCreateCLOptions &o,
                                 const bson::BSONObj &adjunct ) = 0;

         virtual INT32 removeCL( IExecutor *executor,
                                 const CHAR *fullName,
                                 const dmsRemoveCLOptions &o ) = 0;

         virtual INT32 testCL( IExecutor *executor,
                               const CHAR *fullName,
                               utilCLUniqueID &uniqueId ) = 0;

         virtual INT32 testCL( IExecutor *executor, utilCLUniqueID uniqueId ) = 0;

         virtual INT32 listCL( IExecutor *executor,
                               const CHAR *csName,
                               DATA_CURSOR_PTR &cursor ) = 0;

         virtual INT32 openCL( IExecutor *executor,
                               const CHAR *fullName,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) = 0;

         virtual INT32 openCL( IExecutor *executor,
                               utilCLUniqueID uniqueId,
                               const dmsOpenCLOptions &o,
                               DATA_COLLECTION_PTR &ptr ) = 0;

         virtual INT32 getCLCount( IExecutor *executor, const CHAR *csName, UINT32 &count ) = 0;
   }; // class IDataStorageEngine
} // namespace engine

#endif // SDB_I_DATA_STORAGE_ENGINE_HPP_