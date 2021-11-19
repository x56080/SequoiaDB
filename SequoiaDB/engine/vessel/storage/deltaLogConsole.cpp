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
      _secretValue = base->getCommonHeadInMem().secretValue;
      _base = base->getCommonHeadInMem().sequence;

      rc = initLogFiles(context, fl);
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
      _secretValue = 0;
      _base = 0;
      _file.close();
      _history.clear();
      _validSegmentCount = 0;
      return;
   }

   void deltaLogConsole::destroy(requestContext *context)
   {
      _file.destroy();
      destroyHistoryFiles(context);
      fini();
      return;
   }

   deltaLogFile *deltaLogConsole::getOnlineFile()
   {
      SDB_ASSERT(isReady() && hasOnlineFile(), "can not be invalid");
      return &_file;
   }

   INT32 deltaLogConsole::createOnlineFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      storageUnit *su = NULL;
      vesselFileName fn;
      createStorageFileOptions o;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (hasOnlineFile())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be invalid");
      if (!fn.build(context->getSpaceID(), FILE_TYPE_DELTA_LOG,
                    _type, _base))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = storageCoreArgs(deltaLogFile::PAGE_SIZE,
                               deltaLogFile::PAGE_COUNT_PER_SEGMENT,
                               deltaLogFile::MAX_SEGMENT_COUNT_PER_FILE);
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = _secretValue;

      rc = su->createStorageFile(fn, o, slice(), &_file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }

      rc = _file.removeShadowSuffix();
      if (SDB_OK != rc)
      {
         _file.close();
         PD_LOG(PDERROR, "failed to remove shadow suffix:%d", rc);
         goto error;
      }

      _validSegmentCount = 0;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::initLogFiles(requestContext *context,
                                       const FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be invalid");

      FILE_NAME_LIST::const_iterator itr;
      storageUnit *su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
      const vesselFileName *online = NULL;

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

         if (fn.getSequence() < _base)
         {
            _history.push_back(fn);
            continue;
         }
         else if (fn.getSequence() > _base)
         {
            PD_LOG(PDERROR, "invalid file sequence found[%s]", fn.getFileName());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         else
         {
            online = &fn;
         }
      }

      if (NULL != online)
      {
         rc = su->openStorageFile(*online, &_file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file[%s], rc:%d", online->getFileName(), rc);
            goto error;
         }

         if (deltaLogFile::PAGE_SIZE != _file.getCommonHeadInMem().pageSize ||
             deltaLogFile::PAGE_COUNT_PER_SEGMENT != _file.getCommonHeadInMem().maxPageCountPerSeg ||
             deltaLogFile::MAX_SEGMENT_COUNT_PER_FILE != _file.getCommonHeadInMem().maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "invalid core args of log file:%s", _file.getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (_file.getCommonHeadInMem().secretValue !=
             _secretValue)
         {
            PD_LOG(PDERROR, "invalid secret value found in file:%s",
                   _file.getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         rc = findOnlineFileEnding();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find ending delta log:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      _history.clear();
      goto done;
   }

   void deltaLogConsole::destroyHistoryFiles(requestContext *context)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      storageUnit *su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
      for (FILE_NAME_LIST::const_iterator itr = _history.begin();
           itr != _history.end(); ++itr)
      {
         PD_LOG(PDINFO, "begin to remove history file[%s]", itr->getFileName());
         su->destroyStorageFile(*itr);
      }
      _history.clear();
      return;
   }
   
   INT32 deltaLogConsole::rebase(requestContext *context,
                                 UINT64 base)
   {
      INT32 rc = SDB_OK;
      vesselFileName fn;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            base <= _base))
      {
         SDB_ASSERT(FALSE, "can not be invalid");
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      if (hasOnlineFile())
      {
         fn.extract(strSlice(_file.getCommonHeadInMem().name));
         _file.close();
         _history.push_back(fn);
      }

      destroyHistoryFiles(context);
      _base = base;
      _validSegmentCount = 0;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::findOnlineFileEnding()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(hasOnlineFile(), "can not be invalid");
      
      UINT32 batchChecksum = ossRand();
      INT32 lastValidEnding = -1;

      UINT32 segmentCount = _file.getSegmentCount();
      if (0 == segmentCount)
      {
         _validSegmentCount = 0;
         goto done;
      }

      for (UINT32 i = 0; i < segmentCount; ++i)
      {
         const deltaLogCheckpointRecord *record = NULL;
         UINT32 checksum = 0;
         slice buffer;
         ossValuePtr ptr = 0;
         rc = _file.getSegmentPtr(i, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get segment[%d] ptr:%d", rc);
            goto error;
         }

         buffer.reset(deltaLogFile::FILE_SEGMENT_SIZE, (const void *)ptr);
         checksum = *(buffer.getReadableObjPtr<UINT32>(0));
         record = buffer.getReadableObjPtr<deltaLogCheckpointRecord>
                         (deltaLogFile::FILE_SEGMENT_SIZE - DELTA_LOG_CHECKPOINT_RECORD_SIZE);
         if (checksum != record->checksum)
         {
            PD_LOG(PDWARNING, "invalid checksum found in seg[%d]", i);
            break;
         }

         if (record->isBegin())
         {
            batchChecksum = record->checksum;
         }

         if (batchChecksum != record->checksum)
         {
            PD_LOG(PDWARNING, "different checksum found in seg[%d]", i);
            break;
         }

         if (record->isEnd())
         {
            lastValidEnding = (INT32)i;
         }
      }

      _validSegmentCount = (lastValidEnding < 0) ? 0 : (UINT32)lastValidEnding + 1;
      PD_LOG(PDINFO, "[%d] valid segment found in file:%s",
             _validSegmentCount, _file.getFullPath());
   done:
      return rc;
   error:
      goto done;
   }

   slice deltaLogConsole::getDumpedRecord(UINT32 segmentId,
                                         deltaLogCheckpointRecord *out)const
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(hasOnlineFile(), "can not be invalid");
      SDB_ASSERT(segmentId < _validSegmentCount, "out of bound");
      ossValuePtr ptr = 0;
      _file.getSegmentPtr(segmentId, ptr);
      slice buffer(deltaLogFile::FILE_SEGMENT_SIZE, (const void *)ptr);
      SDB_ASSERT(buffer.isValid(), "impossible");
      const deltaLogCheckpointRecord *record =
                         buffer.getReadableObjPtr<deltaLogCheckpointRecord>
                         (deltaLogFile::FILE_SEGMENT_SIZE - DELTA_LOG_CHECKPOINT_RECORD_SIZE);
      SDB_ASSERT(NULL != record, "can not be null");

      if (NULL != out)
      {
         *out = *record;
      }
      return buffer.getReadableSlice(sizeof(UINT32),
                                     deltaLogFile::FILE_SEGMENT_SIZE -
                                     DELTA_LOG_CHECKPOINT_RECORD_SIZE -
                                     sizeof(UINT32));
   }

   INT32 deltaLogConsole::append(requestContext *context,
                                 const ossPoolVector<memoryBlock> &buffers,
                                 const LPS_CHECKPOINT &checkpoint)
   {
      INT32 rc = SDB_OK;
      deltaLogCheckpointRecord cr;

      if (OSS_UNLIKELY(NULL == context ||
                       !checkpoint.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (!hasOnlineFile())
      {
         rc = createOnlineFile(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create online file:%d", rc);
            goto error;
         }
      }

      rc = _file.ensureSegmentCount(_validSegmentCount + buffers.size());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure segment count:%d", rc);
         goto error;
      }

      cr.checkpoint = checkpoint;
      cr.checksum = ossRand();

      for (UINT32 i = 0; i < buffers.size(); ++i)
      {
         deltaLogCheckpointRecord *tail = NULL;
         SDB_ASSERT(buffers[i].getSize() < deltaLogFile::FILE_SEGMENT_SIZE, "impossible");
         SDB_ASSERT(0 != buffers[i].getSize(), "can not be empty");
         SDB_ASSERT(0 == buffers[i].getSize() % DELTA_LOG_DUMP_RECORD_SIZE, "must be aligned");
         const memoryBlock &mb = buffers[i];

         cr.elementCount = mb.getSize() / DELTA_LOG_DUMP_RECORD_SIZE;
         cr.flags = 0;
         if (0 == i)
         {
            cr.setBegin();
         }
         /// not else if         
         if ((i + 1) == buffers.size())
         {
            cr.setEnd();
         }

         slice buffer;
         ossValuePtr ptr = 0;
         rc = _file.getSegmentPtr(i + _validSegmentCount, ptr);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to get segment ptr:%d", rc);
            goto error;
         }

         buffer.makeWritable(deltaLogFile::FILE_SEGMENT_SIZE, (void *)ptr);
         buffer.write(0, sizeof(UINT32), &(cr.checksum));
         buffer.write(sizeof(UINT32), mb.getSize(), mb.getBuffer());
         tail = buffer.getWritableObjPtr<deltaLogCheckpointRecord>
                         (deltaLogFile::FILE_SEGMENT_SIZE - DELTA_LOG_CHECKPOINT_RECORD_SIZE);
         *tail = cr;
      }

      _file.fsync();
      _validSegmentCount += buffers.size();
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine