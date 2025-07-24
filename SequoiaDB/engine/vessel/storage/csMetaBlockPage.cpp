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

   Source File Name = csMetaBlockPage.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

