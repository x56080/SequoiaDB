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
#define VESSEL_VESSEL_H_

#include "vessel/vesselOptions.h"
#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/api/cursorHandler.h"
#include "vessel/api/collectionHandler.h"
#include "vessel/outerResource.h"
#include "sdbInterface.hpp"
#include "vessel/api/IQueryFilter.h"
#include "vessel/collectionSpaceIdentifier.h"

namespace engine
{
namespace vessel
{
   class IVessel : public SDBObject
   {
      public:
         IVessel(){}
         virtual ~IVessel(){}
         IVessel(const IVessel &) = delete;
         IVessel &operator=(const IVessel &) = delete;

      public:
         virtual BOOLEAN isOpen() = 0;

         virtual INT32 open(IExecutor *executor,
                            const outerResource *resource,
                            const openDBOptions &options) = 0;

         
         virtual INT32 close(IExecutor *executor,
                             const closeDBOptions &options) = 0;
         

         virtual INT32 createCollectionSpace(IExecutor *executor,
                                             const CHAR *name,
                                             utilCSUniqueID uniqueId,
                                             const createCSOptions &options,
                                             collectionSpaceIdentifier &identifier) = 0;

         virtual INT32 testCollectionSpace(IExecutor *executor,
                                           const CHAR *name,
                                           collectionSpaceIdentifier &identifier) = 0;

         /// cursor's mem managed by user.
         /// filter's mem managed by user.
         virtual INT32 listCollectionSpace(IExecutor *executor,
                                           IQueryFilter *filter,
                                           cursorHandler &cursor) = 0;

         virtual INT32 getCollectionSpaceCount(IExecutor *executor,
                                               UINT32 &count) = 0;


         virtual INT32 dropCollectionSpace(IExecutor *executor,
                                           const CHAR *name,
                                           UINT32 logicalID,
                                           const dropCSOptions &options) = 0;

         virtual INT32 createCollection(IExecutor *executor,
                                        const CHAR *csName,
                                        const CHAR* clName,
                                        utilCLInnerID innerID,
                                        const createCLOptions &options) = 0;

         virtual INT32 listCollections(IExecutor *executor,
                                       const CHAR *csName,
                                       IQueryFilter *filter,
                                       cursorHandler &cursor) = 0;

         virtual INT32 getCollectionCount(IExecutor *executor,
                                          const CHAR *csName,
                                          UINT32 &count) = 0;

         ///obj's mem managed by user
         virtual INT32 openCollection(IExecutor *executor,
                                      const CHAR *csName,
                                      const CHAR *clName,
                                      const openCLOptions &options,
                                      collectionHandler &handler) = 0;

   }; /// end of class IVessel
} /// end of namespace vessel
} /// end of namespace engine

#endif