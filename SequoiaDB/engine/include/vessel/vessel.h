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

   Source File Name = vessel.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_H_
#define VESSLE_VESSEL_H_

#include "vessel/vesselDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/cursorHandler.h"
#include "vessel/collectionHandler.h"

namespace engine
{
namespace vessel
{
   class ISession;
   class outerResource;
   class IQueryFilter;

   class vessel
   {
      public:
         vessel(){}
         virtual ~vessel(){}

      public:
         virtual BOOLEAN isOpen() = 0;
         virtual INT32 initOuterResource(const outerResource &resource) = 0;
         virtual INT32 open(ISession *session, const openDBOptions &options) = 0;

         
         virtual INT32 close(ISession *session, const closeDBOptions &options) = 0;
         

         virtual INT32 createCollectionSpace(ISession *session,
                                             const CHAR *name,
                                             utilCSUniqueID uniqueID, 
                                             const createCSOptions &options) = 0;

         /// cursor's mem managed by user.
         /// filter's mem managed by user.
         virtual INT32 listCollectionSpace(ISession *session,
                                           IQueryFilter *filter,
                                           cursorHandler &cursor) = 0;

         virtual INT32 getCollectionSpaceCount(ISession *session,
                                               UINT32 &count) = 0;
/*
         virtual INT32 alterCollectionSpace(ISession *session,
                                            const CHAR *name,
                                            UINT32 logicalID,
                                            alterCSOptions &options) = 0;*/


         virtual INT32 dropCollectionSpace(ISession *session,
                                           const CHAR *name,
                                           UINT32 logicalID,
                                           const dropCSOptions &options) = 0;

         virtual INT32 createCollection(ISession *session,
                                        const CHAR *csName,
                                        const CHAR* clName,
                                        utilCLInnerID innerID,
                                        const createCLOptions &options) = 0;

         virtual INT32 listCollections(ISession *session,
                                       const CHAR *csName,
                                       IQueryFilter *filter,
                                       cursorHandler &cursor) = 0;

         virtual INT32 getCollectionCount(ISession *session,
                                          const CHAR *csName,
                                          UINT32 &count) = 0;

         ///obj's mem managed by user
         virtual INT32 openCollection(ISession *session,
                                      const CHAR *csName,
                                      const CHAR *clName,
                                      const openCLOptions &options,
                                      collectionHandler &handler) = 0;
/*
         virtual INT32 createCollection(ISession *session,
                                        const clNameOrID &noi,
                                        const createCLOptions &options) = 0;

         virtual INT32 alterCollection(ISession *session,
                                       const clNameOrID &noi,
                                       const alterCLOptions &options) = 0;

         virtual INT32 dropCollection(ISession *session,
                                      const clNameOrID &noi,
                                      const dropCLOptions &options) = 0;

         virtual INT32 createIndex(ISession *session,
                                   const clNameOrID &noi,
                                   const CHAR *indexName,
                                   UINT32 id,
                                   const createIndexOptions &options) = 0;

         virtual INT32 alterIndex(ISession *session,
                                  const clNameOrID &noi,
                                  const CHAR *indexName,
                                  UINT32 id,
                                  const alterIndexOptions &options) = 0;

         virtual INT32 dropIndex(ISession *session,
                                 const clNameOrID &noi,
                                 const CHAR *indexName,
                                 UINT32 id,
                                 const dropIndexOptions &options) = 0;

         virtual INT32 openCollection(ISession *session,
                                      const clNameOrID &noi,
                                      const openCLOptions &options,
                                      COLLECTION_HANDLE &handle);

         virtual INT32 openCollection(ISession * session,
                                      const clNameOrID &noi,
                                      const openCLOption &options,
                                      collection &clHandler);

         virtual INT32 closeCollection(COLLECTION_HANDLE handle);

         

         virtual INT32 resetScan(ISession *session,
                                 COLLECTION_HANDLE handle,
                                 const scanOptions &options) = 0;

         virtual INT32 next(ISession *session,
                            COLLECTION_HANDLE handle,
                            slice &record) = 0;

         virtual INT32 openScanCursor(ISession *session,
                                      COLLECTION_HANDLE handle,
                                      const scanOptions &options,
                                      collectionScanCursor *cursor) = 0;

         virtual INT32 resetIndexScan(ISession *session,
                                      COLLECTION_HANDLE handle,
                                      UINT32 indexID,
                                      const CHAR *indexName,
                                      const idxScanOptions &options) = 0;

         virtual INT32 advance(ISession *session,
                               COLLECTION_HANDLE handle,
                               slice &key,
                               recordID &rid) = 0;

         virtual INT32 openIndexCursor(sessionContext *sc,
                                       const nameOrId &noi,
                                       UINT32 indexId,
                                       const CHAR *indexName,
                                       const idxScanOptions &options,
                                       indexScanCursor *cursor) = 0;

         virtual INT32 closeCursor(sessionContext *sc,
                                   ICursor *cursor);

         virtual INT32 update(ISession *session,
                              COLLECTION_HANDLE handle,
                              const recordID &rid,
                              const slice &newRecord,
                              const updateOptions &options) = 0;

         virtual INT32 remove(sessionContext *sc,
                              CS_SLOT_ID cs,
                              CL_SLOT_ID cl,
                              const recordID &rid,
                              const removeOptions &options) = 0;
*/
   }; /// end of class vessel
} /// end of namespace vessel
} /// end of namespace engine

#endif