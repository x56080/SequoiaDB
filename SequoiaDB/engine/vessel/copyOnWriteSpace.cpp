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

   Source File Name = copyOnWriteSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/copyOnWriteSpace.h"
#include "vessel/storageUnit.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "ossLikely.hpp"
#include "utilStr.hpp"
#include "vessel/idMapPage.h"
#include "vessel/spaceManagementPage.h"

namespace engine
{
namespace vessel
{
   copyOnWriteSpace::~copyOnWriteSpace()
   {
      fini();
   }

   void copyOnWriteSpace::fini()
   {
      if (_controlFile.isOpen())
      {
         _controlFile.close();
      }
      _su = NULL;
      _inMemLpidPool.fini();
      _inMemPidPool.fini(); 
      return;
   }

   INT32 copyOnWriteSpace::open(requestContext *context,
                                storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _su, "do not reopen");
      strSlice dirSlice;
      strSlice pathSlice;
      UINT32 idxMetaPageSize = 0;
      UINT32 idxDataPageSize = 0;
       

      if (NULL == context ||
          NULL == su ||
          !su->isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = su->getCoreArgs(FILE_TYPE_IDX_M, &idxMetaPageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }
      rc = su->getCoreArgs(FILE_TYPE_IDX_D, &idxDataPageSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get core args:%d", rc);
         goto error;
      }

      pathSlice.reset(context->getEnv()->options.path.dataPath.c_str(),
                      context->getEnv()->options.path.dataPath.size());
      dirSlice.reset(su->getDirName());
      SDB_ASSERT(!pathSlice.empty(), "can not be empty");
      SDB_ASSERT(!dirSlice.empty(), "can not be empty");
      if (pathSlice.empty() || dirSlice.empty())
      {
         PD_LOG(PDERROR, "invalid path or dir");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = initControlFile(context, pathSlice, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpace::preallocateNewIndexPages(requestContext *context,
                                                    UINT32 count,
                                                    PAGE_ID *lpids,
                                                    PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context) ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == lpids)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpace::initControlFile(requestContext *context,
                                           const strSlice &path,
                                           const strSlice &dirPath)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!path.empty() && !dirPath.empty(), "can not be empty");
      SDB_ASSERT(!_controlFile.isOpen(), "can not be open");
      CHAR *pathBuffer = NULL;
      UINT32 bufferSize = path.strLen() + dirPath.strLen() + 2;
      
      pathBuffer = context->allocateBuffer(bufferSize);
      if (NULL == pathBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = utilBuildFullPath(path.str(), dirPath.str(),
                             bufferSize, pathBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build file path:%d", rc);
         goto error;
      }

      rc = _controlFile.open(pathBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control file:%d", rc);
         goto error;
      }
   done:
      if (NULL != pathBuffer)
      {
         context->releaseBuffer(pathBuffer, bufferSize);
      }
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpace::restoreSpaceToLastCheckpoint(requestContext *context,
                                                        storageUnit *su)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_controlFile.isOpen(), "must be open");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != su, "can not be null");
      copyOnWriteStorageUnit &cowUnit = su->getCowUnit();
      if (0 == _controlFile.getAliveVersionCount())
      {

      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 copyOnWriteSpace::initInMemBitMap(UINT32 idxMetaPageSize,
                                           UINT32 idxDataPageSize)
   {
      INT32 rc = SDB_OK;
      UINT32 lpidCapacity = 0;
      UINT32 smpCapacity = 0;
      rc = getCapacityOfIMP(idxMetaPageSize, lpidCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of id map:%d", rc);
         goto error;
      }

      rc = getSMPCapacity8BytesAligned();
      
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine