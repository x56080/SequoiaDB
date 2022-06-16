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

   Source File Name = multiControlFilesMgr.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/16/2022  ZHY Initial Draft
   Last Changed =

******************************************************************************/

#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "ossUtil.h"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "vessel/fileNameLoader.h"
#include "vessel/multiControlFilesMgr.h"
#include <cstdio>
#include <string>
namespace fs = boost::filesystem;
namespace engine
{
namespace vessel
{
   INT32 multiControlFilesMgr::load(const std::string &prefix, const std::string &path, BOOLEAN deleteInvalidFiles)
   {
      INT32 rc = SDB_OK;
      _prefix = prefix;
      _dirPath = path;
      rc = _scanFiles(deleteInvalidFiles);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = _deleteDeprecatedFiles();
      if (SDB_OK != rc)
      {
         goto error;
      }
      _isLoaded = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   void multiControlFilesMgr::setMaxValidFilesNum(UINT32 filesMaxNum)
   {
      _filesNumMax = filesMaxNum;
      if (_isLoaded)
      {
         _deleteDeprecatedFiles();
      }
   }

   INT32 multiControlFilesMgr::_scanFiles(BOOLEAN deleteInvalidFiles)
   {
      INT32 rc = SDB_OK;
      fs::path dirPath(_dirPath);
      std::map<UINT64, const std::string *> validPaths;
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
               rc = _deleteFile(version);
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
      goto done;
   }

   fs::path multiControlFilesMgr::_getPathFromVersion(UINT32 version) const
   {
      fs::path path(_dirPath);
      CHAR filename[CONTROL_FILE_MAX_NAME_LEN];
      std::sprintf(filename, "%s.%06d", _prefix.c_str(), version);
      path /= filename;
      return std::move(path);
   }

   UINT32 multiControlFilesMgr::_getNextVersion() const
   {
      if (_fileMetaList.size() == 0)
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
      fs::path filePath(path);
      std::vector<std::string> columns = utilStrSplit(filePath.filename().string(), ".");
      if (filePath.filename().string().size() > CONTROL_FILE_MAX_NAME_LEN)
      {
         return FALSE;
      }
      if (columns.size() != CONTROL_FILE_NAME_FORMAT_COLUMNS)
      {
         return FALSE;
      }
      if (!utilStrIsDigit(columns.at(1).c_str()) || columns.at(1).size() < CONTROL_FILE_MIN_VERSION_SUFFIX_LEN)
      {
         return FALSE;
      }
      if (columns.at(0).compare(_prefix))
      {
         return FALSE;
      }
      version = std::stoul(columns.at(1));
      return TRUE;
   }

   void multiControlFilesMgr::_pushBackFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime)
   {
      if (_fileMetaList.size() != 0)
      {
         SDB_ASSERT(version > _fileMetaList.back().commitVersion, "version to be added must be greater than current max version");
      }
      _fileMetaList.emplace_back(version, path, contentLen, creationTime);
   }

   void multiControlFilesMgr::_pushFrontFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime)
   {
      if (_fileMetaList.size() != 0)
      {
         SDB_ASSERT(version < _fileMetaList.front().commitVersion, "version to be added must be less than current min version");
      }
      _fileMetaList.emplace_front(version, path, contentLen, creationTime);
   }

   UINT32 multiControlFilesMgr::getValidFilesNum() const
   {
      SDB_ASSERT(_isLoaded, "the mgr must load control files first");
      return _fileMetaList.size();
   }

   INT32 multiControlFilesMgr::_deleteFile(UINT32 version)
   {
      INT32 rc = SDB_OK;
      fs::path filePath = _getPathFromVersion(version);
      try
      {
         fs::remove(filePath);
      }
      catch (exception e)
      {
         PD_LOG(PDERROR, "failed to remove control file, rc:%d ,exception: %s", rc, e.what());
         rc = SDB_IO;
         goto error;
      }
      PD_LOG(PDINFO, "removed control file:%s", filePath.c_str());

   done:
      return rc;
   error:
      goto done;
   }

   INT32 multiControlFilesMgr::_deleteDeprecatedFiles()
   {
      INT32 rc = SDB_OK;
      while (_fileMetaList.size() > _filesNumMax)
      {
         rc = _deleteFile(_fileMetaList.front().commitVersion);
         if (SDB_OK != rc)
         {
            goto error;
         }
         _fileMetaList.pop_front();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 multiControlFilesMgr::createFile(const CHAR *buf, UINT32 bufSize)
   {
      SDB_ASSERT(_isLoaded, "the mgr must load control files first");
      INT32 rc = SDB_OK;
      UINT32 version = _getNextVersion();
      fs::path filePath = _getPathFromVersion(version);
      strSlice fileSlice(filePath.c_str());
      controlFile file;
      invalidFileReason reason;
      rc = controlFile::create(fileSlice, buf, bufSize);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rc = file.openToRead(fileSlice, reason);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file after creation, rc:%d", rc);
         goto error;
      }
      _pushBackFileMeta(version, filePath.string(), file.getContentLen(), file.getCreationTime());
      rc = _deleteDeprecatedFiles();
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 multiControlFilesMgr::getCurrentVersionFileContent(memoryBlock &block) const
   {
      INT32 rc = SDB_OK;
      if (_fileMetaList.size() == 0)
      {
         rc = SDB_FNE;
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
            PD_LOG(PDERROR, "failed to open current file %s, rc:%d", _fileMetaList.back().path.c_str(), rc);
            goto error;
         }
         block.resize(file.getContentLen());
         rc = file.read(block.getBuffer(), file.getContentLen());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read current file %s content,rc:%d", _fileMetaList.back().path.c_str(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void multiControlFilesMgr::list(ossPoolList<bson::BSONObj> &l) const
   {
      SDB_ASSERT(_isLoaded, "the mgr must load control files first");
      bson::BSONObjBuilder builder;
      for (auto it = _fileMetaList.begin(); it != _fileMetaList.end(); it++)
      {
         builder.reset();
         builder.appendNumber(CONTROL_FILE_FILEDNAME_COMMIT_VERSION, static_cast<INT32>(it->commitVersion));
         builder.append(CONTROL_FILE_FILEDNAME_PATH, it->path.c_str(), it->path.size());
         builder.appendNumber(CONTROL_FILE_FILEDNAME_CONTENT_LENGTH, static_cast<INT32>(it->contentLen));
         builder.appendTimestamp(CONTROL_FILE_FILEDNAME_CREATION_TIME, it->creationTime);
         l.emplace_back(builder.obj());
      }
   }
} // namespace vessel
} // namespace engine