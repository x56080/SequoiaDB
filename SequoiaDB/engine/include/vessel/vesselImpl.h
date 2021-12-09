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
#include "vessel/indexOptions.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/indexParameters.h"

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
         virtual BOOLEAN isOpen(){return _open;}

         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options);
                            
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options);

         virtual INT32 listCollectionSpace(IExecutor *executor,
                                           IQueryFilter *filter,
                                           cursorHandler &cursor);

         virtual INT32 getCollectionSpaceCount(IExecutor *executor,
                                               UINT32 &count);

         virtual INT32 createCollectionSpace(IExecutor *executor,
                                             const CHAR *name,
                                             utilCSUniqueID uniqueId,
                                             const createCSOptions &options,
                                             collectionSpaceId &identifier);

         /// lazy to modify all unit tests after add identifier in createCollectionSpace
         INT32 createCollectionSpace(IExecutor *executor,
                                             const CHAR *name,
                                             utilCSUniqueID uniqueId,
                                             const createCSOptions &options);

         virtual INT32 testCollectionSpace(IExecutor *executor,
                                           const CHAR *name,
                                           collectionSpaceId &identifier);

         virtual INT32 dropCollectionSpace(IExecutor *executor,
                                           const CHAR *name,
                                           UINT32 logicalID,
                                           const dropCSOptions &options);

         virtual INT32 createCollection(IExecutor *executor,
                                        const CHAR *csName,
                                        const CHAR* clName,
                                        utilCLInnerID innerID,
                                        const createCLOptions &options);
                                        
         virtual INT32 listCollections(IExecutor *executor,
                                       const CHAR *csName,
                                       IQueryFilter *filter,
                                       cursorHandler &cursor);

         virtual INT32 getCollectionCount(IExecutor *executor,
                                          const CHAR *csName,
                                          UINT32 &count);

         virtual INT32 openCollection(IExecutor *executor,
                                      const CHAR *csName,
                                      const CHAR *clName,
                                      const openCLOptions &options,
                                      collectionHandler &handler);

      public:
         INT32 createIndex(IExecutor *executor,
                           const globalCollectionId &gcid,
                           const strSlice &indexName,
                           const bson::BSONObj &keyPattern,
                           const indexParameters &params,
                           const createIndexOptions &options);

         INT32 listIndexes(IExecutor *executor,
                           const globalCollectionId &gcid,
                           ossPoolVector<bson::BSONObj> &indexes);
      
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

         INT32 getTotalRecordCountInPageHead(IExecutor *executor,
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
         outerResource _outerResource;
   }; /// end of class vesselImpl 


} /// end of namespace vessel 
} /// end of namespace engine
#endif // VESSEL_VESSEL_IMPL_H_