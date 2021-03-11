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

   Source File Name = openCLHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/openCLHandler.h"
#include "vessel/instanceEnv.h"
#include "vessel/collection.h"
#include "vessel/collectionSpace.h"
#include "vessel/collectionHandler.h"
#include "vessel/spaceIDLockHelper.h"

namespace engine
{
namespace vessel
{
   void openCLHandler::fini()
   {
      if (_context.isOpen())
      {
         _context.close();
      }
   }

   INT32 openCLHandler::doit(vesselImpl *db,
                             const strSlice &csName,
                             const strSlice &clName,
                             const openCLOptions &options,
                             collectionHandler &clHandler)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      CS_CONTAINER &cc = getEnv()->csContainer;
      collectionSpace *cs = NULL;
      collection *cl = NULL;

      if (OSS_UNLIKELY(NULL== db))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = cc.getCSByName(getContext(), csName, SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByName(getContext(), clName, SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      clHandler = collectionHandler(collectionHandle(cs->getLogicalID(),
                                                     cl->getLogicalID(),
                                                     cs->getSpaceID(),
                                                     cl->getMBID()),
                                    db);
   done:
      if (NULL != cl)
      {
         getContext()->unlockMB();
      }
      if (NULL != cs)
      {
         getContext()->unlockSpaceID();
      }
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine