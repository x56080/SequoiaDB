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

   Source File Name = dataExtentIDMapFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataExtentIDMapFile.h"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/storageUnitDef.h"
#include "vessel/spaceManagementPage.h"

namespace engine
{
namespace vessel
{
   const SEGMENT_ID META_FILE_FISRT_SEG = 0;
   const UINT32 META_FILE_GLOBAL_EXTENT = 0;

   dataExtentIDMapFile::dataExtentIDMapFile()
   {
      
   }

   dataExtentIDMapFile::~dataExtentIDMapFile()
   {
      
   }

   INT32 dataExtentIDMapFile::cacheHead()
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      rc = getUserDefinedHeadPtr(ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedt to get user head ptr:%d", rc);
         goto error;
      }

      _headCache = *((const dataIDMapFileHead *)ptr);
   done:
      return rc;
   error:
      goto done;
   }


   INT32 dataExtentIDMapFile::initUserDefinedHead(const void *userDefinedOptions, CHAR *headBuf)
   {
      INT32 rc = SDB_OK;
      
      const createSUOptions *options = NULL;
      dataIDMapFileHead *head = NULL;
      UINT32 checksum = 0;
      if (OSS_UNLIKELY(NULL == userDefinedOptions || NULL == headBuf))
      {
         PD_LOG(PDERROR, "invalid args ptr");
         rc = SDB_INVALIDARG;
         goto error;
      }

      options = (const createSUOptions *)userDefinedOptions;

      ossMemset(headBuf, 0, getCommonHeadInMem().userDefinedHeadLen);
      options = (const createSUOptions *)userDefinedOptions;
      head = (dataIDMapFileHead *)headBuf;
      head->version = META_FILE_USER_HEAD_VERSION;
      head->headChecksum = 0;
      head->data = options->dataArgs;
      head->index = options->idxArgs;
      head->meta = options->metaArgs;
      head->indexMeta = options->idxMetaArgs;
      
      rc = createChecksum(headBuf, sizeof(dataIDMapFileHead), checksum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checksum:%d", rc);
         goto error;
      }
      
      head->headChecksum = checksum;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataExtentIDMapFile::validateUserDefinedHead(const void *head)
   {
      INT32 rc = SDB_OK;
   
      dataIDMapFileHead tmpHead;
      const dataIDMapFileHead *inputHead = NULL;
      UINT32 checksum = 0;

      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "invalid head ptr");
         rc = SDB_INVALIDARG;
         goto error;
      }

      inputHead = (const dataIDMapFileHead *)head;
      tmpHead = *inputHead;
      tmpHead.headChecksum = 0;
      rc = createChecksum(&tmpHead, sizeof(dataIDMapFileHead), checksum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create head checksum:%d", rc);
         goto error;
      }

      if (checksum != inputHead->headChecksum)
      {
         PD_LOG(PDERROR, "checksum error:%d, %d", checksum, inputHead->headChecksum);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine