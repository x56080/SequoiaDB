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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/controlFile.h"
#include "ossLikely.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "vessel/vesselFileName.h"
#include "ossUtil.hpp"

namespace engine
{
namespace vessel
{
   const UINT64 VESSEL_CF_SEQ_INVALID = OSS_UINT64_MAX;
   const UINT64 VESSEL_CF_SEQ_UNUSED = 0;

   const UINT32 VALID_USER_CONTENT_SIZE = VESSEL_CONTROL_FILE_SIZE - sizeof(controlFile::head);
   const UINT16 CURRENT_CF_VERSION = 0;
   const UINT32 MAX_CF_SEQUENCE_WINDOW = 10;

   controlFile::controlFile()
   :_count(0),
    _maxSeqSlot(-1),
    _sequences(NULL),
    _buf(NULL),
    _files(NULL)
   {
      
   }

   controlFile::~controlFile()
   {
      close();
   }


   INT32 controlFile::open(const CHAR *path)
   {
      INT32 rc = SDB_OK;
      UINT32 count = getSeqWindow();
      UINT32 abnormal = 0;
      UINT32 unused = 0;
      UINT64 max = 0;

      if (NULL == path ||
          0 == count ||
          MAX_CF_SEQUENCE_WINDOW < count)
      {
         PD_LOG(PDERROR, "invalid args of control file creating");
         rc = SDB_INVALIDARG;
         goto error;
      }

       _count = count;
      rc = initMem(count);
      if (SDB_OK != rc)
      {
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         const head *head = NULL;
         INT64 readSize = 0;
         ossFile &f = _files[i];
         CHAR fileNO[2];
         ossItoa(i, fileNO, 2);
         std::string fullPath;
         fullPath.append(path)
                 .append(OSS_FILE_SEP)
                 .append(getName())
                 .append(".")
                 .append(VESSEL_CONTROL_FILE_SUFFIX)
                 .append(".")
                 .append(fileNO);
         rc = f.open(fullPath.c_str(),
                     OSS_CREATE | OSS_READWRITE | OSS_EXCLUSIVE,
                     OSS_DEFAULTFILE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open control file:%s, %d", fullPath.c_str(), rc);
            rc = SDB_OK;
            _sequences[i] = VESSEL_CF_SEQ_INVALID;
            ++abnormal;
            continue;
         }

         rc = f.readN(_buf + i * VESSEL_CONTROL_FILE_SIZE, VESSEL_CONTROL_FILE_SIZE, readSize);
         if (SDB_EOF == rc)
         {
            PD_LOG(PDERROR, "empty control file:%s", fullPath.c_str());
            _sequences[i] = VESSEL_CF_SEQ_UNUSED;
            ++unused;
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read control file:%s, %d", fullPath.c_str(), rc);
            rc = SDB_OK;
            _sequences[i] = VESSEL_CF_SEQ_INVALID;
            ++abnormal;
            continue;
         }
         else if (VESSEL_CONTROL_FILE_SIZE != readSize)
         {
            PD_LOG(PDERROR, "wrong content len in file:%s, len:%lld", fullPath.c_str(), readSize);
            rc = SDB_OK;
            _sequences[i] = VESSEL_CF_SEQ_INVALID;
            ++abnormal;
            continue;
         }

         head = getHead(i);
         if (!head->valid())
         {
            PD_LOG(PDERROR, "wrong content in file:%s", fullPath.c_str());
            rc = SDB_OK;
            _sequences[i] = VESSEL_CF_SEQ_INVALID;
            ++abnormal;
         }
         _sequences[i] = head->sequence;
         if (max < head->sequence)
         {
            max = head->sequence;
            _maxSeqSlot = i;
         }
      }

      if (count == abnormal)
      {
         PD_LOG(PDERROR, "all control files are broken!");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 controlFile::close()
   {
      INT32 rc = SDB_OK;
      if (NULL != _files)
      {
         for (UINT32 i = 0; i < _count; ++i)
         {
            ossFile &f = _files[i];
            if (f.isOpened())
            {
               f.close();
            }
         }

         SDB_OSS_DEL []_files;
         _files = NULL;
      }

      if (NULL != _buf)
      {
         SDB_OSS_FREE(_buf);
         _buf = NULL;
      }

      if (NULL != _sequences)
      {
         SDB_OSS_FREE(_sequences);
         _sequences = NULL;
      }

      _count = 0;
      _maxSeqSlot = -1;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::initMem(UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (NULL != _files)
      {
         PD_LOG(PDERROR, "control file has already been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      _files = SDB_OSS_NEW ossFile[count];
      if (NULL == _files)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      _buf = (CHAR *)SDB_OSS_MALLOC(count * VESSEL_CONTROL_FILE_SIZE);
      if (NULL == _buf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      ossMemset(_buf, 0, VESSEL_CONTROL_FILE_SIZE * count);

      _sequences = (UINT64 *)SDB_OSS_MALLOC(sizeof(UINT64) * count);
      if (NULL == _sequences)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      ossMemset(_buf, 0, sizeof(UINT64) * count);
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 controlFile::readLatestVersion(head *head, void *buf)const
   {
      INT32 rc = SDB_OK;
      const CHAR *data = NULL;
      const controlFile::head *cHead = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "control file has not been open");
         rc = SDB_INVALIDARG;
         goto error;
      }
      if (_maxSeqSlot < 0)
      {
         PD_LOG(PDERROR, "no commition yet");
         rc = SDB_VESSEL_CF_INVALID_SEQUENCE;
         goto error;
      }

      data = getBuf((UINT32)_maxSeqSlot);
      cHead = (const controlFile::head *)data;
      if (NULL != head)
      {
         *head = *cHead;
      }
      if (NULL != buf)
      {
         ossMemcpy(buf, data + sizeof(controlFile::head), cHead->contentLen);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::readOldestVersion(head *head, void *buf)const
   {
      INT32 rc = SDB_OK;
      UINT32 pos = 0;
      const CHAR *data = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "control file has not been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_maxSeqSlot < 0)
      {
         PD_LOG(PDERROR, "no commition yet");
         rc = SDB_VESSEL_CF_INVALID_SEQUENCE;
         goto error;
      }

      pos = (UINT32)_maxSeqSlot + 1;
      for (UINT32 i = 0; i < _count; ++i, ++pos)
      {
         UINT32 slot = pos % _count;
         UINT64 s = _sequences[slot];
         if (OSS_UNLIKELY(VESSEL_CF_SEQ_INVALID == s))
         {
            continue;
         }
         if (OSS_UNLIKELY(VESSEL_CF_SEQ_UNUSED == s))
         {
            continue;
         }

         data = getBuf(slot);
         const controlFile::head *cHead = (controlFile::head *)data;
         if (NULL != head)
         {
            *head = *cHead;
         }
         if (NULL != buf)
         {
            ossMemcpy(buf, data + sizeof(controlFile::head), cHead->contentLen);
         }
         break;

      }

      if (NULL == data)
      {
         PD_LOG(PDERROR, "all control files are broken!");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::read(UINT64 sequence, head *head, void *buf)const
   {
      INT32 rc = SDB_OK;
      const controlFile::head *cHead = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "control file has not been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(VESSEL_CF_SEQ_UNUSED == sequence ||
                       VESSEL_CF_SEQ_INVALID == sequence))
      {
         PD_LOG(PDERROR, "invalid sequence:%lld", sequence);
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _count; ++i)
      {
         if (sequence == _sequences[i])
         {
            const CHAR *data = getBuf(i);
            cHead = (controlFile::head *)data;
            if (NULL != head)
            {
               *head = *cHead;
            }
            if (NULL != buf)
            {
               ossMemcpy(buf, data + sizeof(controlFile::head), cHead->contentLen);
            }
         }
      }

      if (NULL == cHead)
      {
         rc = SDB_VESSEL_CF_INVALID_SEQUENCE;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::commit(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "control file has not been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = write(size, buf);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 controlFile::write(UINT32 size, const void *buf)
   {
      INT32 rc = SDB_OK;
      ossFile *file = NULL;
      CHAR pad[VESSEL_CONTROL_FILE_SIZE] = {0};
      UINT32 pos = 0;
      head *h = (head *)pad;

      if (OSS_UNLIKELY(0 == size ||
          VALID_USER_CONTENT_SIZE < size ||
          NULL == buf))
      {
         PD_LOG(PDERROR, "invalid size or buf");
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(pad + sizeof(controlFile::head), buf, size);
      h->version = CURRENT_CF_VERSION;
      h->userType = getUserType();
      h->updateTime = ossGetCurrentMilliseconds();
      h->contentLen = size;
      if (OSS_UNLIKELY(_maxSeqSlot < 0))
      {
         h->sequence = 1;
      }
      else
      {
         h->sequence = _sequences[_maxSeqSlot] + 1;
      }

      do
      {
         BOOLEAN r = getNextPosition(pos);
         if (!r)
         {
            PD_LOG(PDERROR, "all control files are broken!");
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         file = getFile(pos);
         SDB_ASSERT(NULL != file, "can not be null");
         rc = file->seekAndWriteN(0, pad, VESSEL_CONTROL_FILE_SIZE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write control file:%d, seq:%lld, path:%s",
                  rc, h->sequence, file->getPath().c_str());
            _sequences[pos] = VESSEL_CF_SEQ_INVALID;
            /// failed to write file, try to write a new one.
            continue;
         }

         rc = file->sync();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync control file:%d, seq:%lld, path:%s",
                  rc, h->sequence, file->getPath().c_str());
            _sequences[pos] = VESSEL_CF_SEQ_INVALID;
            continue;
         }

         _sequences[pos] = h->sequence;
         ossMemcpy(getBuf(pos), pad, sizeof(controlFile::head) + size);
         _maxSeqSlot = pos;
         break;
      } while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN controlFile::getNextPosition(UINT32 &p)const
   {
      UINT64 min = OSS_UINT64_MAX;
      INT32 pos = -1;
      INT32 res = -1;

      if (OSS_LIKELY(0 <= _maxSeqSlot))
      {
         pos = _maxSeqSlot + 1;
      }
      else
      {
         pos = 0;
      }
      
      for (UINT32 i = 0; i < _count; ++i, ++pos)
      {
         UINT32 slot = pos % _count;
         UINT64 s = _sequences[slot];
         if (OSS_UNLIKELY(VESSEL_CF_SEQ_INVALID == s))
         {
            continue;
         }
         else
         {
            res = slot;
            break;
         }  
      }

      if (-1 == res)
      {
         return FALSE;
      }
      p = res;
      return TRUE;
   }
} // namespace vessel
} // namespace engine