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

   Source File Name = rlogFileManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RLOG_FILE_MANAGER_H_
#define VESSEL_RLOG_FILE_MANAGER_H_

#include "vessel/redoLogFile.h"
#include "ossMemPool.hpp"
#include "vessel/redoLogDef.h"
#include "vessel/redoLogBuffer.h"
#include <mutex>

namespace engine
{
namespace vessel
{
   class rlogFileManager : public SDBObject
   {
      public:
         rlogFileManager() = default;
         ~rlogFileManager();
         rlogFileManager(const rlogFileManager &) = delete;
         rlogFileManager &operator=(const rlogFileManager &) = delete;

      public:
         INT32 init(const CHAR *dir, UINT64 writingOffset, redoLogFilesSummary &summary);
         void fini();

         BOOLEAN isLogFileToBeSwitched(UINT64 lsn,
                                       UINT32 recordSize,
                                       UINT32 &fillingSize) const;

         UINT32 getFileLastFreeSize(UINT64 lsn, UINT32 recordSize) const;

         INT32 prepareFile(UINT64 offset);

         INT32 write(redoLogBuffer &buffer, BOOLEAN &fileIsFull);

         INT32 switchWorkingFile();

         INT32 flush();

         UINT64 getMinLSN();

      public:
         ///WARNING: not runtime apis!

         INT32 exportFiles(ossPoolVector<const redoLogFile *> &files);

      public:
         OSS_INLINE BOOLEAN isTheFirstLSNInFile(UINT64 lsn) const
         {
            return 0 == lsn % RLOG_FILE_BODY_SIZE;
         }
         OSS_INLINE UINT64 getFirstFileLSN(UINT32 fileId) const
         {
            return (UINT64)fileId * RLOG_FILE_BODY_SIZE;
         }
         OSS_INLINE UINT64 getWritingOffset() const
         {
            return _writingOffset;
         }
         OSS_INLINE UINT64 getDirtyOffset() const
         {
            return _dirtyOffset;
         }
         OSS_INLINE UINT64 getDirtySize() const
         {
            return _writingOffset - _dirtyOffset;
         }

      private:
         using _FILE_OBJ_MAP = ossPoolMap<UINT32, RLOG_FILE_UPTR>;
         using _FILE_OBJ_LIST = ossPoolList<RLOG_FILE_UPTR>;

      private:
         INT32 _preload(const CHAR *dir, _FILE_OBJ_MAP &fmap);
         INT32 _load(UINT64 writingOffset, _FILE_OBJ_MAP &fmap, redoLogFilesSummary &summary);
         BOOLEAN _hasMissingFiles(UINT32 workingFileId, const _FILE_OBJ_MAP &fmap) const;
         INT32 _createFullPath(UINT32 logicalId, ossPoolString &path) const;
         INT32 _createPoolFile();
         INT32 _pushWorkingFileIntoReadonlyList();
         INT32 _flushWorkingFile();
         BOOLEAN _setUpWorkingFile();
         INT32 _extractCurrentLSN(UINT64 lsnUpBound, UINT64 &lsn) const;

      private:
         void _closeAndClear(_FILE_OBJ_LIST &fl);
         INT32 _classifyFiles(UINT32 workingFileId,
                              _FILE_OBJ_MAP &fmap);
      private:
         OSS_INLINE UINT32 _getFileId(UINT64 offset) const
         {
            return offset / RLOG_FILE_BODY_SIZE;
         }
         OSS_INLINE BOOLEAN _isWorkingFileDirty() const
         {
            return _dirtyOffset < _writingOffset;
         }

      private:
         std::string _logDir;
         UINT64 _writingOffset = 0;
         UINT64 _dirtyOffset = 0;
         UINT32 _nextFileId = 0;
         RLOG_FILE_UPTR _workingFile;
         std::mutex _rmutex;
         _FILE_OBJ_LIST _readonlyFiles;
         std::mutex _poolMutex;
         _FILE_OBJ_LIST _pool;
   };//class rlogFileManager
} // namespace vessel

} // namespace engine


#endif//VESSEL_RLOG_FILE_MANAGER_H_