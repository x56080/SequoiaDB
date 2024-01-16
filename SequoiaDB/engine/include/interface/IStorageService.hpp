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

   Source File Name = IStorageService.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_STORAGE_SERVICE_HPP_
#define SDB_I_STORAGE_SERVICE_HPP_

#include "interface/IPersistUnit.hpp"
#include "sdbInterface.hpp"
#include "interface/IStorageEngine.hpp"
#include "interface/IStorageBackupLogger.hpp"
#include "interface/ICollection.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      IStorageService define
    */
   class IStorageService : public SDBObject
   {
   public:
      IStorageService() = default ;
      virtual ~IStorageService() = default ;
      IStorageService( const IStorageService &o ) = delete ;
      IStorageService &operator =( const IStorageService & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const = 0 ;

      virtual INT32 openEngine( const dmsOpenEngineOptions &options ) = 0 ;
      virtual INT32 closeEngine( const dmsCloseEngineOptions &options ) = 0 ;

      virtual IStorageEngine *getEngine() = 0 ;

      virtual INT32 fsync( BOOLEAN isForce, BOOLEAN isSync, IExecutor *executor ) = 0 ;
      virtual INT32 backup( IStorageBackupLogger &backupLogger ) = 0 ;
      virtual INT32 getPersistUnit( IExecutor *executor, IPersistUnit *&persistUnit ) = 0 ;

      virtual INT32 createCS( const dmsCSMetadata &metadata,
                              const dmsCreateCSOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 dropCS( const dmsCSMetadata &metadata,
                            const dmsDropCSOptions &options,
                            IExecutor *executor ) = 0 ;

      virtual INT32 createCL( const dmsCLMetadata &metadata,
                              const dmsCreateCLOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 dropCL( const dmsCLMetadata &metadata,
                            const dmsDropCLOptions &options,
                            IContext *context,
                            IExecutor *executor ) = 0 ;

      virtual INT32 getCollection( const dmsCLMetadataKey &metadataKey,
                                   IExecutor *executor,
                                   std::shared_ptr< ICollection > &collPtr ) = 0 ;
      virtual INT32 loadCollection( const dmsCLMetadata &metadata,
                                    IExecutor *executor,
                                    std::shared_ptr< ICollection > &collPtr ) = 0 ;

      virtual BOOLEAN isAlterCompressorSupported() const = 0 ;
   } ;

}

#endif // SDB_I_STORAGE_SERVICE_HPP_