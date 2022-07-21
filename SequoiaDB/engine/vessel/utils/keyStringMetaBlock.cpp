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

   Source File Name = keyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/keyStringMetaBlock.h"
#include "vessel/keyStringDef.h"
#include "ossLikely.hpp"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   void keyStringMetaBlock::reset()
   {
      version = 0;
      metaByte = 0;
      blockSize = 0;
      sizeBeforeKey = 0;
      keySize = 0;
      sizeAfterKey = 0;
      typeBitsSize = 0;
   }

   INT32 keyStringMetaBlock::init(const slice &s)
   {
      INT32 rc = SDB_OK;
      const CHAR *ksData = nullptr;
      UINT32 offset = 0;
      UINT32 bSize = 0;
      BOOLEAN hasSliceBeforeKey = FALSE;
      BOOLEAN isLargeKeySize = FALSE;
      BOOLEAN isLargeTypeBitsSize = FALSE;

      if (OSS_UNLIKELY(!s.isValid()))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid key string slice");
         goto error;
      }

      ksData = s.getData();
      offset = s.getSize() - 1;
      /// get version
      version = static_cast<UINT8>(*(ksData + offset));
      if (OSS_UNLIKELY(KEY_STRING_VERSION_1 != version))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid key string version");
         goto error;
      }
      offset--;
      bSize++;

      /// get meta byte
      metaByte = static_cast<UINT8>(*(ksData + offset));
      bSize++;

      /// parse meta byte
      if (1 == (metaByte & 0x01))
      {
         hasSliceBeforeKey = TRUE;
      }

      if (1 == ((metaByte >> 1) & 0x01))
      {
         isLargeKeySize = TRUE;
      }

      if (1 == ((metaByte >> 2) & 0x01))
      {
         isLargeTypeBitsSize = TRUE;
      }

      /// get size before key
      if (hasSliceBeforeKey)
      {
         offset--;
         sizeBeforeKey = static_cast<UINT8>(*(ksData + offset));
         if (OSS_UNLIKELY(0 == sizeBeforeKey))
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid size before key");
            goto error;
         }
         bSize++;
      }

      /// get key size
      if (isLargeKeySize)
      {
         offset -= 4;
         bSize += 4;
      }
      else
      {
         offset--;
         bSize++;
      }
      keySize = (*(ksData + offset));

      /// get type bits size
      if (isLargeTypeBitsSize)
      {
         offset -= 4;
         bSize += 4;
      }
      else
      {
         offset--;
         bSize++;
      }
      typeBitsSize = static_cast<UINT8>(*(ksData + offset));

      /// get block size and size after key
      blockSize = bSize;
      sizeAfterKey = s.getSize() -
                     (sizeBeforeKey + keySize + 
                        typeBitsSize + blockSize);

   done:
      return rc;
   error:
      reset();
      goto done;
   }
} // namespace vessel
} // namespace engine
