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

   Source File Name = btreeAccessContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeAccessContext.h"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
////////////btreeAccessContext
   btreeAccessContext::btreeAccessContext(const indexContext *ic):
   _path(ic)
   {

   }
////////////btreeAccessContext end

////////////btreeInsertContext
   btreeInsertContext::btreeInsertContext(const indexContext *ic):
   btreeAccessContext::btreeAccessContext(ic)
   {

   }

   btreeInsertContext::~btreeInsertContext()
   {}

   INT32 btreeInsertContext::init(const ixmKey &key,
                                  const recordID &rid,
                                  const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(!key.isValid() ||
                       !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((INT32)MAX_IXM_KEY_SIZE < key.dataSize())
      {
         rc = SDB_IXM_KEY_TOO_LARGE;
         goto error;
      }

      _key.assign(key);
      _rid = rid;
      if (transID.isValid())
      {
         _transID = transID;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void btreeInsertContext::fini()
   {
      if (_key.isValid())
      {
         _key.assign(NULL);
         _rid = recordID();
         _transID = DPS_TRANS_ID();
         _pessimistically = FALSE;

         _obstructed = FALSE;
         _path.fini();
         _pos = INVALID_RECORD_SLOT_ID;
      }
      return;
   }
////////////btreeInsertContext end
} // namespace vessel

} // namespace engine
