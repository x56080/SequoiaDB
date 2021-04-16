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

   Source File Name = controlFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/controlFile.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   controlFile::controlFile()
   {
      
   }

   controlFile::~controlFile()
   {
      close();
   }

   INT32 controlFile::open(const CHAR *path)
   {
      INT32 rc = SDB_OK;
      const CHAR *prefix = getFileNamePrefix();
      SDB_ASSERT(NULL != prefix, "can not be null");
      SDB_ASSERT(0 < ossStrlen(prefix), "can not be empty");
      SDB_ASSERT(0 < getMaxAliveVersionCount() &&
                 getMaxAliveVersionCount() <= 64, "can not be invalid");
      SDB_ASSERT(!isOpen(), "do not reinit");
      strSlice pathSlice(path);
      if (pathSlice.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = openFilesUnderPath(pathSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init control files:%s, rc:%d", path, rc);
         goto error;
      }

      if (_workshop.empty() && _unused.empty())
      {
         PD_LOG(PDERROR, "no available files exist");
         rc = SDB_VESSEL_CF_FAILED_TO_INIT;
         goto error;
      }
      else if (!_workshop.empty())
      {
         _fileObj *obj = _workshop.back();
         _commitVersion = obj->getHead()->commitVersion + 1;
      }

      _isOpen = TRUE;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void controlFile::close()
   {
      for (_FILE_OBJ_LIST::iterator itr = _unused.begin();
           itr != _unused.end(); ++itr)
      {
         SDB_OSS_DEL *itr;
      }
      _unused.clear();

      for (_FILE_OBJ_LIST::iterator itr = _workshop.begin();
           itr != _workshop.end(); ++itr)
      {
         SDB_OSS_DEL *itr;
      }
      _workshop.clear();

      _commitVersion = 0;
      _isOpen = FALSE;
      return;
   }

   INT32 controlFile::commit(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (0 == size || NULL == buf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((CONTROL_FILE_SIZE - sizeof(controlFile::head) < size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_unused.empty())
      {
         rc = commitFromUnusedList(size, buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit data from unused list:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = commitFromWorkshop(size, buf);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit data from workshop:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::commitFromUnusedList(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(!_unused.empty(), "can not be empty");
      SDB_ASSERT(size + sizeof(controlFile::head) <= CONTROL_FILE_SIZE,
                 "can not be invalid");
      _fileObj *obj = _unused.front();
      _unused.pop_front();

      obj->getHead()->commitVersion = _commitVersion;
      obj->getHead()->updateMillis = ossGetCurrentMilliseconds();
      obj->getHead()->contentLen = size;
      ossMemcpy(obj->buf + sizeof(controlFile::head),
                buf, size);
      rc = writeFile(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write control file[%d], rc:%d", obj->seq, rc);
         goto error;
      }

      _workshop.push_back(obj);
      ++_commitVersion;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   INT32 controlFile::commitFromWorkshop(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(!_workshop.empty(), "can not be empty");
      SDB_ASSERT(size + sizeof(controlFile::head) <= CONTROL_FILE_SIZE,
                 "can not be invalid");
      CHAR backup[CONTROL_FILE_SIZE] = {0};
      _fileObj *obj = _workshop.front();
      ossMemcpy(backup, obj->buf, CONTROL_FILE_SIZE);
      obj->getHead()->commitVersion = _commitVersion;
      obj->getHead()->updateMillis = ossGetCurrentMilliseconds();
      obj->getHead()->contentLen = size;
      ossMemset(obj->buf + sizeof(controlFile::head),
                0, CONTROL_FILE_SIZE - sizeof(controlFile::head));
      ossMemcpy(obj->buf + sizeof(controlFile::head), buf, size);
      rc = writeFile(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write control file[%d], rc:%d", obj->seq, rc);
         goto error;
      }

      if (1 < _workshop.size())
      {
         _workshop.pop_front();
         _workshop.push_back(obj);
      }
      ++_commitVersion;
   done:
      return rc;
   error:
      ossMemcpy(obj->buf, backup, CONTROL_FILE_SIZE);
      goto done;
   }

   INT32 controlFile::readLatestVersion(head &h, UINT32 bufSize, void *buf)const
   {
      INT32 rc = SDB_OK;
      if (NULL == buf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_workshop.empty())
      {
         rc = SDB_VESSEL_CF_VERSION_NOT_AVAILABLE;
         goto error;
      }

      rc = read(_workshop.back(), bufSize, h, buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read data:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readOldestVersion(head &h,
                                        UINT32 bufSize,
                                        void *buf)const
   {
      INT32 rc = SDB_OK;
      if (NULL == buf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_workshop.empty())
      {
         rc = SDB_VESSEL_CF_VERSION_NOT_AVAILABLE;
         goto error;
      }

      rc = read(_workshop.front(), bufSize, h, buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read data:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readPreVersion(UINT32 preCountOfLatest,
                                     head &h,
                                     UINT32 bufSize,
                                     void *buf)const
   {
      INT32 rc = SDB_OK;
      _FILE_OBJ_LIST::const_reverse_iterator itr;
      if (NULL == buf)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_workshop.size() < (preCountOfLatest + 1))
      {
         rc = SDB_VESSEL_CF_VERSION_NOT_AVAILABLE;
         goto error;
      }

      itr = _workshop.rbegin();
      for (UINT32 i = 0; i < preCountOfLatest; ++i)
      {
         ++itr;
      }

      rc = read(*itr, bufSize, h, buf);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::read(const _fileObj *obj,
                           UINT32 size,
                           head &h,
                           void *buf)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->getHead()->isValid(), "can not be invalid");
      SDB_ASSERT(!obj->getHead()->isUnused(), "can not be unused");
      SDB_ASSERT(NULL != buf, "can not be null");

      if (size < obj->getHead()->contentLen)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      h = *(obj->getHead());
      ossMemcpy(buf, obj->buf + sizeof(controlFile::head), obj->getHead()->contentLen);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::openFilesUnderPath(const strSlice &path)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!path.empty(), "can not be empty");
      const CHAR *prefix = getFileNamePrefix();
      strSlice prefixSlice(prefix);
      UINT32 count = getMaxAliveVersionCount();
      _fileObj *obj = NULL;

      for (UINT32 i = 0; i < count; ++i)
      {
         std::stringstream ss;
         ss << path.str()
            << OSS_FILE_SEP
            << prefix
            << ".control."
            << i;
         std::string fullPath = ss.str();
         obj = SDB_OSS_NEW _fileObj();
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         obj->seq = i;
         rc = initFileObj(fullPath, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init file obj[%d], rc:%d", i, rc);
            rc = SDB_OK;
            SDB_OSS_DEL obj;
            obj = NULL;
            continue;
         }

         SDB_ASSERT(obj->getHead()->isValid(), "must be valid");
         if (obj->getHead()->isUnused())
         {
            pushToUnusedListWhenOpen(obj);
         }
         else
         {
            pushToWorkshopWhenOpen(obj);
         }
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   INT32 controlFile::initFileObj(const std::string &fullPath, _fileObj *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(NULL != obj && !obj->file.isOpened(), "impossible");
      INT64 fileSize = 0;
      rc = ossOpen(fullPath.c_str(),
                   OSS_CREATE | OSS_READWRITE | OSS_EXCLUSIVE,
                   OSS_DEFAULTFILE,
                   obj->file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control file[%s], rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      rc = ossGetFileSize(&(obj->file), &fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size[%s], rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      if ((INT64)CONTROL_FILE_SIZE < fileSize)
      {
         PD_LOG(PDERROR, "invalid file size:%lld of control file[%d]", fileSize, fullPath.c_str());
         goto error;
      }
      else if (fileSize < (INT64)CONTROL_FILE_SIZE)
      {
         controlFile::head h;
         h.headVerion = CONTROL_FILE_VERSION;
         rc = ossExtendFile(&(obj->file), (INT64)CONTROL_FILE_SIZE - fileSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file[%s] to valid size, rc:%d", fullPath.c_str(), rc);
            goto error;
         }

         *((controlFile::head *)(obj->buf)) = h;
         rc = writeFile(obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write file[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }
      else
      {
         SINT64 read = 0;
         rc = ossReadN(&(obj->file), (INT64)CONTROL_FILE_SIZE, obj->buf, read);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file[%s], rc:%d", fullPath.c_str(), rc);
            goto error;
         }
         else if ((INT64)CONTROL_FILE_SIZE != read)
         {
            PD_LOG(PDERROR, "failed to read valid size of file[%s], rc:%d", fullPath.c_str(), rc);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (!obj->getHead()->isValid())
         {
            PD_LOG(PDERROR, "invalid file head version of file:%s", fullPath.c_str());
            ossMemset(obj->buf, 0, CONTROL_FILE_SIZE);
            controlFile::head h;
            h.headVerion = CONTROL_FILE_VERSION;
            *((controlFile::head *)(obj->buf)) = h;
            rc = writeFile(obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to write file[%s], rc:%d", fullPath.c_str(), rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      if (obj->file.isOpened())
      {
         ossClose(obj->file);
         ossMemset(obj->buf, 0, CONTROL_FILE_SIZE);
      }
      goto done;
   }

   INT32 controlFile::writeFile(_fileObj *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "can not be closed");
      SINT64 written = 0;
      rc = ossSeekAndWriteN(&(obj->file), (SINT64)0,
                            obj->buf, (SINT64)CONTROL_FILE_SIZE,
                            written);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write file[%d], rc:%d", obj->seq, rc);
         goto error;
      }

      ossFsync(&(obj->file));
   done:
      return rc;
   error:
      goto done;
   }

   void controlFile::pushToUnusedListWhenOpen(_fileObj *obj)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "must be open");
      if (!_unused.empty())
      {
         _fileObj *last = _unused.back();
         SDB_ASSERT(last->seq < obj->seq, "must be sorted");
      }
      _unused.push_back(obj);
   done:
      return;
   }

   void controlFile::pushToWorkshopWhenOpen(_fileObj *obj)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "must be open");
      SDB_ASSERT(INVALID_COMMIT_VERSION != obj->getHead()->commitVersion, "can not be invalid");
      _FILE_OBJ_LIST::iterator itr = _workshop.begin();
      for (; itr != _workshop.end(); ++itr)
      {
         if ((*itr)->getHead()->commitVersion < obj->getHead()->commitVersion)
         {
            continue;
         }
         else
         {
            _workshop.insert(itr, obj);
            goto done;
         }
      }

      _workshop.push_back(obj);
   done:
      return;
   }
}//namespace vessel
}//namespace engine