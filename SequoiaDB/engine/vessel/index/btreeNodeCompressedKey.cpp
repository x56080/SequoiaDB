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

   Source File Name = btreeNodeCompressedKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNodeCompressedKey.h"
#include "vessel/btreeNodePage.h"

namespace engine
{
namespace vessel
{
   btreeNodeCompressedKey::~btreeNodeCompressedKey()
   {
      if (NULL != _ownedSuffix)
      {
         _allocator.Free(_ownedSuffix);
      }
   }

   void btreeNodeCompressedKey::shallowInit(UINT32 prefixPos,
                                            const ixmKey &prefix,
                                            const ixmKey &suffix)
   {
      SDB_ASSERT(prefix.isValid(), "can not be invalid");
      reset();
      _prefixPos = prefixPos;
      _prefix.assign(prefix);
      _suffix.assign(suffix);
      return;
   }

   void btreeNodeCompressedKey::reset()
   {
      _prefixPos = -1;
      _prefix.assign(NULL);
      _suffix.assign(NULL);
      if (NULL != _ownedSuffix)
      {
         _allocator.Free(_ownedSuffix);
         _ownedSuffix = NULL;
      }
      return;
   }

   BOOLEAN btreeNodeCompressedKey::hasSuffix()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _suffix.isValid();
   }

   BOOLEAN btreeNodeCompressedKey::isSuffixOwned()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return NULL != _ownedSuffix;
   }
   
   INT32 btreeNodeCompressedKey::getSuffixOwned()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!hasSuffix())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      if (!isSuffixOwned())
      {
         UINT32 size = _suffix.dataSize();
         _ownedSuffix = (CHAR*)_allocator.Malloc(size);
         if (OSS_UNLIKELY(NULL == _ownedSuffix))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }
         ossMemcpy(_ownedSuffix, _suffix.data(), size);
         _suffix.assign(_ownedSuffix);
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 btreeNodeCompressedKey::getSuffixSize()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 size = 0;
      if (hasSuffix())
      {
         size = _suffix.dataSize();
      }
      return size;
   }
} // namespace vessel

} // namespace engine


