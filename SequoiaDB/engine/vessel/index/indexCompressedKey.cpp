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

   Source File Name = indexCompressedKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexCompressedKey.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/indexKeySuffix.h"

namespace engine
{
namespace vessel
{
   UINT32 indexCompressedKey::getKeyDataSize()const
   {
      UINT32 size = 0;
      const CHAR *p = _data;
      const CHAR *ixmKeyData = NULL;
      if (!isValid())
      {
         goto done;
      }

      do
      {
         INT32 thisKeysize = getThisKeySize(p);
         if (thisKeysize < 0)
         {
            PD_LOG(PDERROR, "failed to get key size");
            goto done;
         }

         size += (UINT32)thisKeysize;

         if (indexKeySuffix::hasMoreSuffixKey(*p))
         {
            p += (UINT32)thisKeysize;
            continue;
         }
         else if (indexKeySuffix::hasMoreKey(*p))
         {
            ixmKeyData = p + (UINT32)thisKeysize;
            break;
         }
         else
         {
            break;
         }
         
      } while (TRUE);
      
      if (NULL != ixmKeyData)
      {
         size += ixmKey(ixmKeyData).dataSize();
      }

   done:
      return size;
   }

   INT32 indexCompressedKey::getThisKeySize(const CHAR *flags)const
   {
      SDB_ASSERT(NULL != flags, "can not be null");
      INT32 size = -1;
      indexKeySuffix::TYPE type = indexKeySuffix::getSuffixType(*flags);

      switch (type)
      {
      case indexKeySuffix::TYPE_0B:
         size = 1;
         break;
      case indexKeySuffix::TYPE_1B:
         size = 2;
         break;
      case indexKeySuffix::TYPE_2B:
         size = 3;
         break;
      case indexKeySuffix::TYPE_4B:
         size = 5;
         break;
      case indexKeySuffix::TYPE_BYTES:
      {
         UINT8 s = *(flags + 1);
         size = 2 + s;
         break;
      }
      case indexKeySuffix::TYPE_LONG_BYTES:
      {
         UINT16 s = *((const UINT16 *)(flags + 1));
         size = 3 + s;
         break;
      }
      default:
         PD_LOG(PDERROR, "invalid suffix type[%d]", type);
         break;
      }

   done:
      return size;
   }
} // namespace vesse;

} // namespace engine
