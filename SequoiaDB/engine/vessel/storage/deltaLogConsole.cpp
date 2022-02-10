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
#include "vessel/memoryBlock.h"

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
      _baseSequence = base->getCommonHeadInMem().sequence;

      rc = load(context, fl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load delta log files:%d", rc);
         goto error;
      }

      rc = resumeToLastCheckpoint();
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
      _type = INVALID_SPACE_TYPE;
      _secretValue = 0;
      _baseSequence = 0;
      _workingFile.close();
      _buffer.release();
      _writingPid = INVALID_PAGE_ID;
      _page = NULL;
      _lastCheckpoint = LPS_CHECKPOINT();
      _lastCheckpointPid = INVALID_PAGE_ID;
      _history.clear();
      _checkpointReserved = INVALID_PAGE_ID;
      return;
   }

   INT32 deltaLogConsole::createNewFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!_workingFile.isOpen(), "do not reopen");
      storageUnit *su = NULL;
      vesselFileName fn;
      createStorageFileOptions o;


      su = context->getEnv()->dms.getStorageUnit(context->getSpaceID());
      SDB_ASSERT(NULL != su, "can not be invalid");
      if (!fn.build(context->getSpaceID(), FILE_TYPE_DELTA_LOG,
                    _type, _baseSequence))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = storageCoreArgs(DELTA_LOG_FILE_PAGE_SIZE,
                               DELTA_LOG_FILE_PAGE_COUNT_PER_SEG,
                               DELTA_LOG_FILE_SEG_COUNT_PER_FILE);
      o.createAsTmpFile = FALSE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = _secretValue;

      rc = su->createStorageFile(fn, o, slice(), &_workingFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::load(requestContext *context,
                               const FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be invalid");

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

         if (fn.getSequence() != _baseSequence)
         {
            PD_LOG(PDERROR, "unmatched file found:%s", fn.getFileName());
            _history.push_back(fn);
            continue;
         }
         else if (_workingFile.isOpen())
         {
            PD_LOG(PDERROR, "duplidated working file found[%s]", fn.getFileName());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = su->openStorageFile(fn, &_workingFile);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open delta log file[%s], rc:%d",
                   fn.getFileName(), rc);
            goto error;
         }

         if (DELTA_LOG_FILE_PAGE_SIZE != _workingFile.getCommonHeadInMem().pageSize ||
             DELTA_LOG_FILE_PAGE_COUNT_PER_SEG != _workingFile.getCommonHeadInMem().maxPageCountPerSeg ||
             DELTA_LOG_FILE_SEG_COUNT_PER_FILE != _workingFile.getCommonHeadInMem().maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "invalid core args of log file:%s", _workingFile.getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (_workingFile.getCommonHeadInMem().secretValue !=
             _secretValue)
         {
            PD_LOG(PDERROR, "invalid secret value found in file:%s",
                   _workingFile.getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }

      destroyHistoryFiles(context);
   done:
      return rc;
   error:
      _workingFile.close();
      _history.clear();
      goto done;
   }

   void deltaLogConsole::destroy(requestContext *context)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      destroyHistoryFiles(context);
      if (_workingFile.isOpen())
      {
         _workingFile.destroy();
      }
      fini();
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
   
   void deltaLogConsole::rebase(requestContext *context,
                                 UINT64 base,
                                 BOOLEAN destroyHistoryFileAtOnce)
   {
      vesselFileName fn;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_baseSequence <= base, "invalid base sequence");
      
      _baseSequence = base;

      if (_workingFile.isOpen())
      {
         strSlice nameSlice(_workingFile.getCommonHeadInMem().name);
         fn.extract(nameSlice);
         SDB_ASSERT(fn.isValid(), "must be valid");
         _history.push_back(fn);
         _workingFile.close();
      }

      _writingPid = INVALID_PAGE_ID;
      _page = NULL;
      _lastCheckpoint = LPS_CHECKPOINT();
      _lastCheckpointPid = INVALID_PAGE_ID;
      _checkpointReserved = INVALID_PAGE_ID;

      if (destroyHistoryFileAtOnce)
      {
         destroyHistoryFiles(context);
      }

      return;
   }

   INT32 deltaLogConsole::append(requestContext *context,
                                  const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context ||
                       !dlr.isValid() ||
                       DELTA_LOG_TYPE_CHECKPOINT == dlr.getLogType() ||
                       deltaLogFilePage::getDataCapacity() < dlr.getLogSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _append(context, dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append new log record:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::_append(requestContext *context,
                                  const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(dlr.isValid(), "can not be invalid");
      SDB_ASSERT(dlr.getLogSize() <= deltaLogFilePage::getDataCapacity(), "out of resource");
      UINT32 size = dlr.getLogSize();

      do
      {
         rc = ensureFileAndBuffer(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure file or buffer:%d", rc);
            goto error;
         }

         if (deltaLogFilePage::getDataCapacity() < (_page->dataOffset + size))
         {
            flushBufferAndShiftWritingPid();
            /// ensure file space again, do not break.
         }
         else
         {
            break;
         }
      } while (TRUE);

      ossMemcpy(_page->data + _page->dataOffset, dlr.getLogHead(), size);
      if (DELTA_LOG_TYPE_CHECKPOINT == dlr.getLogType())
      {
         _page->checkpointOffset = (INT32)(_page->dataOffset);
      }
      _page->dataOffset += size;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::reserveCheckpoint(requestContext *context)
   {
      INT32 rc = SDB_OK;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

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

      builder.buildCheckpointLog(LPS_CHECKPOINT());
      dlr = builder.getDeltaLogRecord();

      rc = _append(context, dlr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append dummy checkpoint log record:%d", rc);
         goto error;
      }

      _checkpointReserved = _writingPid;
      /// always shift writing pid at once
      flushBufferAndShiftWritingPid();
   done:
      return rc;
   error:
      goto done;
   }

   void deltaLogConsole::commit(const LPS_CHECKPOINT &checkpoint)
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(checkpoint.isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != _checkpointReserved, "can not be invalid");
      SDB_ASSERT(_workingFile.isOpen(), "must be open");
      deltaLogFilePage *page = NULL;
      deltaLogRecordBuilder builder;
      builder.buildCheckpointLog(checkpoint);
      deltaLogRecord dlr = builder.getDeltaLogRecord();
      ossValuePtr ptr = 0;
      INT32 rc = _workingFile.getPagePtr(_checkpointReserved, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", _checkpointReserved, rc);
         ossPanic();
      }

      page = (deltaLogFilePage *)ptr;
      if (!page->hasCheckpoint())
      {
         PD_LOG(PDERROR, "page[%d] does not have checkpoint", _checkpointReserved);
         ossPanic();
      }

      ossMemcpy(page->data + page->checkpointOffset,
                dlr.getLogHead(), dlr.getLogSize());

      fsyncDirtyPages();
      
      _lastCheckpoint = checkpoint;
      _lastCheckpointPid = _checkpointReserved;
      _checkpointReserved = INVALID_PAGE_ID;
      return;
   }

   void deltaLogConsole::flushBufferAndShiftWritingPid()
   {
      SDB_ASSERT(_workingFile.isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != _writingPid, "can not be invalid");
      SDB_ASSERT(NULL != _page && _page->isValid(), "can not be null");

      ossValuePtr ptr = 0;
      _workingFile.getPagePtr(_writingPid, ptr);
      SDB_ASSERT(0 != ptr, "impossible");

      ossMemcpy((void *)ptr, _page, DELTA_LOG_FILE_PAGE_SIZE);
      ++_writingPid;
      _page->init();
      return;
   }

   INT32 deltaLogConsole::ensureFileAndBuffer(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 minSegmentCount = 0;

      if (!_workingFile.isOpen())
      {
         rc = createNewFile(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create working file:%d", rc);
            goto error;
         }

         _writingPid = 0;
      }

      SDB_ASSERT(ossIsPowerOf2(DELTA_LOG_FILE_PAGE_COUNT_PER_SEG), "must be power of 2");
      minSegmentCount = ossAlignX(_writingPid + 1, DELTA_LOG_FILE_PAGE_COUNT_PER_SEG) /
                                  DELTA_LOG_FILE_PAGE_COUNT_PER_SEG;
      rc = _workingFile.ensureSegmentCount(minSegmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure file segment space:%d", rc);
         goto error;
      }

      if (0 == _buffer.getCapacity())
      {
         rc = _buffer.reserve(DELTA_LOG_FILE_PAGE_SIZE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve buffer size:%d", rc);
            goto error;
         }
      }

      if (NULL == _page)
      {
          _page = (deltaLogFilePage *)(_buffer.getBuffer());
         _page->init();
      }

   done:
      return rc;
   error:
      goto done;
   }

   UINT64 deltaLogConsole::getDeltaLogSize()const
   {
      return INVALID_PAGE_ID == _writingPid ?
             0 : ((UINT64)DELTA_LOG_FILE_PAGE_SIZE * (_writingPid + 1));
   }
   
   INT32 deltaLogConsole::findLastCheckpointPid(const storageFile &file,
                                                PAGE_ID &pid)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(file.isOpen(), "must be open");
      pid = INVALID_PAGE_ID;

      for (UINT32 i = 0; i < file.getSegmentCount(); ++i)
      {
         for (UINT32 j = 0; j < DELTA_LOG_FILE_PAGE_COUNT_PER_SEG; ++j)
         {
            PAGE_ID pidToScan = (i * DELTA_LOG_FILE_PAGE_COUNT_PER_SEG) + j;
            ossValuePtr ptr = 0;
            const deltaLogFilePage *page = NULL;

            rc = file.getPagePtr(pidToScan, ptr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get page[%d] ptr:%d in file[%s]",
                      pidToScan, rc, file.getFullPath());
               goto error;
            }

            page = (const deltaLogFilePage *)ptr;
            if (!page->isValid())
            {
               /// ignore all data after the first crashed page,
               /// even still non-crashed pages on there.
               goto done;
            }
            else if (page->hasCheckpoint())
            {
               deltaLogRecord dlr;
               dlr.reset(page->data + page->checkpointOffset);
               const LPS_CHECKPOINT *checkpoint = NULL;
               if (!dlr.isValid())
               {
                  PD_LOG(PDERROR, "checkpoint found at page[%d], but not invalid", pidToScan);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }

               checkpoint = dlr.getRecordBodyPtr<LPS_CHECKPOINT>(0);
               if (NULL == checkpoint)
               {
                  PD_LOG(PDERROR, "checkpoint found at page[%d], but failed to parse", pidToScan);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }

               /// if checkpoint is not valid, may be reserved but not commit
               if (checkpoint->isValid())
               {
                  pid = pidToScan;
               }
            }
            else
            {
               /// do nothing
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 deltaLogConsole::resumeToLastCheckpoint()
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;

      if (!_workingFile.isOpen())
      {
         goto done;
      }
   
      rc = findLastCheckpointPid(_workingFile, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find last checkpoint page in file:%s, rc:%d",
                _workingFile.getFullPath(), rc);
         goto error;
      }

      if (INVALID_PAGE_ID == pid)
      {
         _writingPid = 0;
         goto done;
      }
      else
      {
         deltaLogRecord dlr;
         const deltaLogFilePage *page = NULL;
         ossValuePtr ptr = 0;
         _workingFile.getPagePtr(pid, ptr);
         page = (const deltaLogFilePage *)ptr;
         SDB_ASSERT(page->isValid() && page->hasCheckpoint(), "impossible");
         dlr.reset((const CHAR *)(page->data) + page->checkpointOffset);
         const LPS_CHECKPOINT *checkpoint = dlr.getRecordBodyPtr<LPS_CHECKPOINT>(0);
         _lastCheckpoint = *checkpoint;
         _lastCheckpointPid = pid;
         _writingPid = pid + 1; /// move to the next page
      }
   done:
      return rc;
   error:
      goto done;
   }

   void deltaLogConsole::fsyncDirtyPages()const
   {
      SDB_ASSERT(INVALID_PAGE_ID != _checkpointReserved, "can not be invalid");
      SDB_ASSERT(_workingFile.isOpen(), "can not be closed");
      UINT32 pageCount = _workingFile.getCommonHeadInMem().maxPageCountPerSeg;
      UINT32 minSegment = 0;
      UINT32 maxSegment = 0;
      INT32 rc = SDB_OK;

      if (INVALID_PAGE_ID != _lastCheckpointPid)
      {
         minSegment = _lastCheckpointPid / pageCount;
      }

      maxSegment = _checkpointReserved / pageCount;

      SDB_ASSERT(minSegment <= maxSegment, "impossible");
      for (UINT32 i = minSegment; i < maxSegment; ++i)
      {
         rc = _workingFile.fsyncSegment(i);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDSEVERE, "failed to fsync delta log segment[%d], rc:%d", i, rc);
         }
      }

      rc = _workingFile.fsyncPagesInSeg(maxSegment, (_checkpointReserved % pageCount) + 1);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDSEVERE, "failed to fsync delta log segment[%d], rc:%d", maxSegment, rc);
      }
      return;
   }
}//namespace vessel
}//namespace engine