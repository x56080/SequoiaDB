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

   Source File Name = btreeScanEntryParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeScanEntryParser.h"
#include "ixmKey.hpp"

namespace engine
{
namespace vessel
{
   INT32 btreeScanEntryParser::parse(const slice &entryData)
   {
      INT32 rc = SDB_OK;
      const _fixedSizeFields *fields = NULL;
      UINT32 keySize = 0;
      const CHAR *keyData = NULL;

      if (OSS_UNLIKELY(entryData.getSize() <= sizeof(_fixedSizeFields)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fields = entryData.getReadableObjPtr<_fixedSizeFields>(0);
      if (!fields->indexRid.isValid() ||
          !fields->rid.isValid())
      {
         PD_LOG(PDERROR, "invalid field value found");
         rc = SDB_INVALIDARG;
         goto error;
      }

      keyData = entryData.getReadablePtrWithoutSize(sizeof(_fixedSizeFields));
      keySize = ixmKey(keyData).dataSize();
      if (entryData.getSize() < (sizeof(_fixedSizeFields) + keySize))
      {
         PD_LOG(PDERROR, "size[%d] parsed is out of entry size[%d]",
                keySize + sizeof(_fixedSizeFields), entryData.getSize());
         rc = SDB_VESSEL_INVALID_PTR_OFFSET;
         goto error;
      }

      _fields = *fields;
      _keySlice = slice(keySize, keyData);
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void btreeScanEntryParser::init(const recordID &indexRid,
                                    const recordID &rid,
                                    const DPS_TRANS_ID &transID,
                                    const slice &keySlice)
   {
      SDB_ASSERT(indexRid.isValid(), "can not be invalid");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(!keySlice.isEmpty(), "can not be invalid");

      _fields.indexRid = indexRid;
      _fields.rid = rid;
      _fields.transID = transID;
      _keySlice = keySlice;
      return;
   }
} // namespace vessel

} // namespace engine

