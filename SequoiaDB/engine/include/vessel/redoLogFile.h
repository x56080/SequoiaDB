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

   Source File Name = redoLogFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_REDO_LOG_FILE_H_
#define VESSEL_REDO_LOG_FILE_H_

#include "ossIO.hpp"
#include "vessel/redoLogDef.h"
#include <memory>

namespace engine
{
namespace vessel
{
   struct redoFileInfo
   {
      redoFileInfo() = default;
      ~redoFileInfo() = default;
      redoFileInfo(redoFileInfo &&o) noexcept:
      path(std::move(o.path)),
      logicalId(o.logicalId),
      startLSN(o.startLSN)
      {
         o.reset();
      }

      redoFileInfo &operator=(redoFileInfo &&o) noexcept
      {
         path = std::move(o.path);
         logicalId = o.logicalId;
         startLSN = o.startLSN;
         o.reset();
         return *this;
      }

      void reset()
      {
         path.clear();
         logicalId = 0;
         startLSN = 0;
      }
      std::string path;
      UINT32 logicalId = 0;
      UINT64 startLSN = 0;
   };

   class redoLogFile : public SDBObject
   {
      public:
         redoLogFile() = default;
         ~redoLogFile();
         redoLogFile(const redoLogFile &) = delete;
         redoLogFile &operator=(const redoLogFile &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen() const { return _f.isOpened(); }
         OSS_INLINE const std::string &getPath() const { return _info.path; }
         OSS_INLINE UINT32 getLogicalId() const { return _info.logicalId; }
         OSS_INLINE UINT64 getStartLSN() const { return _info.startLSN; }
         OSS_INLINE UINT32 getFileBodySize() const { return RLOG_FILE_BODY_SIZE; }

      public:
         INT32 create(const CHAR *path, UINT32 logicalId);
         INT32 open(const CHAR *path);
         void close();
         void unlink();
         INT32 fsync();
         INT32 write(UINT32 offset, UINT32 size, const CHAR *data);
         INT32 read(UINT32 offset, UINT32 size, void *data) const;
         redoFileInfo closeAndExportInfo();

      private:
         INT32 _createFile();
         INT32 _parseFile(redoLogFileHeader &header);
         INT32 _initInfo(UINT32 logicalId, UINT64 startLSN, const CHAR *path);

         OSS_INLINE UINT32 _getRealOffset(UINT32 bodyOffset) const
         {
            return RLOG_FILE_HEAD_SIZE + bodyOffset;
         }

      private:
         _OSS_FILE _f;
         redoFileInfo _info;
   };//class redoLogFile

   using RLOG_FILE_UPTR = std::unique_ptr<redoLogFile>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_REDO_LOG_FILE_H_