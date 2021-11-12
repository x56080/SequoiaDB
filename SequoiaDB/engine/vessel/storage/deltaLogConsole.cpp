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
#include "vessel/storageFileCreater.h"
#include "vessel/storageUtils.h"
#include "vessel/deltaLogUtils.h"

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

   INT32 deltaLogConsole::init(const storageFileCreater *creater)
   {
      INT32 rc = SDB_OK;
      fini();
      if (OSS_UNLIKELY(NULL == creater ||
                       !creater->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _creater = creater;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::init(const storageFileCreater *creater,
                               const FILE_NAME_LIST *fl,
                               UINT64 beginOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _creater, "do not reinit");
      fini();

      if (OSS_UNLIKELY(NULL == creater ||
                       !creater->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _creater = creater;
      rc = initLogFiles(fl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log files:%d", rc);
         goto error;
      }
      rc = restoreToLastCheckpoint(fl, beginOffset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to restore to last checkpoint:%d", rc);
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
      _creater = NULL;
      _logFiles.close();
      _nextCheckpointOffset = DPS_INVALID_LSN_OFFSET;
      _checkpoint = LPS_CHECKPOINT();
      _nextRecordOffset = 0;
      _minDirtyOffset = 0;
      _fileWriteOffset = 0;
      if (NULL != _logBuffer)
      {
         SDB_THREAD_FREE(_logBuffer);
         _logBuffer = NULL;
      }
      _logBufferWriteSize = 0;
      return;
   }

   void deltaLogConsole::destroy()
   {
      _logFiles.destroy();
      fini();
      return;
   }

   INT32 deltaLogConsole::restoreToLastCheckpoint(const FILE_NAME_LIST *fl,
                                                  UINT64 beginOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != beginOffset, "can not be invalid");
      SDB_ASSERT(0 == _nextRecordOffset, "must be zero");
      SDB_ASSERT(!_checkpoint.isValid(), "must be invalid");
      
      deltaLogRecord dlr;
      BOOLEAN checkpointFound = FALSE;
      LPS_CHECKPOINT checkpoint;

      rc = findLastCheckpoint(beginOffset, checkpointFound, checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find the last checkpoint:%d", rc);
         goto error;
      }

      if (checkpointFound)
      {
         _nextRecordOffset = checkpoint.offset;
         _nextRecordOffset += DELTA_LOG_RECORD_HEAD_SIZE
                              + LPS_CHECKPOINT_SIZE
                              + deltaLogFileDef::CHECKSUM_SIZE;
         _minDirtyOffset = _nextRecordOffset;
         _fileWriteOffset = _nextRecordOffset;
         _checkpoint = checkpoint;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::initReaderBeforeAddingNewRecord(UINT64 beginOffset,
                                                          deltaLogScanner &reader)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_nextRecordOffset <= beginOffset)
      {
         rc = SDB_DPS_LSN_OUTOFRANGE;
         goto error;
      }

      rc = reader.init(&_logFiles, beginOffset, _nextRecordOffset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log reader:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::reserveNextCheckpoint()
   {
      INT32 rc = SDB_OK;
      deltaLogRecordBuilder builder;
      UINT64 offset = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET != _nextCheckpointOffset))
      {
         PD_LOG(PDERROR, "checkpoint has already been reserved");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }


      rc = builder.buildDummyLog(LPS_CHECKPOINT_SIZE + DELTA_LOG_RECORD_HEAD_SIZE);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build dummy log record:%d", rc);
         goto error;
      }

      rc = _append(builder.getDeltaLogRecord(), offset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append dummy record:%d", rc);
         goto error;
      }

      rc = copyDataFromBufferToFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy data to file:%d", rc);
         goto error;
      }

      _nextCheckpointOffset = offset;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::commitCheckpoint(const checkpointLSN &lsn)
   {
      INT32 rc = SDB_OK;
      storageFile *file = NULL;
      ossValuePtr ptr = 0;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;
      UINT64 fileId = 0;
      UINT32 segmentId = 0;
      UINT32 offsetInSegment = 0;
      DELTA_LOG_CHECKSUM checksum = 0;
      LPS_CHECKPOINT checkpoint;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!lsn.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DPS_INVALID_LSN_OFFSET == _nextCheckpointOffset))
      {
         PD_LOG(PDERROR, "reserve checkpoint first before committing");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      checkpoint.init(0, lsn, _nextCheckpointOffset, _checkpoint.offset);
      rc = builder.buildCheckpointLog(checkpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build checkpoint log record:%d", rc);
         goto error;
      }

      dlr = builder.getDeltaLogRecord();
      checksum = createDeltaLogRecordChecksum(dlr);

      fileId = deltaLogFileDef::getLogFileSequenceByOffset(_nextCheckpointOffset);
      segmentId = deltaLogFileDef::getSegmentIdInFileByOffset(_nextCheckpointOffset);
      offsetInSegment = deltaLogFileDef::getOffsetInSegmentByOffset(_nextCheckpointOffset);
      SDB_ASSERT((offsetInSegment + dlr.getLogHead()->_size + deltaLogFileDef::CHECKSUM_SIZE) <=
                  deltaLogFileDef::FILE_SEGMENT_SIZE, "impossible");

      file = _logFiles.findFromBackToFront(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to get log file[%lld], rc:%d", fileId, rc);
         goto error;
      }

      rc = file->getSegmentPtr(segmentId, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get segment ptr[%lld, %d], rc:%d",
                fileId, segmentId, rc);
         goto error;
      }

      ossMemcpy((void *)(ptr + offsetInSegment),
                dlr.getLogHead(), dlr.getLogHead()->_size);
      *((DELTA_LOG_CHECKSUM *)(ptr + offsetInSegment + dlr.getLogHead()->_size)) = checksum;

      rc = fsyncDeltaLog(_nextCheckpointOffset +
                         dlr.getLogHead()->_size +
                         deltaLogFileDef::CHECKSUM_SIZE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to fsync log files:%d", rc);
         goto error;
      }

      _checkpoint = checkpoint;
      _nextCheckpointOffset = DPS_INVALID_LSN_OFFSET;
   done:
      return rc;
   error:
      /// clear offset reserved, waiting for next creating.
      _nextCheckpointOffset = DPS_INVALID_LSN_OFFSET;
      goto done;
   }

   UINT64 deltaLogConsole::getFuzzyDirtyLogSize()const
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      UINT64 dirtyOffset = ((const ossAtomic64 *)(&_minDirtyOffset))->peek();
      UINT64 nextRecordOffset = ((const ossAtomic64 *)(&_nextRecordOffset))->peek();
      return (dirtyOffset <= nextRecordOffset) ?
             (nextRecordOffset - dirtyOffset) : 0;
   }

   INT32 deltaLogConsole::tryToDestroyHistroyFiles(UINT64 offset)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (offset < _minDirtyOffset)
      {
         _logFiles.destroyIfLess(offset / deltaLogFileDef::MAX_FILE_SIZE);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::append(const deltaLogRecord &dlr, UINT64 *offset)
   {
      INT32 rc = SDB_OK;
      UINT64 recordOffset = DPS_INVALID_LSN_OFFSET;
      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!dlr.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(DELTA_LOG_TYPE_CHECKPOINT == dlr.getLogHead()->_type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _append(dlr, recordOffset);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (NULL != offset)
      {
         *offset = recordOffset;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::_append(const deltaLogRecord &dlr, UINT64 &offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(dlr.isValid(), "can not be invalid");

      UINT32 sizeNeeded = 0;
      UINT32 currentOffsetInSegment = 0;
      UINT32 currentSegmentFreeSize = 0;
      DELTA_LOG_CHECKSUM checksum = 0;

      if (!isBufferReady())
      {
         rc = initLogBuffer();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init log buffer:%d", rc);
            goto error;
         }
      }

      sizeNeeded = dlr.getLogHead()->_size + deltaLogFileDef::CHECKSUM_SIZE;
      currentOffsetInSegment = deltaLogFileDef::getOffsetInSegmentByOffset(_nextRecordOffset);
      currentSegmentFreeSize = deltaLogFileDef::FILE_SEGMENT_SIZE - currentOffsetInSegment;

      /// If free size of current segment is not enough, switch to next.
      if (currentSegmentFreeSize < sizeNeeded)
      {
         rc = copyDataFromBufferToFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy data to file:%d", rc);
            goto error;
         }

         if (deltaLogFileDef::MIN_RECORD_SIZE_ON_DISK <= currentSegmentFreeSize)
         {
            rc = appendDummyLogToSegment();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to append dummy log:%d", rc);
               goto error;
            }
         }

         _nextRecordOffset += currentSegmentFreeSize;
         _fileWriteOffset = _nextRecordOffset;
      }

      if (0 == (_nextRecordOffset & (deltaLogFileDef::FILE_SEGMENT_SIZE - 1)))
      {
         UINT64 fileId = deltaLogFileDef::getLogFileSequenceByOffset(_nextRecordOffset);
         UINT32 segmentId = deltaLogFileDef::getSegmentIdInFileByOffset(_nextRecordOffset);
         rc = ensureFileSpace(fileId, segmentId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure file space[%lld,%d], rc:%d",
                   fileId, segmentId, rc);
            goto error;
         }
      }

      checksum = createDeltaLogRecordChecksum(dlr);
      rc = writeLogBuffer(dlr, checksum);
      if (SDB_OK != rc)
      {
         SDB_ASSERT(FALSE, "impossible");
         PD_LOG(PDERROR, "failed to write new log record to buffer:%d", rc);
         goto error;
      }

      offset = _nextRecordOffset;
      _nextRecordOffset += sizeNeeded;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::createNewLogFile(UINT64 fileId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _creater, "can not be null");
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != fileId, "can not be invalid");
      storageFile *file = NULL;
      storageCoreArgs args(deltaLogFileDef::PAGE_SIZE,
                           deltaLogFileDef::PAGE_COUNT_PER_SEGMENT,
                           deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE);

      file = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _creater->createTmpFile(FILE_TYPE_DELTA_LOG,
                                   fileId,
                                   args,
                                   file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp delta log file:%d", rc);
         goto error;
      }

      rc = file->allocateNewSegment();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file[%s], rc:%d", file->getFullPath(), rc);
         goto error;
      }

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush file[%s] head segment:%d", file->getFullPath(), rc);
         goto error;
      }

      rc = renameToFormalAndReopen(_creater->getDirSlice(),
                                   TRUE, FILE_SHADOW_SUFFIX_TMP,
                                   file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename to formal file, rc:%d", rc);
         goto error;
      }

      rc = _logFiles.pushBack(file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert new log file to list, sequence:%lld, rc:%d",
                fileId, rc);
         goto error;
      }

      file = NULL;
      
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

   INT32 deltaLogConsole::ensureFileSpace(UINT64 fileId,
                                          UINT32 segmentId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(STORAGE_FILE_INVALID_SEQUENCE != fileId, "can not be invalid");
      SDB_ASSERT(segmentId < deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE, "impossible");

      storageFile *file = _logFiles.findFromBackToFront(fileId);
      if (NULL != file)
      {
         /// Do not use "allocateNewSegment" to extend log file.
         /// We never rollback file's segment.
         rc = file->ensureSegmentCount(segmentId + 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure file segment count:%d", rc);
            goto error;
         }
      }
      else if (0 != segmentId)
      {
         PD_LOG(PDERROR, "segmentId[%d] is not zero but file[%lld] not found",
                segmentId, fileId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         rc = createNewLogFile(fileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new file[%lld], rc:%d", fileId, rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::copyDataFromBufferToFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isBufferReady(), "must be ready");
      UINT64 fileId = 0;
      UINT32 segmentId = 0;
      storageFile *file = NULL;
      ossValuePtr segmentPtr = 0;
      UINT32 offsetInSegment = 0;

      if (0 == _logBufferWriteSize)
      {
         goto done;
      }

      fileId = deltaLogFileDef::getLogFileSequenceByOffset(_fileWriteOffset);
      segmentId = deltaLogFileDef::getSegmentIdInFileByOffset(_fileWriteOffset);
      offsetInSegment = deltaLogFileDef::getOffsetInSegmentByOffset(_fileWriteOffset);

      if (OSS_UNLIKELY(deltaLogFileDef::FILE_SEGMENT_SIZE <
                       (offsetInSegment + _logBufferWriteSize)))
      {
         PD_LOG(PDSEVERE, "log buffer write size out of range,"
                          "file write offset[%lld], log buffer write size[%d",
                          _fileWriteOffset, _logBufferWriteSize);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = _logFiles.findFromBackToFront(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to get file[%lld]", fileId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = file->getSegmentPtr(segmentId, segmentPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get segment ptr of[%lld, %d], rc:%d",
                fileId, segmentId, rc);
         goto error;
      }

      ossMemcpy((void *)(segmentPtr + offsetInSegment), _logBuffer, _logBufferWriteSize);
      _fileWriteOffset += _logBufferWriteSize;
      _logBufferWriteSize = 0;
   done:
      return rc;
   error:
      PD_LOG(PDERROR, "failed to copy data to file[%lld], rc:%d", _fileWriteOffset, rc);
      goto done;
   }

   INT32 deltaLogConsole::appendDummyLogToSegment()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(_fileWriteOffset == _nextRecordOffset, "must be same");
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;
      DELTA_LOG_CHECKSUM checksum = 0;
      storageFile *file = NULL;
      ossValuePtr ptr = 0;
      UINT64 fileId = deltaLogFileDef::getLogFileSequenceByOffset(_nextRecordOffset);
      UINT32 segmentId = deltaLogFileDef::getSegmentIdInFileByOffset(_nextRecordOffset);
      UINT32 segmentOffset = deltaLogFileDef::getOffsetInSegmentByOffset(_nextRecordOffset);
      UINT32 remainSize = deltaLogFileDef::FILE_SEGMENT_SIZE - segmentOffset;

      SDB_ASSERT(deltaLogFileDef::MIN_RECORD_SIZE_ON_DISK <= remainSize, "impossible");
      SDB_ASSERT(remainSize < (MAX_DELTA_LOG_RECORD_SIZE + deltaLogFileDef::CHECKSUM_SIZE),
                 "impossible");

      file = _logFiles.findFromBackToFront(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to get file with sequence[%lld]", fileId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = file->getSegmentPtr(segmentId, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get segment[%lld, %d] ptr", fileId, segmentId);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = builder.buildDummyLog(remainSize - deltaLogFileDef::CHECKSUM_SIZE);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to build dummy log");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      dlr = builder.getDeltaLogRecord();
      checksum = createDeltaLogRecordChecksum(dlr);
      ossMemcpy((void *)(ptr + segmentOffset), dlr.getLogHead(), dlr.getLogHead()->_size);
      *((DELTA_LOG_CHECKSUM *)(ptr + segmentOffset + dlr.getLogHead()->_size)) = checksum;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::initLogFiles(const FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _creater, "can not be null");
      SDB_ASSERT(_creater->isValid(), "cann ot be invalid");
      SDB_ASSERT(_logFiles.isEmpty(FALSE), "must be empty");

      storageFile *file = NULL;
      FILE_NAME_LIST::const_iterator itr;
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
         else if (OSS_UNLIKELY(fn.getSpaceID() != _creater->getSpaceID() ||
                               fn.getSpaceType() != _creater->getSpaceType() ||
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

         rc = file->open(_creater->getDirSlice(), fn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open delta log file[%s], rc:%d", fn.getFileName(), rc);
            goto error;
         }

         if (OSS_UNLIKELY(deltaLogFileDef::PAGE_SIZE != file->getCommonHeadInMem().pageSize ||
                          deltaLogFileDef::PAGE_COUNT_PER_SEGMENT != file->getCommonHeadInMem().maxPageCountPerSeg ||
                          deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE != file->getCommonHeadInMem().maxSegmentCountPerFile))
         {
            PD_LOG(PDERROR, "invalid core args of log file:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().secretValue != _creater->getSecretValue())
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

         _logFiles.unsortedPushBack(file);
         file = NULL;
      }

      _logFiles.resort();

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(file);
      goto done;
   }

   INT32 deltaLogConsole::findLastCheckpoint(UINT64 beginOffset,
                                             BOOLEAN &found,
                                             LPS_CHECKPOINT &checkpoint)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != beginOffset, "can not be invalid");
      deltaLogScanner reader;
      LPS_CHECKPOINT lastFound;
      found = FALSE;

      if (_logFiles.isEmpty(FALSE))
      {
         goto done;
      }

      rc = reader.init(&_logFiles, beginOffset);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init log reader:%d", rc);
         goto error;
      }

      do
      {
         UINT64 offset = 0;
         deltaLogRecord dlr;
         rc = reader.getNext(dlr, &offset);
         if (SDB_OK == rc)
         {
            if (DELTA_LOG_TYPE_CHECKPOINT == dlr.getLogHead()->_type)
            {
               const LPS_CHECKPOINT *tmp = (const LPS_CHECKPOINT *)
                                           (dlr.getRecordBodyPtr(0, LPS_CHECKPOINT_SIZE));
               if (NULL == tmp)
               {
                  PD_LOG(PDERROR, "valid log record found at [%lld] but failed to get ptr", offset);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }
               else if (offset != tmp->offset)
               {
                  PD_LOG(PDERROR, "record offset[%lld] does not match checkpoint offset[%lld]",
                         offset, tmp->offset);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }
               else
               {
                  lastFound = *tmp;
               }
            }
            continue;
         }
         else if (SDB_VESSEL_EOC == rc)
         {
            rc = SDB_OK;
            break;
         }
         else
         {
            PD_LOG(PDERROR, "failed to get next record from reader:%d", rc);
            goto error;
         }
      } while (TRUE);
      
      if (lastFound.isValid())
      {
         found = TRUE;
         checkpoint = lastFound;
      }
      
   done:
      return rc;
   error:
      goto done;
   }


   INT32 deltaLogConsole::initLogBuffer()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL == _logBuffer, "must be null");
      SDB_ASSERT(ossIsPowerOf2(DEFAULT_LOG_BUFFER_SIZE), "msut be power of 2");

      _logBuffer = (CHAR *)SDB_THREAD_ALLOC(DEFAULT_LOG_BUFFER_SIZE);
      if (NULL == _logBuffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _logBufferWriteSize = 0;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::writeLogBuffer(const deltaLogRecord &dlr,
                                         DELTA_LOG_CHECKSUM checksum)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dlr.isValid(), "must be valid");
      UINT32 size = dlr.getLogHead()->_size + deltaLogFileDef::CHECKSUM_SIZE;
      SDB_ASSERT(size < DEFAULT_LOG_BUFFER_SIZE, "impossible");
      UINT32 freeSize = DEFAULT_LOG_BUFFER_SIZE - _logBufferWriteSize;

      if (!isBufferReady())
      {
         rc = initLogBuffer();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init log buffer:%d", rc);
            goto error;
         }
      }
      else if (freeSize < size)
      {
         rc = copyDataFromBufferToFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy data to file:%d", rc);
            goto error;
         }
      }

      ossMemcpy((void *)((ossValuePtr)_logBuffer + _logBufferWriteSize),
                dlr.getLogHead(), dlr.getLogHead()->_size);
      *((DELTA_LOG_CHECKSUM *)((ossValuePtr)_logBuffer +
                               _logBufferWriteSize +
                               dlr.getLogHead()->_size)) = checksum;
      
      _logBufferWriteSize += size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::fsyncDeltaLog(UINT64 upperOffset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < upperOffset, "can not be zero");
      SDB_ASSERT(_minDirtyOffset < upperOffset, "impossible");
      SDB_ASSERT(upperOffset <= _nextRecordOffset, "impossible");
      SDB_ASSERT(upperOffset <= _fileWriteOffset, "impossible");
      UINT64 minGlobalSegmentId = _minDirtyOffset / deltaLogFileDef::FILE_SEGMENT_SIZE;
      UINT64 maxGlobalSegmentId = (upperOffset - 1) /
                                  deltaLogFileDef::FILE_SEGMENT_SIZE;

      SDB_ASSERT(ossIsPowerOf2(deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE),
                 "must be power of 2");

      for (UINT64 i = minGlobalSegmentId; i <= maxGlobalSegmentId; ++i)
      {
         UINT64 fileId = i / deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE;
         UINT32 segmentId = (i & (deltaLogFileDef::MAX_SEGMENT_COUNT_PER_FILE - 1));
         storageFile *file = _logFiles.findFromFrontToBack(fileId);
         if (NULL == file)
         {
            PD_LOG(PDERROR, "failed to get file[%lld] to fsync", fileId);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = file->fsyncSegment(segmentId, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush segment[%lld, %d], rc:%d",
                   fileId, segmentId, rc);
            goto error;
         }
      }

      _minDirtyOffset = upperOffset;
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine