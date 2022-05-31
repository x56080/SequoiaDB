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

   Source File Name = csMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/csMetaBlockPage.h"

namespace engine
{
namespace vessel
{
   BOOLEAN csMetaBlock::isValid()const
   {
      BOOLEAN r = FALSE;
      if (CS_META_BLOCK_VERSION_1 != version)
      {
         goto done;
      }
      else if (CS_STATUS_INVALID == status)
      {
         goto done;
      }
      else if (CS_TYPE_NORMAL != type)
      {
         goto done;
      }
      else if (0 == name[0] ||
               0 != name[DMS_COLLECTION_SPACE_NAME_SZ])
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }

   BOOLEAN initCSMetaBlockPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               void *buf)
   {
      BOOLEAN r = FALSE;
      csMetaBlock record;

      if (!initCommonPage(PAGE_TYPE_CS_META, pageSize, pid, lpid, psv, buf))
      {
         goto done;
      }
      
      ossMemcpy((void *)((ossValuePtr)buf + PAGE_HEAD_SIZE), &record, CS_META_BLOCK_LEN);

      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine

