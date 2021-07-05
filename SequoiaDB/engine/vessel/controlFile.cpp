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
#include "utilStr.hpp"
#include "xxHashInc.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 MAX_CONTENT_SIZE = CONTROL_FILE_SIZE - sizeof(controlFile::head) - sizeof(UINT32);

   OSS_INLINE UINT32 getMagicCode()
   {
      static const CHAR a[4] = {'S', 'D', 'B', 'V'};
      return *((UINT32 *)a);
   }

   void createChecksum(void *buf)
   {
      SDB_ASSERT(NULL != buf, "can not be null");
      UINT32 *checksum = (UINT32 *)((ossValuePtr)buf + CONTROL_FILE_SIZE - sizeof(UINT32));
      *checksum = XXH3_64bits(buf, CONTROL_FILE_SIZE - sizeof(UINT32));
      return;
   }

   INT32 validateBuf(const void *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != buf, "can not be null");
      const controlFile::head *h = (const controlFile::head *)buf;
      const UINT32 *checksum = NULL;
      UINT32 c = 0;

      if (getMagicCode() != h->magicCode)
      {
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
      if (CONTROL_FILE_VERSION != h->headVerion)
      {
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      checksum = (UINT32 *)((ossValuePtr)buf + CONTROL_FILE_SIZE - sizeof(UINT32));
      c = XXH3_64bits(buf, CONTROL_FILE_SIZE - sizeof(UINT32));
      if (c != *checksum)
      {
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   controlFile::controlFile()
   {
      
   }

   controlFile::~controlFile()
   {
      close();
   }

   INT32 controlFile::create(const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < getMaxAliveVersionCount() &&
                 getMaxAliveVersionCount() <= 64, "can not be invalid");
      SDB_ASSERT(!isOpen(), "do not reinit");
      close();

      _dir = dir.str();
      rc = createFiles();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create control file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy();
      goto done;
   }

   INT32 controlFile::open(const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < getMaxAliveVersionCount() &&
                 getMaxAliveVersionCount() <= 64, "can not be invalid");
      SDB_ASSERT(!isOpen(), "do not reinit");
      close();
      if (OSS_UNLIKELY(dir.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _dir = dir.str();
      rc = openFiles();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control files:%s, rc:%d", dir.str(), rc);
         goto error;
      }

      if (getMaxAliveVersionCount() != (_workshop.size() + _unused.size()))
      {
         PD_LOG(PDERROR, "invalid file count under path:%s", dir.str());
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (!_workshop.empty())
      {
         _fileObj *obj = _workshop.back();
         _commitVersion = obj->getHead()->commitVersion + 1;
      }

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
         _fileObj *obj = *itr;
         obj->close();
         SDB_OSS_DEL obj;
      }
      _unused.clear();

      for (_FILE_OBJ_LIST::iterator itr = _workshop.begin();
           itr != _workshop.end(); ++itr)
      {
         _fileObj *obj = *itr;
         obj->close();
         SDB_OSS_DEL obj;
      }
      _workshop.clear();

      _commitVersion = 0;
      _dir.clear();
      return;
   }

   void controlFile::destroy()
   {
      CHAR path[OSS_MAX_PATHSIZE + 1] = {0};
      for (_FILE_OBJ_LIST::iterator itr = _unused.begin();
           itr != _unused.end(); ++itr)
      {
         _fileObj *obj = *itr;
         if (OSS_LIKELY(!obj->name.empty() &&
                        obj->file.isOpened()))
         {
            INT32 rc = utilBuildFullPath(_dir.c_str(),
                                      obj->name.c_str(),
                                      OSS_MAX_PATHSIZE + 1,
                                      path);
            if (OSS_LIKELY(SDB_OK == rc))
            {
               obj->close();
               ossDelete(path);
            }
         }
         else
         {
            SDB_ASSERT(FALSE, "impossible");
         }
         SDB_OSS_DEL obj;
      }
      _unused.clear();

      for (_FILE_OBJ_LIST::iterator itr = _workshop.begin();
           itr != _workshop.end(); ++itr)
      {
         _fileObj *obj = *itr;
         if (OSS_LIKELY(!obj->name.empty() &&
                        obj->file.isOpened()))
         {
            INT32 rc = utilBuildFullPath(_dir.c_str(),
                                      obj->name.c_str(),
                                      OSS_MAX_PATHSIZE + 1,
                                      path);
            if (OSS_LIKELY(SDB_OK == rc))
            {
               obj->close();
               ossDelete(path);
            }
         }
         else
         {
            SDB_ASSERT(FALSE, "impossible");
         }
         SDB_OSS_DEL obj;
      }
      _workshop.clear();
      _commitVersion = 0;
      _dir.clear();
   done:
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
      else if (OSS_UNLIKELY(0 == size || NULL == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_CONTENT_SIZE < size))
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
      SDB_ASSERT(size <= MAX_CONTENT_SIZE, "can not be invalid");
      _fileObj *obj = _unused.front();
      
      updateFileBuf(obj, size, buf);
      rc = writeFile(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write control file[%s], rc:%d",
                obj->name.c_str(), rc);
         goto error;
      }

      _unused.pop_front();
      _workshop.push_back(obj);
      ++_commitVersion;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::commitFromWorkshop(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_workshop.empty(), "can not be empty");
      SDB_ASSERT(size <= MAX_CONTENT_SIZE, "can not be invalid");

      CHAR backup[CONTROL_FILE_SIZE] = {0};
      _fileObj *obj = _workshop.front();
      ossMemcpy(backup, obj->buf, CONTROL_FILE_SIZE);

      updateFileBuf(obj, size, buf);
      rc = writeFile(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write control file[%s], rc:%d",
                obj->name.c_str(), rc);
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

   INT32 controlFile::readLatestVersion(UINT64 &version,
                                        UINT32 bufSize,
                                        void *buf)const
   {
      INT32 rc = SDB_OK;
      head h;
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
      version = h.commitVersion;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readOldestVersion(UINT64 &version,
                                        UINT32 bufSize,
                                        void *buf)const
   {
      INT32 rc = SDB_OK;
      head h;
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
      version = h.commitVersion;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readPreVersion(UINT32 preCountOfLatest,
                                     UINT64 &version,
                                     UINT32 bufSize,
                                     void *buf)const
   {
      INT32 rc = SDB_OK;
      head h;
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
      version = h.commitVersion;
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

   INT32 controlFile::openFiles()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_dir.empty(), "can not be empty");
      UINT32 count = getMaxAliveVersionCount();
      CHAR path[OSS_MAX_PATHSIZE + 1] = {0};
      _fileObj *obj = NULL;
      UINT32 flags = OSS_READWRITE | OSS_EXCLUSIVE | OSS_WRITETHROUGH;

      for (UINT32 i = 0; i < count; ++i)
      {
         INT64 fileSize = 0;
         obj = SDB_OSS_NEW _fileObj();
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         obj->sequence = i;
         if (!getFileName(i, obj->name))
         {
            PD_LOG(PDERROR, "failed to get file name of sequence:%d", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = utilBuildFullPath(_dir.c_str(),
                                obj->name.c_str(),
                                OSS_MAX_PATHSIZE + 1,
                                path);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build file full path:%d", rc);
            goto error;
         }

         rc = ossOpen(path,
                      flags,
                      OSS_DEFAULTFILE,
                      obj->file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s, rc:%d", path, rc);
            goto error;
         }

         rc = ossGetFileSize(&(obj->file), &fileSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get file size[%s], rc:%d", path, rc);
            goto error;
         }

         if ((INT64)CONTROL_FILE_SIZE != fileSize)
         {
            PD_LOG(PDERROR, "invalid file size:%lld of control file[%s]",
                   fileSize, path);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         rc = readFile(obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file[%s], rc:%d", path, rc);
            goto error;
         }

         rc = validateBuf(obj->buf);
         if (SDB_VESSEL_PAGE_CRASHED == rc)
         {
            PD_LOG(PDERROR, "invalid file checksum:[%s], reinit it as unused one", path);
            initFileBuf(obj);
            rc = writeFile(obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to write file[%s], rc:%d", path, rc);
               goto error;
            }
            pushToUnusedListWhenOpen(obj);
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "invalid file head[%s]", path);
            goto error;
         }
         else if (obj->getHead()->isUnused())
         {
            pushToUnusedListWhenOpen(obj);
         }
         else
         {
            pushToWorkshopWhenOpen(obj);
         }
         obj = NULL;
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   INT32 controlFile::createFiles()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_dir.empty(), "can not be empty");
      UINT32 count = getMaxAliveVersionCount();
      CHAR path[OSS_MAX_PATHSIZE + 1] = {0};
      _fileObj *obj = NULL;
      UINT32 flags = OSS_CREATEONLY |OSS_READWRITE | OSS_EXCLUSIVE | OSS_WRITETHROUGH;

      for (UINT32 i = 0; i < count; ++i)
      {
         UINT32 *checksum = NULL;

         obj = SDB_OSS_NEW _fileObj();
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         obj->sequence = i;
         if (!getFileName(i, obj->name))
         {
            PD_LOG(PDERROR, "failed to get file name of sequence:%d", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = utilBuildFullPath(_dir.c_str(),
                                obj->name.c_str(),
                                OSS_MAX_PATHSIZE + 1,
                                path);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build file full path:%d", rc);
            goto error;
         }

         rc = ossOpen(path, flags, OSS_DEFAULTFILE, obj->file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create contril file[%s], rc:%d",
                   path, rc);
            goto error;
         }

         rc = ossExtendFile(&(obj->file), (INT64)CONTROL_FILE_SIZE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file[%s] to valid size, rc:%d",
                   path, rc);
            goto error;
         }

         initFileBuf(obj);
         rc = writeFile(obj);
         if (SDB_OK != rc)
         {
            goto error;
         }

         pushToUnusedListWhenOpen(obj);
         obj = NULL;
      }

   done:
      return rc;
   error:
      if (NULL != obj && obj->file.isOpened())
      {
         obj->close();
         ossDelete(path);
      }
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   void controlFile::initFileBuf(_fileObj *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      ossMemset(obj->buf, 0, CONTROL_FILE_SIZE);
      controlFile::head *head = (controlFile::head*)(obj->buf);

      head->magicCode = getMagicCode();
      head->headVerion = CONTROL_FILE_VERSION;
      head->flags = 0;
      head->commitVersion = INVALID_COMMIT_VERSION;
      head->updateMillis = ossGetCurrentMilliseconds();
      head->contentLen = 0;
      head->pad = 0;

      createChecksum(obj->buf);
      return;
   }

   void controlFile::updateFileBuf(_fileObj *obj, UINT32 size, const void *buf)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(NULL != buf, "can not be null");

      obj->getHead()->commitVersion = _commitVersion;
      obj->getHead()->updateMillis = ossGetCurrentMilliseconds();
      obj->getHead()->contentLen = size;
      ossMemcpy(obj->buf + sizeof(controlFile::head), buf, size);
      ossMemset(obj->buf + sizeof(controlFile::head) + size,
                0, CONTROL_FILE_SIZE - sizeof(controlFile::head) - size);
      ossMemcpy(obj->buf + sizeof(controlFile::head), buf, size);
      createChecksum(obj->buf);
      return;
   }

   INT32 controlFile::writeFile(_fileObj *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "can not be closed");
      SDB_ASSERT(!obj->name.empty(), "must be valid");
      SINT64 written = 0;
      
      do
      {
         SINT64 w = 0;
         rc = ossSeekAndWriteN(&(obj->file),
                               written,
                               (CHAR *)((ossValuePtr)(obj->buf) + written),
                               (SINT64)(CONTROL_FILE_SIZE - written),
                               w);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write file[%s], rc:%d", obj->name.c_str(), rc);
            goto error;
         }

         written += w;
      } while (written < CONTROL_FILE_SIZE);

      /// we set OSS_WRITETHROUGH when open file, no need to 
      /// fsync file again.
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readFile(_fileObj *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(!obj->name.empty(), "must be valid");
      SDB_ASSERT(obj->file.isOpened(), "must be open");
      SINT64 read = 0;

      do
      {
         SINT64 r = 0;
         rc = ossSeekAndReadN(&(obj->file), read,
                              CONTROL_FILE_SIZE - read,
                              (CHAR *)((ossValuePtr)(obj->buf) + read), r);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read content from file:%s, rc:%d",
                   obj->name.c_str(), rc);
            goto error;
         }
         read += r;
      }while (read < CONTROL_FILE_SIZE);
   done:
      return rc;
   error:
      goto done;
   }

   void controlFile::pushToUnusedListWhenOpen(_fileObj *obj)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "must be open");
      SDB_ASSERT(!obj->name.empty(), "must be valid");
      _unused.push_back(obj);
   done:
      return;
   }

   void controlFile::pushToWorkshopWhenOpen(_fileObj *obj)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->file.isOpened(), "must be open");
      SDB_ASSERT(!obj->name.empty(), "can not be invalid");
      SDB_ASSERT(CONTROL_FILE_VERSION == obj->getHead()->headVerion, "must be valid");
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