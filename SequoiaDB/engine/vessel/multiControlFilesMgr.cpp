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

   Source File Name = multiControlFilesMgr.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/16/2022  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "ossUtil.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "utilStr.hpp"
#include "vessel/fileNameLoader.h"
#include "vessel/multiControlFilesMgr.h"
#include <cstdio>
#include <string>

namespace engine
{
namespace vessel
{
   namespace fs = boost::filesystem;
   constexpr UINT32 CONTROL_FILE_NAME_FORMAT_COLUMNS = 3;
   // control file name: <prefix>.control.<version suffix>  eg: cfprefix.control.000001
   constexpr UINT32 CONTROL_FILE_NAME_COLUMN_PREFIX = 0;
   constexpr UINT32 CONTROL_FILE_NAME_COLUMN_CONTROL = 1;
   constexpr UINT32 CONTROL_FILE_NAME_COLUMN_VERSION = 2;
   constexpr UINT32 CONTROL_FILE_MAX_PREFIX_LEN = 64;

   INT32 multiControlFilesMgr::init(const std::string &prefix, const std::string &path, UINT32 maxValidFilesNum)
   {
      INT32 rc = SDB_OK;
      reset();
      if (prefix.empty() || path.empty() || 0 == maxValidFilesNum)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "arguments can not be empty or zero");
         goto error;
      }
      if (prefix.size() > CONTROL_FILE_MAX_PREFIX_LEN)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "the length of prefix must be less than or equal to %d, rc: %d", CONTROL_FILE_MAX_PREFIX_LEN, rc);
         goto error;
      }
      if (std::string::npos != prefix.find('.'))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "the prefix of control files can not contain the '.' symbol");
         goto error;
      }
      if (!fs::exists(path) || !fs::is_directory(path))
      {
         rc = SDB_FNE;
         PD_LOG(PDERROR, "the directory does not exist, path: %s", path.c_str());
         goto error;
      }
      _prefix = prefix;
      _dirPath = path;
      _maxValidFilesNum = maxValidFilesNum;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void multiControlFilesMgr::reset()
   {
      _maxValidFilesNum = CONTROL_FILES_DEFAULT_NUMBER;
      _prefix.clear();
      _dirPath.clear();
      _fileMetaList.clear();
   }

   INT32 multiControlFilesMgr::reload(BOOLEAN deleteInvalidFiles)
   {
      INT32 rc = SDB_OK;
      if (_prefix.empty() || _dirPath.empty())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "the mgr must be initialized first");
         goto error;
      }
      rc = _scanFiles(deleteInvalidFiles);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to scan files when reload, path: %s, rc:%d", _dirPath.c_str(), rc);
         goto error;
      }
      _deleteDeprecatedFiles();
   done:
      return rc;
   error:
      goto done;
   }

   void multiControlFilesMgr::setMaxValidFilesNum(UINT32 maxValidFilesNum)
   {
      SDB_ASSERT(maxValidFilesNum > 0, "Max valid files number must be greater than 0");
      _maxValidFilesNum = maxValidFilesNum;
   }

   INT32 multiControlFilesMgr::_scanFiles(BOOLEAN deleteInvalidFiles)
   {
      SDB_ASSERT(!_prefix.empty() && !_dirPath.empty(), "prefix or directory path can not be empty");
      INT32 rc = SDB_OK;
      _fileMetaList.clear();
      fs::path dirPath(_dirPath);
      ossPoolMap<UINT64, const std::string *> validPaths;
      fileNameLoader loader;
      fileNameList fnl;
      ossPoolList<std::string> paths;
      rc = loader.loadFileNameList(_dirPath, fnl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load file name list, rc:%d", rc);
         goto error;
      }
      paths = fnl.getFullFilePaths(_prefix);
      for (auto it = paths.begin(); it != paths.end(); it++)
      {
         fs::path filePath(*it);
         UINT32 version = 0;
         if (_validateAndExtract(filePath.filename().string(), version))
         {
            validPaths.emplace(version, &*it);
         }
      }
      for (auto it = validPaths.begin(); it != validPaths.end(); it++)
      {
         controlFile file;
         invalidFileReason reason;
         UINT32 version = it->first;
         const CHAR *filePathStr = it->second->c_str();
         strSlice fileSlice(filePathStr, it->second->size());
         rc = file.openToRead(fileSlice, reason);
         if (SDB_OK == rc)
         {
            _pushBackFileMeta(version, *(it->second), file.getContentLen(), file.getCreationTime());
         }
         else if (SDB_VESSEL_INVALID_CONTROL_FILE == rc)
         {
            if (deleteInvalidFiles)
            {
               rc = _deleteFile(*(it->second));
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to delete file %s which is not validated", filePathStr);
                  goto error;
               }
            }
            else
            {
               PD_LOG(PDWARNING, "the control file %s may be corrupted", filePathStr);
               continue;
            }
         }
         else
         {
            PD_LOG(PDERROR, "failed to open file %s, rc:%d", filePathStr, rc);
            goto error;
         }
      } // for (auto it = scannedPaths.rbegin(); it != scannedPaths.rend(); it++)

   done:
      return rc;
   error:
      _fileMetaList.clear();
      goto done;
   }

   fs::path multiControlFilesMgr::_getPathFromVersion(UINT32 version) const
   {
      SDB_ASSERT(!_prefix.empty() && !_dirPath.empty(), "prefix or directory path can not be empty");
      fs::path path(_dirPath);
      // CONTROL_FILE_MAX_PREFIX_LEN + strlen(CONTROL_FILE_NAME_TYPE) + 2(length of two '.') + 10(length of UINT32_MAX decimal digits) + terminator,
      // then align to 32.
      constexpr UINT32 CONTROL_FILE_FILEDNAME_ARRAY_SIZE = 96;
      CHAR filename[CONTROL_FILE_FILEDNAME_ARRAY_SIZE];
      ossSnprintf(filename, CONTROL_FILE_FILEDNAME_ARRAY_SIZE, "%s.%s.%06d", _prefix.c_str(), CONTROL_FILE_NAME_TYPE, version);
      path /= filename;
      return std::move(path);
   }

   UINT32 multiControlFilesMgr::_getNextVersion() const
   {
      if (0 == _fileMetaList.size())
      {
         return 0;
      }
      else
      {
         return _fileMetaList.back().commitVersion + 1;
      }
   }

   BOOLEAN multiControlFilesMgr::_validateAndExtract(const std::string &path, UINT32 &version) const
   {
      SDB_ASSERT(!_prefix.empty() && !_dirPath.empty(), "prefix or directory path can not be empty");
      SDB_ASSERT(!path.empty(), "invalid argument: the path can not be empty");
      version = 0;
      fs::path filePath(path);
      std::vector<std::string> columns = utilStrSplit(filePath.filename().string(), ".");
      if (CONTROL_FILE_NAME_FORMAT_COLUMNS != columns.size())
      {
         return FALSE;
      }
      if (!utilStrIsDigit(columns.at(CONTROL_FILE_NAME_COLUMN_VERSION).c_str()))
      {
         return FALSE;
      }
      UINT64 tmp = std::stoull(columns.at(CONTROL_FILE_NAME_COLUMN_VERSION));
      if (tmp > UINT32_MAX)
      {
         return FALSE;
      }
      if (0 != columns.at(CONTROL_FILE_NAME_COLUMN_CONTROL).compare(CONTROL_FILE_NAME_TYPE))
      {
         return FALSE;
      }
      if (0 != columns.at(CONTROL_FILE_NAME_COLUMN_PREFIX).compare(_prefix))
      {
         return FALSE;
      }
      version = tmp;
      return TRUE;
   }

   void multiControlFilesMgr::_pushBackFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime)
   {
      if (0 != _fileMetaList.size())
      {
         SDB_ASSERT(version > _fileMetaList.back().commitVersion, "version to be added must be greater than current max version");
      }
      _fileMetaList.emplace_back(version, path, contentLen, creationTime);
   }

   void multiControlFilesMgr::_pushFrontFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime)
   {
      if (0 != _fileMetaList.size())
      {
         SDB_ASSERT(version < _fileMetaList.front().commitVersion, "version to be added must be less than current min version");
      }
      _fileMetaList.emplace_front(version, path, contentLen, creationTime);
   }

   UINT32 multiControlFilesMgr::getValidFilesNum() const
   {
      SDB_ASSERT(!_prefix.empty() && !_dirPath.empty(), "the mgr must be initialized first");
      return _fileMetaList.size();
   }

   INT32 multiControlFilesMgr::_deleteFile(const std::string &path)
   {
      SDB_ASSERT(!path.empty(), "invalid argument: the path of file to delete can not be empty");
      INT32 rc = SDB_OK;
      try
      {
         fs::remove(path);
      }
      catch (exception e)
      {
         PD_LOG(PDERROR, "failed to remove control file, rc:%d ,exception: %s", rc, e.what());
         rc = SDB_IO;
         goto error;
      }
      PD_LOG(PDINFO, "removed control file:%s", path.c_str());

   done:
      return rc;
   error:
      goto done;
   }

   void multiControlFilesMgr::_deleteDeprecatedFiles()
   {
      INT32 rc = SDB_OK;
      while (_fileMetaList.size() > _maxValidFilesNum)
      {
         rc = _deleteFile(_fileMetaList.front().path);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to delete deprecated file %s, rc: %d", _fileMetaList.front().path.c_str(), rc);
         }
         _fileMetaList.pop_front();
      }
   }

   INT32 multiControlFilesMgr::createFile(const CHAR *buf, UINT32 bufSize)
   {
      INT32 rc = SDB_OK;
      UINT32 version = _getNextVersion();
      fs::path filePath = _getPathFromVersion(version);
      strSlice fileSlice(filePath.c_str());
      controlFile file;
      UINT32 contentLen = 0;
      UINT64 creationTime = 0;
      if (_prefix.empty() || _dirPath.empty())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "the mgr must be initialized first");
         goto error;
      }
      if (nullptr == buf || 0 == bufSize)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid buffer");
         goto error;
      }
      rc = controlFile::create(fileSlice, buf, bufSize, FALSE, TRUE, &contentLen, &creationTime);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file %s", fileSlice.str());
         goto error;
      }
      _pushBackFileMeta(version, filePath.string(), contentLen, creationTime);
      file.close();
      _deleteDeprecatedFiles();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 multiControlFilesMgr::getCurrentVersionFileContent(memoryBlock &block) const
   {
      INT32 rc = SDB_OK;
      block.resize(0);
      if (_prefix.empty() || _dirPath.empty() || 0 == _fileMetaList.size())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "the mgr must be initialized first");
         goto error;
      }
      else
      {
         strSlice fileSlice(_fileMetaList.back().path.c_str(), _fileMetaList.back().path.size());
         controlFile file;
         invalidFileReason reason;
         rc = file.openToRead(fileSlice, reason);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open current file %s, rc:%d", fileSlice.str(), rc);
            goto error;
         }
         block.resize(file.getContentLen());
         rc = file.read(block.getBuffer(), file.getContentLen());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read current file %s content,rc:%d", fileSlice.str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      block.resize(0);
      goto done;
   }

   void multiControlFilesMgr::list(ossPoolList<bson::BSONObj> &l) const
   {
      l.clear();
      bson::BSONObjBuilder builder;
      for (auto it = _fileMetaList.begin(); it != _fileMetaList.end(); it++)
      {
         builder.reset();
         builder.appendNumber(CONTROL_FILE_FILEDNAME_COMMIT_VERSION, static_cast<INT32>(it->commitVersion));
         builder.append(CONTROL_FILE_FILEDNAME_PATH, it->path.c_str(), it->path.size());
         builder.appendNumber(CONTROL_FILE_FILEDNAME_CONTENT_LENGTH, static_cast<INT32>(it->contentLen));
         builder.appendTimestamp(CONTROL_FILE_FILEDNAME_CREATION_TIME, it->creationTime);
         l.push_back(builder.obj());
      }
   }
} // namespace vessel
} // namespace engine