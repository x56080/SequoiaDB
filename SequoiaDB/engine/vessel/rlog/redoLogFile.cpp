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

   Source File Name = redoLogFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/redoLogFile.h"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ossEnvironment.hpp"
#include "vessel/redoLogDef.h"

namespace engine
{
namespace vessel
{
   redoLogFile::~redoLogFile()
   {
      if (_f.isOpened())
      {
         ossClose(_f);
      }
   }

   INT32 redoLogFile::create(const CHAR *path, UINT32 logicalId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reopen");

      if (OSS_UNLIKELY(nullptr == path || 0 == ossStrlen(path)))
      {
         PD_LOG(PDERROR, "invalid file path");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _initInfo(logicalId, (UINT64)logicalId * RLOG_FILE_BODY_SIZE, path);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file info:%d", rc);
         goto error;
      }

      rc = _createFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 redoLogFile::open(const CHAR *path)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reopen");
      constexpr UINT32 mode = OSS_READWRITE|OSS_EXCLUSIVE;
      constexpr UINT32 permission = OSS_RU|OSS_WU|OSS_RG;
      redoLogFileHeader header;

      if (OSS_UNLIKELY(nullptr == path || 0 == ossStrlen(path)))
      {
         PD_LOG(PDERROR, "invalid file path");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossOpen(path, mode, permission, _f);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%s, rc:%d", path, rc);
         goto error;
      }

      rc = _parseFile(header);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse file header[%s], rc:%d",
                path, rc);
         goto error;
      }

      rc = _initInfo(header.logicalId, header.startLSN, path);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file info:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void redoLogFile::close()
   {
      if (_f.isOpened())
      {
         ossClose(_f);
      }
      _info.reset();
      return;
   }

   void redoLogFile::unlink()
   {
      SDB_ASSERT(_f.isOpened() && !_info.path.empty(), "can not be invalid");
      ossClose(_f);
      INT32 rc = ossDelete(_info.path.c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file[%s], rc:%d",
                _info.path.c_str(), rc);
      }
      _info.reset();

      return;
   }

   INT32 redoLogFile::fsync()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = ossFdatasync(&_f);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync log file[%s], rc:%d", _info.path.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 redoLogFile::write(UINT32 offset, UINT32 size, const CHAR *data)
   {
      INT32 rc = SDB_OK;
      INT64 written = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(RLOG_FILE_BODY_SIZE < (offset + size)))
      {
         PD_LOG(PDERROR, "invalid block range to write[%d,%d]", offset, size);
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = ossSeekAndWriteN(&_f, _getRealOffset(offset), data, size, written);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write data into file[%s]:%d", _info.path.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 redoLogFile::read(UINT32 offset, UINT32 size, void *data) const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == data ||
                            RLOG_FILE_BODY_SIZE < (offset + size)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         SINT64 readN = 0;
         rc = ossSeekAndReadN(const_cast<OSSFILE*>(&_f),
                              _getRealOffset(offset), size, (CHAR *)data, readN);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file content:%d", rc);
            goto error;
         }
         else if (size != readN)
         {
            PD_LOG(PDERROR, "unexpected read size[%lld]", readN);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 redoLogFile::_createFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!_f.isOpened(), "do not reopen");
      SDB_ASSERT(!_info.path.empty(), "can not be empty");
      constexpr UINT32 mode = OSS_READWRITE|OSS_EXCLUSIVE|OSS_CREATEONLY;
      constexpr UINT32 permission = OSS_RU|OSS_WU|OSS_RG;
      ossEnv env;
      redoLogFileHeader header;
      header.init(RLOG_FILE_SIZE, _info.logicalId, _info.startLSN);
      const CHAR *data = reinterpret_cast<const CHAR *>(&header);
      SINT64 written = 0;

      rc = ossOpen(_info.path.c_str(), mode, permission, _f);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%s, rc:%d", _info.path.c_str(), rc);
         goto error;
      }

      rc = env.extendFile(_f, RLOG_FILE_SIZE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend redo file[%s], rc:%d",
                _info.path.c_str(), rc);
         goto error;
      }

      rc = ossSeekAndWriteN(&_f, 0, data, RLOG_FILE_HEAD_SIZE, written);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file[%s] header:%d",
                _info.path.c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      if (_f.isOpened())
      {
         ossClose(_f);
         INT32 trc = ossDelete(_info.path.c_str());
         if (SDB_OK != trc)
         {
            PD_LOG(PDSEVERE, "failed to remove file[%s], rc:%d",
                   _info.path.c_str(), trc);
            ossPanic();
         }
      }
      close();
      goto done;
   }

   INT32 redoLogFile::_parseFile(redoLogFileHeader &header)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_f.isOpened(), "must be open");
      CHAR *data = reinterpret_cast<CHAR *>(&header);
      SINT64 read = 0;
      INT64 fileSize = 0;

      rc = ossSeekAndReadN(&_f, 0, RLOG_FILE_HEAD_SIZE, data, read);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read file header:%d", rc);
         goto error;
      }

      if (RLOG_FILE_HEAD_SIZE != read)
      {
         PD_LOG(PDERROR, "invalid read size[%lld]", read);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!header.validate())
      {
         PD_LOG(PDERROR, "failed to validate file header");
         rc = SDB_VESSEL_INVALID_FILE;
         goto error;
      }
      
      rc = ossGetFileSize(&_f, &fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      if (fileSize != header.fileSize)
      {
         PD_LOG(PDERROR, "file size[%lld] does not match file header", fileSize);
         rc = SDB_VESSEL_INVALID_FILE;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 redoLogFile::_initInfo(UINT32 logicalId, UINT64 startLSN, const CHAR *path)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != path, "can not be invalid");
      try
      {
         _info.path = path;
      }
      catch(std::exception& e)
      {
         PD_LOG(PDERROR, "unexpected exception:%s", e.what());
         rc = ossException2RC(&e);
         goto error;
      }

      _info.logicalId = logicalId;
      _info.startLSN = startLSN;
   done:
      return rc;
   error:
      _info.reset();
      goto done;
   }

   redoFileInfo redoLogFile::closeAndExportInfo()
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      if (_f.isOpened())
      {
         ossClose(_f);
      }
      return std::move(_info);
   }
} // namespace vessel

} // namespace engine
