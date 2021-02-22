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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

#include "vessel/vessel.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   
   class vesselImpl : public vessel, SDBObject
   {
      public:
         vesselImpl();
         virtual ~vesselImpl();
      public:
         virtual BOOLEAN isOpen(){return _open;}
         virtual INT32 setup(const outerResource &resource);
         virtual INT32 open(ISession *session, const openDBOptions &options);
         virtual INT32 close(ISession *session, const closeDBOptions &options);

         virtual INT32 fastGetCollectionSpaceCount(ISession *session,
                                                   UINT32 &count);

         virtual INT32 listCollectionSpace(ISession *session,
                                           IQueryFilter *filter,
                                           ICursor *cursor);

         virtual INT32 createCollectionSpace(ISession *session,
                                             const CHAR *name,
                                             const createCSOptions &options);

         virtual INT32 dropCollectionSpace(ISession *session,
                                           const CHAR *name,
                                           UINT32 logicalID,
                                           const dropCSOptions &options);

         virtual INT32 createCollection(ISession *session,
                                        UINT32 csLogicalID,
                                        const CHAR* clName,
                                        UINT32 clLogicalID,
                                        const createCLOptions &options);
                                        
         virtual INT32 listCollections(ISession *session,
                                       UINT32 csLogicalID,
                                       IQueryFilter *filter,
                                       ICursor *cursor);

         virtual INT32 openCollection(ISession *session,
                                      UINT32 csLogicalID,
                                      UINT32 clLogicalID,
                                      const openCLOptions &options,
                                      collectionObject *obj);

      public:
         virtual INT32 pushMoreToCursor(ISession * session,
                                        cursorObject *cursor);

         virtual INT32 insert(ISession *session,
                              const collectionHandle *handle,
                              const slice &record,
                              const insertOptions &options);
      private:

         INT32 initObjectContainer(ISession *session);

         INT32 flushWholeDirtyList(requestContext *context);

         INT32 testCollection(requestContext *context,
                              UINT32 cslid,
                              UINT32 cllid,
                              SPACE_ID &sid,
                              CL_MB_ID &mid);

      private:
         BOOLEAN _open;
         instanceEnv _env;
         outerResource _outerResource;
         
   }; /// end of class vesselImpl 
} /// end of namespace vessel 
} /// end of namespace engine
#endif // VESSEL_VESSEL_IMPL_H_