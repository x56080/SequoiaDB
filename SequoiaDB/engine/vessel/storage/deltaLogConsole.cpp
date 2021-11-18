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

   Source File Name = deltaLogConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogConsole.h"
#include "ossLikely.hpp"
#include "vessel/deltaLogFileDef.h"
#include "vessel/requestContext.h"
#include "vessel/deltaLogRecordBuilder.h"
#include "vessel/storageFile.h"
#include "vessel/storageUtils.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/idMapFile.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 DEFAULT_LOG_BUFFER_SIZE = 4096;

   deltaLogConsole::deltaLogConsole()
   {}

   deltaLogConsole::~deltaLogConsole()
   {
      fini();
   }

   INT32 deltaLogConsole::init(requestContext *context,
                               const idMapFile *base,
                               const FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      fini();
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isOpen() ||
                       NULL == base ||
                       !base->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _type = base->getCommonHeadInMem().spaceType;

      rc = initLogFiles(context, base, fl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init delta log files:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void deltaLogConsole::fini()
   {
      _type = INVALID_SPACE_TYPE;
      _files.close();
      if (NULL != _reserved)
      {
         _reserved->destroy();
         SDB_OSS_DEL _reserved;
         _reserved = NULL;
      }
      return;
   }

   void deltaLogConsole::destroy()
   {
      _files.destroy();
      fini();
      return;
   }

   deltaLogFile *deltaLogConsole::getLatestFile()
   {
      SDB_ASSERT(!_files.isEmpty(), "can not be empty");
      return static_cast<deltaLogFile *>(_files.getBack());
   }

   INT32 deltaLogConsole::reserveTmpLogFile(requestContext *context,
                                            UINT32 secretValue)
   {
      INT32 rc = SDB_OK;
      vesselFileName fn;
      createStorageFileOptions o;
      deltaLogFileHead tmp;
      tmp.version = deltaLogFile::VERSION; 
      slice head(sizeof(tmp), &tmp);
      UINT64 seq = 0;
      deltaLogFile *file = NULL;
      storageUnit *su = NULL;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _reserved)
      {
         SDB_ASSERT(_reserved->isOpen(), "must be open");
         goto done;
      }

      file = SDB_OSS_NEW deltaLogFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      if (! _files.isEmpty())
      {
         seq = _files.getBack()->getCommonHeadInMem().sequence + 1;
      }

      if (!fn.build(context->getSpaceID(), FILE_TYPE_DELTA_LOG, _type, seq))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.secretValue = secretValue;
      o.args = storageCoreArgs(deltaLogFile::PAGE_SIZE,
                               deltaLogFile::PAGE_COUNT_PER_SEGMENT,
                               deltaLogFile::MAX_SEGMENT_COUNT_PER_FILE);
      o.replaceWhenCreate = TRUE;
      o.createAsTmpFile = TRUE;

      rc = su->createStorageFile(fn, o, head, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }

      _reserved = file;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 deltaLogConsole::addReservedFileToList()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == _reserved))
      {
         PD_LOG(PDERROR, "no file reserved");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _reserved->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%s, rc:%d", _reserved->getFullPath(), rc);
         goto error;
      }

      rc = _reserved->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remvoe shadow suffix from file name:%d", rc);
         goto error;   
      }

      rc = _files.pushBack(_reserved);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push back file:%d", rc);
         goto error;
      }

      _reserved = NULL;
      _files.truncate(3);

   done:
      return rc;
   error:
      if (NULL != _reserved)
      {
         _reserved->destroy();
         SDB_OSS_DEL _reserved;
         _reserved = NULL;
      }
      goto done;
   }

   INT32 deltaLogConsole::initLogFiles(requestContext *context,
                                       const idMapFile *base,
                                       const FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be invalid");
      SDB_ASSERT(_files.isEmpty(), "must be empty");

      storageFile *file = NULL;
      FILE_NAME_LIST::const_iterator itr;
      storageUnit *su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      if (NULL == fl)
      {
         goto done;
      }

      itr = fl->begin();
      for (; itr != fl->end(); ++itr)
      {
         const vesselFileName &fn = *itr;
         if (OSS_UNLIKELY(!fn.isValid()))
         {
            PD_LOG(PDERROR, "invalid file name");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (OSS_UNLIKELY(fn.getSpaceID() != context->getSpaceID() ||
                               fn.getSpaceType() != _type ||
                               fn.getFileType() != FILE_TYPE_DELTA_LOG ||
                               fn.hasShadowSuffix()))
         {
            PD_LOG(PDERROR, "not target file name:%s", fn.getFileName());
            rc = SDB_INVALIDARG;
            goto error;
         }

         file = SDB_OSS_NEW storageFile();
         if (OSS_UNLIKELY(NULL == file))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = su->openStorageFile(fn, file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open delta log file[%s], rc:%d", fn.getFileName(), rc);
            goto error;
         }

         if (OSS_UNLIKELY(deltaLogFile::PAGE_SIZE != file->getCommonHeadInMem().pageSize ||
                          deltaLogFile::PAGE_COUNT_PER_SEGMENT != file->getCommonHeadInMem().maxPageCountPerSeg ||
                          deltaLogFile::MAX_SEGMENT_COUNT_PER_FILE != file->getCommonHeadInMem().maxSegmentCountPerFile))
         {
            PD_LOG(PDERROR, "invalid core args of log file:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().secretValue !=
             base->getCommonHeadInMem().secretValue)
         {
            PD_LOG(PDERROR, "invalid secret value found in file:%s",
                   file->getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (0 == file->getSegmentCount())
         {
            PD_LOG(PDERROR, "invalid segment count of log file:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         _files.unsortedPushBack(file);
         file = NULL;
      }

      _files.resort();

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(file);
      goto done;
   }

}//namespace vessel
}//namespace engine