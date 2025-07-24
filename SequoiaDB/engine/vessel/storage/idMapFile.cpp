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

   Source File Name = idMapFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/idMapFile.h"
#include "ossLikely.hpp"
#include "vessel/idMapFileDef.h"

namespace engine
{
namespace vessel
{
   /*
   BOOLEAN idMapFile::validateUserDefinedHead(const void *head)const
   {
      BOOLEAN r = FALSE;
      const idMapFileHead *h = NULL;
      storageCoreArgs args;

      if (OSS_UNLIKELY(NULL == head))
      {
         goto done;
      }

      h = (const idMapFileHead *)head;
      if (ID_MAP_FILE_HEAD_VERSION != h->version)
      {
         PD_LOG(PDERROR, "invalid id map file head version:%d", h->version);
         goto done;
      }

      args.pageSize = h->dataPageSize;
      args.maxPageCountPerSeg = h->dataPageCountInSeg;
      args.maxSegmentCountPerFile = h->dataSegCountInFile;
      if (!args.isValid())
      {
         PD_LOG(PDERROR, "invalid core args:%d,%d,%d",
                h->dataPageSize,
                h->dataPageCountInSeg,
                h->dataSegCountInFile);
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }*/


   void idMapFile::_close()
   {
      _pageCount = 0;
   }

   INT32 idMapFile::_open(BOOLEAN isCreating)
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = getUserDefinedHeaderPtr();
      if (0 == ptr)
      {
         PD_LOG(PDERROR, "failed to get user defined header ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _pageCount = ((const idMapFileHead *)(ptr))->totalPageCount;

   done:
      return rc;
   error:
      goto done;
   }

   void idMapFile::_onHeaderUpdated(const slice &hs)
   {
      SDB_ASSERT(hs.isValid(), "can not be invalid");
      SDB_ASSERT(hs.getSize() == sizeof(idMapFileHead), "must be same");
      _pageCount = ((const idMapFileHead *)hs.getData())->totalPageCount;
      return;
   }
   
   INT32 idMapFile::getIdMapFileHead(idMapFileHead &h)const
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      ptr = getUserDefinedHeaderPtr();
      if (0 == ptr)
      {
         PD_LOG(PDERROR, "failed to get user defined header ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      h = *((const idMapFileHead *)ptr);
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine
