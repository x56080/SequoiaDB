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

   Source File Name = btreeKeyStringEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeKeyStringEntry.h"
#include "vessel/keyStringCoder.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   btreeKeyStringEntry::btreeKeyStringEntry(const slice &s):
   keyString(s)
   {
      if (!isValid())
      {
         PD_LOG(PDERROR, "failed to init key string");
         reset();
      }
      else if (hasKeyHead())
      {
         PD_LOG(PDERROR, "unexpected key head");
         reset();
      }
      else if (keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
      {
         PD_LOG(PDERROR, "unexpected key tail size");
         reset();
      }
   }

   btreeKeyStringEntry::btreeKeyStringEntry(UINT32 size, const CHAR *data):
   keyString(size, data)
   {
      if (!isValid())
      {
         PD_LOG(PDERROR, "failed to init key string");
         reset();
      }
      else if (hasKeyHead())
      {
         PD_LOG(PDERROR, "unexpected key head");
         reset();
      }
      else if (keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
      {
         PD_LOG(PDERROR, "unexpected key tail size");
         reset();
      }
   }

   INT32 btreeKeyStringEntry::init(const slice &s)
   {
      INT32 rc = SDB_OK;
      
      rc = keyString::init(s);
      if (SDB_OK != rc)
      {
         goto error;
      }
      else if (hasKeyHead())
      {
         PD_LOG(PDERROR, "unexpected key head");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (keyStringCoder::RID_ENCODING_SIZE != getKeyTailSize())
      {
         PD_LOG(PDERROR, "unexpected key tail size");
         rc = SDB_INVALIDARG;
         goto error; 
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 btreeKeyStringEntry::shallowCopy(const keyString &ks)
   {
      INT32 rc = SDB_OK;
      reset();
      if (OSS_UNLIKELY(!ks.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (ks.hasKeyHead())
      {
         PD_LOG(PDERROR, "unexpected key head");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (keyStringCoder::RID_ENCODING_SIZE != ks.getKeyTailSize())
      {
         PD_LOG(PDERROR, "unexpected key tail size");
         rc = SDB_INVALIDARG;
         goto error; 
      }

      keyString::operator=(ks);
   done:
      return rc;
   error:
      goto done;
   }

   recordID btreeKeyStringEntry::getRid() const
   {
      return keyString::getRid();
   }
} // namespace vessel

} // namespace engine
