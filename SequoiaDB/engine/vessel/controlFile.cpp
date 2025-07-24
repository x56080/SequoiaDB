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

   Source File Name = controlFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/controlFile.h"
#include "utilCRC.hpp"
#include "ossLikely.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   OSS_INLINE UINT32 getMagicCode()
   {
      static const CHAR a[4] = {'S', 'D', 'B', 'V'};
      return *((UINT32 *)a);
   }

   controlFile::~controlFile()
   {
      close();
   }

   INT32 controlFile::create(const strSlice &fullPath,
                             const CHAR *buf,
                             UINT32 bufSize,
                             BOOLEAN replace,
                             BOOLEAN chmod,
                             UINT32* contentLen,
                             UINT64* creationTime)
   {
      INT32 rc = SDB_OK;
      UINT32 mode = 0;
      OSSFILE file;
      CHAR *fileBuf = nullptr;
      UINT32 fileSize = 0;
      controlFileHead *headPtr = nullptr;
      strictBuffer buffer;
      BOOLEAN fileCreated = FALSE;
      if (nullptr != contentLen)
      {
         *contentLen = 0;
      }
      if (nullptr != creationTime)
      {
         *creationTime = 0;
      }

      if (OSS_UNLIKELY(fullPath.empty() ||
                       nullptr == buf ||
                       0 == bufSize))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (replace)
      {
         mode = OSS_CREATE | OSS_READWRITE | OSS_EXCLUSIVE;
      }
      else
      {
         mode = OSS_CREATEONLY | OSS_READWRITE | OSS_EXCLUSIVE;
      }

      rc = ossOpen(fullPath.str(), mode, OSS_DEFAULTFILE, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create control file[%s], rc:%d",
                fullPath.str(), rc);
         goto error;
      }
      fileCreated = TRUE;

      fileSize = sizeof(controlFileHead) + bufSize;
      fileBuf = (CHAR *)SDB_THREAD_ALLOC(fileSize);
      if (OSS_UNLIKELY(nullptr == fileBuf))
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      buffer.makeWritable(fileSize, fileBuf);
      headPtr = buffer.getWritableObjPtr<controlFileHead>(0);
      if (OSS_UNLIKELY(nullptr == headPtr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get writable head ptr");
         goto error;
      }

      headPtr->magicCode = getMagicCode();
      headPtr->headVersion = CONTROL_FILE_HEAD_VERSION;
      headPtr->creationTime = ossGetCurrentMilliseconds();
      headPtr->contentLen = bufSize;
      rc = buffer.write(sizeof(controlFileHead), bufSize, buf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write content, rc:%d", rc);
         goto error;
      }

      if (nullptr != contentLen)
      {
         *contentLen = headPtr->contentLen;
      }
      if (nullptr != creationTime)
      {
         *creationTime = headPtr->creationTime;
      }
      // The first 8 bytes of the file header do not be calculated for checksum
      headPtr->checksum = utilCRC32(buffer.getReadablePtr(8, fileSize - 8), fileSize - 8);

      rc = ossWriteN(&file, buffer.getReadablePtr(0, fileSize), fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write control file, rc:%d", rc);
         goto error;
      }

      rc = ossFdatasync(&file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fdatasync file[%s], rc:%d", fullPath.str(), rc);
         goto error;
      }

      ossClose(file);
      if (chmod)
      {
         rc = ossChmod(fullPath.str(), OSS_RU);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to change file mode, rc:%d", rc);
            goto error;
         }
      }

   done:
      if (fileBuf)
      {
         SDB_THREAD_FREE(fileBuf);
      }
      if (file.isOpened())
      {
         ossClose(file);
      }
      return rc;
   error:
      if (fileCreated)
      {
         if (file.isOpened())
         {
            ossClose(file);
         }
         INT32 tmprc = SDB_OK;
         tmprc = ossDelete(fullPath.str());
         if (SDB_OK != tmprc)
         {
            PD_LOG(PDERROR, "failed to delete control file[%s], rc:%d",fullPath.str(), tmprc);
         }
      }
      goto done;
   }

   INT32 controlFile::openToRead(const strSlice &fullPath,
                                 invalidFileReason &reason)
   {
      INT32 rc = SDB_OK;
      INT64 lenRead = 0;
      CHAR* fileBuf = nullptr;
      INT64 fileSize = 0;
      strictBuffer buffer;

      reason = invalidFileReason::NONE;

      close();
      if (OSS_UNLIKELY(fullPath.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossOpen(fullPath.str(), OSS_READONLY, OSS_RU, _file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open control file[%s], rc:%d", 
                fullPath.str(), rc);
         goto error;
      }

      rc = ossGetFileSize(&_file, &fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get size of file[%s], rc:%d",
                fullPath.str(), rc);
         goto error;
      }

      if (fileSize < (INT64)sizeof(controlFileHead))
      {
         PD_LOG(PDERROR, "invalid size[%lld]", fileSize);
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         reason = invalidFileReason::INVALID_HEADER_SIZE;
         goto error;
      }
      else if ((INT64)MAX_CONTROL_FILE_LEN < fileSize)
      {
         PD_LOG(PDERROR, "invalid size[%lld]", fileSize);
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         reason = invalidFileReason::UNEXPECTED_FILE_SIZE;
         goto error;
      }

      fileBuf = (CHAR *)SDB_THREAD_ALLOC(fileSize);
      if (OSS_UNLIKELY(nullptr == fileBuf))
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      rc = ossReadN(&_file, fileSize, fileBuf, lenRead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read control file, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY((SINT64)fileSize != lenRead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "unexpected read length[%lld]", lenRead);
         goto error;
      }
      
      rc = validateFile(fileBuf, fileSize, reason);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "invalid control file, rc:%d", rc);
         goto error;
      }

      buffer.reset(fileSize, fileBuf);
      _head = *(buffer.getReadableObjPtr<controlFileHead>(0));

   done:
      if (fileBuf)
      {
         SDB_THREAD_FREE(fileBuf);
      }
      return rc;
   error:
      close();
      if (!fullPath.empty())
      {
         PD_LOG(PDERROR, "failed to open control file[%s], rc:%d, reason[%d]",
                fullPath.str(), rc, (INT32)reason);
      }
      goto done;
   }

   void controlFile::close()
   {
      if (_file.isOpened())
      {
         ossClose(_file);
         _head = controlFileHead();
      }
   }

   INT32 controlFile::read(CHAR *buf, UINT32 size)
   {
      INT32 rc = SDB_OK;
      INT64 lenRead = 0;

      if (!_file.isOpened())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         PD_LOG(PDERROR, "control file is not opened, rc:%d", rc);
         goto error;
      }
      else if (_head.contentLen > size)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid buffer size, rc:%d", rc);
         goto error;
      }

      rc = ossSeekAndReadN(&_file, sizeof(controlFileHead), (SINT64)size, buf, lenRead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read content from control file, rc:%d", rc);
         goto error;
      }
      else if (OSS_UNLIKELY((INT64)size < lenRead))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "unexpected read length[%lld]", lenRead);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 controlFile::validateFile(const CHAR *buf,
                                   UINT32 size, 
                                   invalidFileReason &reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != buf, "can not be null");
      SDB_ASSERT(sizeof(controlFileHead) <= size, "invalid file size");
      SDB_ASSERT(MAX_CONTROL_FILE_LEN >= size, "invalid file size");
      strictBuffer buffer;
      UINT32 checkSize = 0;

      buffer.reset(size, buf);
      const controlFileHead *head = buffer.getReadableObjPtr<controlFileHead>(0);
      if (OSS_UNLIKELY(nullptr == head))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         PD_LOG(PDERROR, "failed to get readable ptr");
         goto error;
      }

      if (head->magicCode != getMagicCode())
      {
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         PD_LOG(PDERROR, "invalid magic code");
         reason = invalidFileReason::INVLAID_HEADER_MAGIC_CHARS;
         goto error;
      }

      if (head->headVersion != CONTROL_FILE_HEAD_VERSION)
      {
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         PD_LOG(PDERROR, "invalid head version");
         reason = invalidFileReason::UNEXPECTED_HEADER_VERSION;
         goto error;
      }

      if (head->contentLen != (size - sizeof(controlFileHead)))
      {
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         PD_LOG(PDERROR, "invalid content length");
         reason = invalidFileReason::UNEXPECTED_FILE_SIZE;
         goto error;
      }

      /// 8 is the size of data size not to check, 4 bytes magic code and 4 bytes checksum
      checkSize = sizeof(controlFileHead) + head->contentLen - 8;
      if (head->checksum != utilCRC32(buffer.getReadablePtr(8, checkSize), checkSize))
      {
         rc = SDB_VESSEL_INVALID_CONTROL_FILE;
         PD_LOG(PDERROR, "invalid checksum");
         reason = invalidFileReason::INVALID_HEADER_CHECKSUM;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 controlFile::getContentLen()const
   {
      SDB_ASSERT(_file.isOpened(), "can not be closed");
      return _head.contentLen;
   }

   UINT64 controlFile::getCreationTime()const
   {
      SDB_ASSERT(_file.isOpened(), "can not be closed");
      return _head.creationTime;
   }

} // namespace vessel
} // namespace engine