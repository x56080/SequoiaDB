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

   Source File Name = btreeKeyStringEntry.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
   
   btreeKeyStringEntry &btreeKeyStringEntry::operator=(const btreeKeyStringEntry &o)
   {
      reset();
      if (o.isValid())
      {
         keyString::operator=(static_cast<keyString>(o));
      }
      return *this;
   }

   btreeKeyStringEntry::btreeKeyStringEntry(const btreeKeyStringEntry &o):
   keyString(static_cast<keyString>(o))
   {

   }

   btreeKeyStringEntry::btreeKeyStringEntry(btreeKeyStringEntry &&o)noexcept:
   keyString(std::move(o))
   {
      
   }

   btreeKeyStringEntry &btreeKeyStringEntry::operator=(btreeKeyStringEntry &&o)noexcept
   {
      reset();
      keyString::operator=(std::move(o));
      return *this;
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

   INT32 btreeKeyStringEntry::moveFrom(keyString &&ks)
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
      keyString::operator=(std::move(ks));

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
