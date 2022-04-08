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
#include "vessel/memoryBlock.h"
#include "vessel/threadContext.h"
#include "vessel/storageFileMaintainer.h"
#include "vessel/storageFileLoader.h"

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

   INT32 deltaLogConsole::init(SPACE_ID sid,
                               SPACE_TYPE type,
                               UINT32 secretValue,
                               UINT32 base,
                               const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      
      fini();
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _manifest.sid = sid;
      _manifest.ftype = FILE_TYPE_DELTA_LOG;
      _manifest.stype = type;
      _manifest.secretValue = secretValue;
      _manifest.args.pageSize = DELTA_LOG_FILE_PAGE_SIZE;
      _manifest.args.maxPageCountPerSeg = DELTA_LOG_FILE_PAGE_COUNT_PER_SEG;
      _manifest.args.maxSegmentCountPerFile = DELTA_LOG_FILE_SEG_COUNT_PER_FILE;
      SDB_ASSERT(_manifest.isValid(), "must be valid");
      _baseSequence = base;

      if (nullptr != loader)
      {
         const STORAGE_FILE_NAME_LIST *fl =
                loader->getFileList(_manifest.stype, _manifest.ftype);
         if (nullptr != fl)
         {  
            rc = load(fl);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to load delta log files:%d", rc);
               goto error;
            }
         }

         if (nullptr != _workingFile)
         {
            rc = resumeToLastCheckpoint();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to restore to last checkpoint:%d", rc);
               goto error;
            }
         }
      }
      
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void deltaLogConsole::fini()
   {
      _manifest.reset();
      _baseSequence = 0;
      _prechecksum = 0;
      if (nullptr != _workingFile)
      {
         _workingFile->close();
         SDB_OSS_DEL _workingFile;
         _workingFile = nullptr;
      }
      _buffer.release();
      _writingPid = INVALID_PAGE_ID;
      _page = nullptr;
      _lastCheckpoint = LPS_CHECKPOINT();
      _lastCheckpointPid = INVALID_PAGE_ID;
      _checkpointReserved = INVALID_PAGE_ID;
      return;
   }

   INT32 deltaLogConsole::createNewFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(nullptr == _workingFile, "do not reopen");
      storageFileName fn;
      createStorageFileOptions o;

      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest.sid);

      _workingFile = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(nullptr == _workingFile))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(_manifest.ftype, _manifest.stype, _baseSequence))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = _manifest.args;
      o.createAsTmpFile = FALSE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = _manifest.secretValue;
      o.flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;

      rc = sfm.createStorageFile(fn, o, *_workingFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fn.getFileName(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(_workingFile);
      goto done;
   }

   INT32 deltaLogConsole::load(const STORAGE_FILE_NAME_LIST *fl)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "must be ready");
      SDB_ASSERT(nullptr != fl, "can not be null");

      STORAGE_FILE_NAME_LIST expiredList;
      STORAGE_FILE_NAME_LIST::const_iterator itr;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest.sid);

      itr = fl->begin();
      for (; itr != fl->end(); ++itr)
      {
         
         const storageFileName &fn = *itr;
         if (OSS_UNLIKELY(!fn.isValid()))
         {
            PD_LOG(PDERROR, "invalid file name");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (OSS_UNLIKELY(fn.getSpaceType() != _manifest.stype ||
                               fn.getFileType() != _manifest.ftype ||
                               fn.hasShadowSuffix()))
         {
            PD_LOG(PDERROR, "not target file name:%s", fn.getFileName());
            rc = SDB_INVALIDARG;
            goto error;
         }

         if (fn.getSequence() != _baseSequence)
         {
            PD_LOG(PDERROR, "unmatched file found:%s", fn.getFileName());
            expiredList.push_back(fn);
            continue;
         }
         else if (nullptr != _workingFile)
         {
            PD_LOG(PDERROR, "duplidated working file found[%s]", fn.getFileName());
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         _workingFile = SDB_OSS_NEW storageFile();
         if (OSS_UNLIKELY(nullptr == _workingFile))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         rc = sfm.openStorageFile(fn, storageFileCtlFlag::MMAP_DATA_SEGMENT,
                                  *_workingFile);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open delta log file[%s], rc:%d",
                   fn.getFileName(), rc);
            goto error;
         }

         if (_workingFile->getCommonHeadInMem().getCoreArgs() != _manifest.args)
         {
            PD_LOG(PDERROR, "invalid core args of log file:%s", _workingFile->getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (_workingFile->getCommonHeadInMem().secretValue !=
             _manifest.secretValue)
         {
            PD_LOG(PDERROR, "invalid secret value found in file:%s",
                   _workingFile->getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }

      destroyExpiredFiles(expiredList);
   done:
      return rc;
   error:
      if (nullptr != _workingFile)
      {
         _workingFile->close();
         SDB_OSS_DEL _workingFile;
         _workingFile = nullptr;
      }
      goto done;
   }

   void deltaLogConsole::destroy()
   {
      if (isReady())
      {
         if (nullptr != _workingFile)
         {
            _workingFile->destroy();
            SDB_OSS_DEL _workingFile;
            _workingFile = nullptr;
         }
         fini();
      }
   }

   void deltaLogConsole::destroyExpiredFiles(const STORAGE_FILE_NAME_LIST &fl)
   {
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest.sid);
      for (STORAGE_FILE_NAME_LIST::const_iterator itr = fl.begin();
           itr != fl.end(); ++itr)
      {
         PD_LOG(PDINFO, "begin to remove history file[%s]", itr->getFileName());
         sfm.removeStorageFile(*itr);
      }
      return;
   }
   
   void deltaLogConsole::rebase(UINT32 base,
                                storageFileTrashCan &trashCan)
   {
      storageFileName fn;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(_baseSequence <= base, "invalid base sequence");
      
      if (nullptr != _workingFile)
      {
         SDB_ASSERT(_workingFile->getSequence() < base, "must be over current seq");
         trashCan.push(_workingFile);
         _workingFile = nullptr;
      }

      _baseSequence = base;
      _prechecksum = 0;
      _writingPid = INVALID_PAGE_ID;
      _buffer.release();
      _page = nullptr;
      _lastCheckpoint = LPS_CHECKPOINT();
      _lastCheckpointPid = INVALID_PAGE_ID;
      _checkpointReserved = INVALID_PAGE_ID;

      return;
   }

   INT32 deltaLogConsole::append(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!dlr.isValid() ||
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

      rc = _append(dlr);
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

   INT32 deltaLogConsole::_append(const deltaLogRecord &dlr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      SDB_ASSERT(dlr.isValid(), "can not be invalid");
      SDB_ASSERT(dlr.getLogSize() <= deltaLogFilePage::getDataCapacity(), "out of resource");
      UINT32 size = dlr.getLogSize();

      do
      {
         rc = ensureFileAndBuffer();
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

   INT32 deltaLogConsole::reserveCheckpoint()
   {
      INT32 rc = SDB_OK;
      deltaLogRecordBuilder builder;
      deltaLogRecord dlr;

      if (OSS_UNLIKELY(!isReady()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      builder.buildCheckpointLog(LPS_CHECKPOINT());
      dlr = builder.getDeltaLogRecord();

      rc = _append(dlr);
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
      SDB_ASSERT(nullptr != _workingFile, "must be open");
      deltaLogFilePage *page = nullptr;
      deltaLogRecordBuilder builder;
      builder.buildCheckpointLog(checkpoint);
      deltaLogRecord dlr = builder.getDeltaLogRecord();
      ossValuePtr ptr = 0;
      INT32 rc = _workingFile->getPagePtr(_checkpointReserved, ptr);
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
      SDB_ASSERT(_manifest.isValid(), "must be valid");
      SDB_ASSERT(nullptr != _workingFile, "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != _writingPid, "can not be invalid");
      SDB_ASSERT(nullptr != _page && _page->isValid(), "can not be null");

      ossValuePtr ptr = 0;
      _workingFile->getPagePtr(_writingPid, ptr);
      SDB_ASSERT(0 != ptr, "impossible");

      ossMemcpy((void *)ptr, _page, _manifest.args.pageSize);
      ++_writingPid;
      _prechecksum = _page->frontChecksum;
      _page = nullptr;
      return;
   }

   INT32 deltaLogConsole::ensureFileAndBuffer()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReady(), "can not be invalid");
      UINT32 minSegmentCount = 0;
      const storageCoreArgs &args = _manifest.args;

      if (nullptr == _workingFile)
      {
         rc = createNewFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create working file:%d", rc);
            goto error;
         }

         _writingPid = 0;
      }

      SDB_ASSERT(ossIsPowerOf2(args.maxPageCountPerSeg), "must be power of 2");
      minSegmentCount = ossAlignX(_writingPid + 1, args.maxPageCountPerSeg) /
                                  args.maxPageCountPerSeg;
      rc = _workingFile->ensureSegmentCount(minSegmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure file segment space:%d", rc);
         goto error;
      }

      if (0 == _buffer.getCapacity())
      {
         rc = _buffer.reserve(_manifest.args.pageSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve buffer size:%d", rc);
            goto error;
         }
      }

      if (nullptr == _page)
      {
          _page = (deltaLogFilePage *)(_buffer.getBuffer());
         _page->init(_prechecksum);
      }

   done:
      return rc;
   error:
      goto done;
   }

   UINT64 deltaLogConsole::getDeltaLogSize()const
   {
      SDB_ASSERT(isReady(), "can not be invalid");
      return (INVALID_PAGE_ID == _writingPid) ?
             0 : ((UINT64)_manifest.args.pageSize * (_writingPid + 1));
   }
   
   INT32 deltaLogConsole::findLastCheckpointPid(const storageFile &file,
                                                PAGE_ID &pid)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(file.isOpen(), "must be open");
      pid = INVALID_PAGE_ID;
      UINT32 prechecksum = 0;

      for (UINT32 i = 0; i < file.getSegmentCount(); ++i)
      {
         for (UINT32 j = 0; j < DELTA_LOG_FILE_PAGE_COUNT_PER_SEG; ++j)
         {
            PAGE_ID pidToScan = (i * DELTA_LOG_FILE_PAGE_COUNT_PER_SEG) + j;
            ossValuePtr ptr = 0;
            const deltaLogFilePage *page = nullptr;

            rc = file.getPagePtr(pidToScan, ptr);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get page[%d] ptr:%d in file[%s]",
                      pidToScan, rc, file.getFullPath());
               goto error;
            }

            page = (const deltaLogFilePage *)ptr;
            if (!page->isValid() || prechecksum != page->prechecksum)
            {
               /// ignore all data after the first crashed page,
               /// even still non-crashed pages on there.
               PD_LOG(PDDEBUG, "stop to scan delta log [%s] at pid[%d]",
                      file.getFullPath(), pidToScan);
               goto done;
            }

            prechecksum = page->frontChecksum;
            if (page->hasCheckpoint())
            {
               deltaLogRecord dlr;
               dlr.reset(page->data + page->checkpointOffset);
               const LPS_CHECKPOINT *checkpoint = nullptr;
               if (!dlr.isValid())
               {
                  PD_LOG(PDERROR, "checkpoint found at page[%d], but not invalid", pidToScan);
                  rc = SDB_VESSEL_INTERNAL_ERR;
                  goto error;
               }

               checkpoint = dlr.getRecordBodyPtr<LPS_CHECKPOINT>(0);
               if (nullptr == checkpoint)
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
      SDB_ASSERT(nullptr != _workingFile, "must be open");
      PAGE_ID pid = INVALID_PAGE_ID;
   
      rc = findLastCheckpointPid(*_workingFile, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find last checkpoint page in file:%s, rc:%d",
                _workingFile->getFullPath(), rc);
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
         const deltaLogFilePage *page = nullptr;
         ossValuePtr ptr = 0;
         _workingFile->getPagePtr(pid, ptr);
         page = (const deltaLogFilePage *)ptr;
         SDB_ASSERT(page->isValid() && page->hasCheckpoint(), "impossible");
         dlr.reset((const CHAR *)(page->data) + page->checkpointOffset);
         const LPS_CHECKPOINT *checkpoint = dlr.getRecordBodyPtr<LPS_CHECKPOINT>(0);
         _lastCheckpoint = *checkpoint;
         _lastCheckpointPid = pid;
         _writingPid = pid + 1; /// move to the next page
         _prechecksum = page->frontChecksum;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void deltaLogConsole::fsyncDirtyPages()const
   {
      SDB_ASSERT(INVALID_PAGE_ID != _checkpointReserved, "can not be invalid");
      SDB_ASSERT(nullptr != _workingFile, "can not be closed");
      UINT32 pageCount = _workingFile->getCommonHeadInMem().maxPageCountPerSeg;
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
         rc = _workingFile->fsyncSegment(i);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDSEVERE, "failed to fsync delta log segment[%d], rc:%d", i, rc);
         }
      }

      rc = _workingFile->fsyncPagesInSeg(maxSegment, (_checkpointReserved % pageCount) + 1);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDSEVERE, "failed to fsync delta log segment[%d], rc:%d", maxSegment, rc);
      }
      return;
   }
}//namespace vessel
}//namespace engine