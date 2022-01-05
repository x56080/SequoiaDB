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

   Source File Name = vesselImpl.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_IMPL_H_
#define VESSEL_VESSEL_IMPL_H_

#include "vessel/api/vessel.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/indexParameters.h"
#include "vessel/dmlRequest.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class cursorKernal;
   
   class vesselImpl : public IVessel 
   {
      public:
         vesselImpl(){}
         virtual ~vesselImpl();
      public:
         OSS_INLINE BOOLEAN isOpen()const {return _open;}

         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options);
                            
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options);

      /// IDataStorageEngine begin
      public: 
         virtual INT32 createCS(IExecutor *executor,
                                const CHAR *name,
                                const utilCSUniqueID &uniqueId,
                                const dmsCreateCSOptions &o,
                                const bson::BSONObj &adjunct);

         virtual INT32 testCS(IExecutor *executor,
                              const CHAR *name,
                              utilCSUniqueID &uniqueId);

         virtual INT32 testCS(IExecutor *executor,
                              const utilCSUniqueID &uniqueId);

         virtual INT32 listCS(IExecutor *executor,
                              DATA_CURSOR_PTR &cursor);

         virtual INT32 getCSCount(IExecutor *executor,
                                  UINT32 &countt); 

         virtual INT32 removeCS(IExecutor *executor,
                                const CHAR *csName);

      public:

         virtual INT32 createCL(IExecutor *executor,
                                const CHAR *fullName,
                                const utilCLUniqueID &uniqueId,
                                const dmsCreateCLOptions &o,
                                const bson::BSONObj &adjunct);

         virtual INT32 removeCL(IExecutor *executor,
                                const CHAR *fullName,
                                const dmsRemoveCLOptions &o);

         virtual INT32 testCL(IExecutor *executor,
                              const CHAR *fullName,
                              utilCLUniqueID &uniqueId);
                        
         virtual INT32 testCL(IExecutor *executor,
                              const utilCLUniqueID &uniqueId);

         virtual INT32 listCL(IExecutor *executor,
                              const CHAR *csName,
                              DATA_CURSOR_PTR &cursor);

         virtual INT32 openCL(IExecutor *executor,
                              const CHAR *fullName,
                              const dmsOpenCLOptions &o,
                              DATA_COLLECTION_PTR &ptr);

         virtual INT32 getCLCount(IExecutor *executor,
                                  const CHAR *csName,
                                  UINT32 &count);

      /// IDataStorageEngine end

      public:
         INT32 createIndex(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const dmsBuildIndexOptions &o,
                           const bson::BSONObj &adjunct);

         INT32 listIndexes(IExecutor *executor,
                           const globalCollectionId &gcid,
                           ossPoolVector<bson::BSONObj> &indexes);

         INT32 removeIndex(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const CHAR *indexName);

         INT32 testIndex(IExecutor *executor,
                         const globalCollectionId &gcid,
                         const strSlice &indexName,
                         indexIdentifier &indexId);

      public:
         INT32 truncate(IExecutor *executor,
                        const globalCollectionId &gcid,
                        const dmsTruncateCLOptions &o);
      
      public:

         INT32 insert(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlInsertRequest &request,
                      utilInsertResult *res);

         INT32 insertBatch(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const dmlBatchInsertRequest &request,
                           utilInsertResult *res);

         INT32 update(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlUpdateRequest &request,
                      IRecordUpdater *updater,
                      utilUpdateResult *res);

         INT32 remove(IExecutor *executor,
                      const globalCollectionId &gcid,
                      const dmlRemoveRequest &request,
                      utilDeleteResult *res);

         INT32 count(IExecutor *executor,
                     const globalCollectionId &gcid,
                     UINT64 &count);

      public:
         INT32 pushMoreToCursor(IExecutor *executor,
                                 cursorKernal *cursor);   
         
      private:

         void fini();
         INT32 initLsmDB(const openDBOptions &options);
         INT32 flushWholeDirtyList(requestContext *context);

      private:
         BOOLEAN _open = FALSE;
         instanceEnv _env;
   }; /// end of class vesselImpl 


} /// end of namespace vessel 
} /// end of namespace engine
#endif // VESSEL_VESSEL_IMPL_H_