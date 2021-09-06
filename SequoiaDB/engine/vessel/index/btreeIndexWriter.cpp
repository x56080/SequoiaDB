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

   Source File Name = btreeIndexWriter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeIndexWriter.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "vessel/btreeNode.h"

namespace engine
{
namespace vessel
{
   INT32 btreeIndexWriter::init(requestContext *context,
                                indexContext *ic)
   {
      INT32 rc = SDB_OK;
      fini();

      rc = btreeIndexAccessor::_init(context, ic);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init index accessor:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void btreeIndexWriter::fini()
   {
      btreeIndexAccessor::_fini();
   }

   INT32 btreeIndexWriter::insert(const bson::BSONObj &key,
                                  const recordID &rid,
                                  DPS_LSN_OFFSET lsn,
                                  const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      btreeNode root;

      if (OSS_UNLIKELY(!btreeIndexAccessor::_isInitialized()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid() ||
                            key.isEmpty() ||
                            !rid.valid() ||
                            DPS_INVALID_LSN_OFFSET == lsn))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         ixmKeyOwned ownedKey(key);
         if (MAX_IXM_KEY_SIZE < ownedKey.dataSize())
         {
            rc = SDB_IXM_KEY_TOO_LARGE;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine