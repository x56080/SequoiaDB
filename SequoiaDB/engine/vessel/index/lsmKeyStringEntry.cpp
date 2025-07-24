/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = lsmKeyStringEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
