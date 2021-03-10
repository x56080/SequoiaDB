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

   Source File Name = insertHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/insertHandler.h"
#include "vessel/instanceEnv.h"
#include "vessel/collection.h"
#include "vessel/collectionSpace.h"
#include "vessel/recordData.h"
#include "vessel/spaceIDLockHelper.h"

namespace engine
{
namespace vessel
{
   INT32 insertHandler::doit(const collectionHandle &handle,
                              const recordData &record,
                              const DPS_TRANS_ID &transID,
                              STRIPING_ID striping,
                              const insertOptions &options,
                              utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      collection *cl = NULL;

      if (OSS_UNLIKELY(!handle.valid() ||
                       !record.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getEnv()->csContainer.getCSBySpaceID(getContext(),
                                                handle.getSpaceID(),
                                                handle.getCSLId(),
                                                SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cs->getCollectionByMBID(getContext(), handle.getMbId(),
                                   handle.getCLLId(), SHARED, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = cl->insert(getContext(), record, transID, striping, options, res);
      if (SDB_IXM_DUP_KEY == rc)
      {
         goto error;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert record into collection:%d", rc);
         goto error;
      }
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