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

   Source File Name = lsmKeyStringEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsmKeyStringEntry.h"
#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   lsmKeyStringEntry::lsmKeyStringEntry(const slice &s):
   keyString(s)
   {
      if (isValid())
      {
         if (keyStringCoder::INDEX_ID_ENCODEING_SIZE != getKeyHeadSize() ||
             keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
         {
            reset();
         }
      }
   }

   lsmKeyStringEntry::lsmKeyStringEntry(UINT32 size, const CHAR *data):
   keyString(size, data)
   {
      if (isValid())
      {
         if (keyStringCoder::INDEX_ID_ENCODEING_SIZE != getKeyHeadSize() ||
             keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
         {
            reset();
         }
      }
   }

   INT32 lsmKeyStringEntry::init(const slice &s)
   {
      INT32 rc = SDB_OK;
      rc = keyString::init(s);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (keyStringCoder::INDEX_ID_ENCODEING_SIZE != getKeyHeadSize() ||
               keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   recordID lsmKeyStringEntry::getRid() const
   {
      if (isValid())
      {
         return keyStringCoder().decodeToRid(getKeyTailSlice().data());
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return recordID();
      }
   }

   globalIndexID lsmKeyStringEntry::getIndexId() const
   {
      if (isValid())
      {
         return keyStringCoder().decodeToIndexId(getKeyHeadSlice().data());
      }
      else
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         return globalIndexID();
      }
   }
} // namespace vessel

} // namespace engine
