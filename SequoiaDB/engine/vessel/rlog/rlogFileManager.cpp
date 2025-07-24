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

   Source File Name = rlogFileManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rlogFileManager.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "../bson/util/builder.h"
#include "dpsDef.hpp"
#include "vessel/redoLogFileReader.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   rlogFileManager::~rlogFileManager()
   {

   }

   INT32 rlogFileManager::init(const CHAR *dir, UINT64 writingOffset, redoLogFilesSummary &summary)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_logDir.empty(), "do not reinit");
      _FILE_OBJ_MAP fmap;

      if (OSS_UNLIKELY(nullptr == dir || 0 == ossStrlen(dir)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _logDir = dir;
      
      rc = _preload(dir, fmap);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to preload log files:%d", rc);
         goto error;
      }

      rc = _load(writingOffset, fmap, summary);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load log files:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini();
      summary = redoLogFilesSummary();
      goto done;
   }

   void rlogFileManager::fini()
   {
      _logDir.clear();
      _writingOffset = 0;
      _dirtyOffset = 0;
      _nextFileId = 0;
      if (nullptr !=_workingFile)
      {
         _workingFile->close();
         _workingFile.reset();
      }
      _closeAndClear(_readonlyFiles);
      _closeAndClear(_pool);
      return;
   }

   INT32 rlogFileManager::_preload(const CHAR *dir, _FILE_OBJ_MAP &fmap)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != dir, "can not be invalid");
      fmap.clear();

      try
      {
         fs::directory_iterator end_iter ;
         fs::path dataDir(dir);
         if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
         {
            PD_LOG(PDERROR, "invalid dir path to load:%s", dir);
            rc = SDB_FNE;
            goto error;
         }

         for (fs::directory_iterator dir_iter(dataDir);
              dir_iter != end_iter; ++dir_iter)
         {
            RLOG_FILE_UPTR file;
            std::string path = dir_iter->path().string();
            std::string fileName = dir_iter->path().filename().string();
            if (!fs::is_regular_file(dir_iter->status()))
            {
               PD_LOG(PDDEBUG, "not regular file", fileName.c_str());
               continue;
            }
            else if (!boost::starts_with(fileName, RLOG_FILE_NAME_PREFIX))
            {
               PD_LOG(PDDEBUG, "not redo log file", fileName.c_str());
               continue;
            }
            
            file.reset(SDB_OSS_NEW redoLogFile());
            if (OSS_UNLIKELY(!file))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            rc = file->open(path.c_str());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to open file:%s, rc:%d", path.c_str(), rc);
               goto error;
            }

            if (!fmap.emplace(file->getLogicalId(), std::move(file)).second)
            {
               PD_LOG(PDERROR, "duplidated logical id found:[%d,%s]",
                      file->getLogicalId(), fileName.c_str());
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected exception:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }
   
   done:
      return rc;
   error:

      goto done;
   }

   INT32 rlogFileManager::_load(UINT64 writingOffset, _FILE_OBJ_MAP &fmap, redoLogFilesSummary &summary)
   {
      INT32 rc = SDB_OK;
      UINT32 workingFileId = writingOffset / RLOG_FILE_BODY_SIZE;
                               
      summary = redoLogFilesSummary();

      if (_hasMissingFiles(workingFileId, fmap))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      summary.totalFiles = fmap.size();
      if (!fmap.empty())
      {
         summary.oldestLSN = fmap.cbegin()->second->getStartLSN();
      }

      rc = _classifyFiles(workingFileId, fmap);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to classify log files:%d", rc);
         goto error;
      }

      /// all values of fmap can not be accessed from here!

      summary.readonlyFiles = _readonlyFiles.size();

      if (0 < writingOffset)
      {
         rc = _extractCurrentLSN(writingOffset, summary.currentLSN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extract current lsn:%d", rc);
            goto error;
         }
      }

      _writingOffset = writingOffset;
      _dirtyOffset = writingOffset;
      _nextFileId = fmap.empty() ? 0 : fmap.crbegin()->first + 1;
      
   done:
      fmap.clear();
      return rc;
   error:
      summary = redoLogFilesSummary();
      
      goto done;
   }

   BOOLEAN rlogFileManager::isLogFileToBeSwitched(UINT64 lsn,
                                                  UINT32 recordSize,
                                                  UINT32 &fillingSize) const
   {
      SDB_ASSERT(0 < recordSize, "can not be invalid");
      fillingSize = 0;
      UINT64 startFileId = lsn / RLOG_FILE_BODY_SIZE;
      UINT64 endFileId = (lsn + recordSize - 1) / RLOG_FILE_BODY_SIZE;
      if (startFileId != endFileId)
      {
         fillingSize = RLOG_FILE_BODY_SIZE - (lsn % RLOG_FILE_BODY_SIZE);
      }

      return 0 < fillingSize;
   }

   UINT32 rlogFileManager::getFileLastFreeSize(UINT64 lsn, UINT32 recordSize) const
   {
      return RLOG_FILE_BODY_SIZE - ((lsn + recordSize) % RLOG_FILE_BODY_SIZE);
   }
   
   BOOLEAN rlogFileManager::_hasMissingFiles(UINT32 workingFileId, const _FILE_OBJ_MAP &fmap) const
   {
      BOOLEAN r = FALSE;
      if (!fmap.empty())
      {
         auto itr = fmap.cbegin();
         UINT32 lastLogicalId = itr->first;
         ++itr;
         for (; itr != fmap.cend(); ++itr)
         {
            if (itr->first != (lastLogicalId + 1))
            {
               PD_LOG(PDERROR, "file missed between[%d, %d]", lastLogicalId, itr->first);
               r = TRUE;
               goto done;
            }

            lastLogicalId = itr->first;
         }

         if (workingFileId < fmap.cbegin()->first)
         {
            PD_LOG(PDERROR, "working file id[%d] is lower than first file[%d]",
                  workingFileId, fmap.cbegin()->first);
            r = TRUE;
            goto done;
         }
         else if (workingFileId > (fmap.crbegin()->first + 1))
         {
            PD_LOG(PDERROR, "log file missed between last file and working file [%d,%d]",
                  fmap.crbegin()->first, workingFileId);
            r = TRUE;
            goto done;
         }
      }
      else if (0 != workingFileId)
      {
         PD_LOG(PDERROR, "non log file preloaded but working file id is [%d]",
                workingFileId);
         r = TRUE;
         goto done;
      }

   done:
      return r;
   }

   INT32 rlogFileManager::_flushWorkingFile()
   {
      INT32 rc = SDB_OK;
      
      if (_isWorkingFileDirty())
      {
         SDB_ASSERT(nullptr != _workingFile, "can not be invalid");
         rc = _workingFile->fsync();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to flush working file[%d]:%d", _workingFile->getLogicalId(), rc);
            goto error;
         }

         _dirtyOffset = _writingOffset;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::_pushWorkingFileIntoReadonlyList()
   {
      INT32 rc = SDB_OK;
      try
      {
         std::unique_lock<std::mutex> guard(_rmutex);
         SDB_ASSERT(nullptr != _workingFile, "can not be invalid");
         _readonlyFiles.emplace_back(std::move(_workingFile));
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected exception happened:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void rlogFileManager::_closeAndClear(_FILE_OBJ_LIST &fl)
   {
      for (auto itr = fl.begin(); itr != fl.end(); ++itr)
      {
         (*itr)->close();
      }
      fl.clear();
      return;
   }

   INT32 rlogFileManager::_createFullPath(UINT32 logicalId, ossPoolString &path) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_logDir.empty(), "can not be empty");
      path.clear();
      try
      {
         bson::StringBuilder builder;
         builder << _logDir << OSS_FILE_SEP << RLOG_FILE_NAME_PREFIX
               << '.';
         builder.appendUint32WithF(logicalId, "%06d");
         path = builder.poolStr();
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected exception happened:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::prepareFile(UINT64 offset)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_logDir.empty(), "can not be empty");

      UINT32 fileId = _getFileId(offset);
      if (fileId < _nextFileId)
      {
         goto done;
      }
      else if (fileId != _nextFileId)
      {
         PD_LOG(PDERROR, "the next file id shoud be[%d], offset[%lld] can not be preared",
                _nextFileId, offset);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else
      {
         rc = _createPoolFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create pool file:%d", rc);
            goto error;
         }

         ++_nextFileId;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::_createPoolFile()
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = _nextFileId;
      ossPoolString path;
      RLOG_FILE_UPTR ptr(SDB_OSS_NEW redoLogFile());
      if (OSS_UNLIKELY(!ptr))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = _createFullPath(fileId, path);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create full path:%d", rc);
         goto error;
      }

      rc = ptr->create(path.c_str(), fileId);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", path.c_str(), rc);
         goto error;
      }

      try
      {
         std::unique_lock<std::mutex> guard(_poolMutex);
         _pool.emplace_back(std::move(ptr));
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected exception happened:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }
      
   done:
      return rc;
   error:
      if (ptr && ptr->isOpen())
      {
         ptr->unlink();
      }
      goto done;
   }

   BOOLEAN rlogFileManager::_setUpWorkingFile()
   {
      SDB_ASSERT(!_workingFile, "do not reinit");
      std::unique_lock<std::mutex> guard(_poolMutex);
      if (!_pool.empty())
      {
         _workingFile = std::move(_pool.front());
         _pool.pop_front();
         return TRUE;
      }
      
      return FALSE;
   }

   INT32 rlogFileManager::write(redoLogBuffer &buffer, BOOLEAN &fileIsFull)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(buffer.isValid(), "can not be invalid");
      UINT32 offset = 0;
      UINT32 size = 0;
      fileIsFull = FALSE;
      
      if (nullptr == _workingFile)
      {
         if (!_setUpWorkingFile())
         {
            PD_LOG(PDERROR, "failed to set up working file:%d", rc);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      buffer.wait(size);
      if (0 == size)
      {
         goto done;
      }

      offset = _writingOffset % RLOG_FILE_BODY_SIZE;

      rc = _workingFile->write(offset, size, buffer.getUnsyncedPtr());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write log file:%d", rc);
         goto error;
      }

      buffer.updateSyncedPos(size);
      _writingOffset += size;
      fileIsFull = (0 == _writingOffset % RLOG_FILE_BODY_SIZE);
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::switchWorkingFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _workingFile, "can not be invalid");
      UINT64 offset = _workingFile->getStartLSN() + RLOG_FILE_BODY_SIZE;
      SDB_ASSERT(_writingOffset == offset, "must be same");

      rc = _flushWorkingFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to flush working file:%d", rc);
         ossPanic();
         goto error;
      }

      rc = _pushWorkingFileIntoReadonlyList();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to archive working file:%d", rc);
         ossPanic();
         goto error;
      }

      /// pool may be empty now.
      _setUpWorkingFile();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::flush()
   {
      return _flushWorkingFile();
   }

   UINT64 rlogFileManager::getMinLSN()
   {
      UINT64 minLSN = DPS_INVALID_LSN_OFFSET;
      std::unique_lock<std::mutex> guard(_rmutex);
      if (!_readonlyFiles.empty())
      {
         minLSN =  _readonlyFiles.front()->getStartLSN();
      }
      else if (_workingFile && _workingFile->getStartLSN() < _writingOffset)
      {
         /// we are holding readonly list's mutex,
         /// working file ptr is safe to access
         minLSN =  _workingFile->getStartLSN();
      }

      return minLSN;
   }

   INT32 rlogFileManager::exportFiles(ossPoolVector<const redoLogFile *> &files)
   {
      INT32 rc = SDB_OK;
      files.clear();
      std::unique_lock<std::mutex> lk(_rmutex);
      try
      {
         files.reserve(_readonlyFiles.size() + 1);
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected error happened:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }

      for (auto itr = _readonlyFiles.cbegin(); itr != _readonlyFiles.cend(); ++itr)
      {
         files.push_back(itr->get());
      }

      if (nullptr != _workingFile &&_workingFile->getStartLSN() < _writingOffset)
      {
         files.push_back(_workingFile.get());
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::_extractCurrentLSN(UINT64 lsnUpBound, UINT64 &lsn) const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < lsnUpBound, "can not be invalid");
      redoLogFileReader reader;
      const redoLogFile *file = nullptr;
      lsn = DPS_INVALID_LSN_OFFSET;

      if (0 != lsnUpBound % RLOG_FILE_BODY_SIZE)
      {
         if (nullptr != _workingFile)
         {
            file = _workingFile.get();
         }
         else
         {
            PD_LOG(PDERROR, "the last record shoud be saved in working file but it is invalid");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         
      }
      else if (_readonlyFiles.empty())
      {
         PD_LOG(PDERROR, "the last record shoud not be saved in working file but readonly list is empty");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else
      {
         file = _readonlyFiles.back().get();
      }

      rc = reader.open(file, lsnUpBound);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file reader:%d", rc);
         goto error;
      }

      while (reader.isReady())
      {
         if (reader.isTailInBound())
         {
            const dpsLogRecordHeader &h = reader.getRecordHeader();
            if (h._lsn + h._length != lsnUpBound)
            {
               PD_LOG(PDERROR, "expected lsn does not match the last lsn[%lld, %d]",
                      lsnUpBound, h._lsn, h._length);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            lsn = h._lsn;
            break;
         }
         else
         {
            rc = reader.next();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fetch next record:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rlogFileManager::_classifyFiles(UINT32 workingFileId,
                                         _FILE_OBJ_MAP &fmap)
   {
      INT32 rc = SDB_OK;
      try
      {
         for (auto itr = fmap.begin(); itr != fmap.end(); ++itr)
         {
            if (itr->first < workingFileId)
            {
               _readonlyFiles.emplace_back(std::move(itr->second));
            }
            else if (itr->first == workingFileId)
            {
               _workingFile = std::move(itr->second);
            }
            else
            {
               _pool.emplace_back(std::move(itr->second));
            }
         }
      }
      catch (std::exception &e)
      {
         PD_LOG(PDERROR, "unexpected exception:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
