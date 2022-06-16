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

   Source File Name = multiControlFilesMgr.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/16/2022  ZHY Initial Draft
   Last Changed =

******************************************************************************/

#include "vessel/controlFile.h"
#include "vessel/memoryBlock.h"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <string>

namespace engine
{
namespace vessel
{
   constexpr UINT32 CONTROL_FILES_DEFAULT_NUMBER = 1;
   constexpr UINT32 CONTROL_FILE_NAME_FORMAT_COLUMNS = 2;
   // control file name: <prefix>.<version suffix>  eg: cfprefix.000001
   constexpr UINT32 CONTROL_FILE_MAX_NAME_LEN = 64;
   // Will not supplement zero if sequence has more than 6 digits.
   constexpr UINT32 CONTROL_FILE_MIN_VERSION_SUFFIX_LEN = 6;
   constexpr CHAR * const CONTROL_FILE_FILEDNAME_COMMIT_VERSION = "CommitVersion";
   constexpr CHAR * const CONTROL_FILE_FILEDNAME_PATH = "Path";
   constexpr CHAR * const CONTROL_FILE_FILEDNAME_CONTENT_LENGTH = "ContentLen";
   constexpr CHAR * const CONTROL_FILE_FILEDNAME_CREATION_TIME = "CreationTime";
   class multiControlFilesMgr : public SDBObject
   {
   public:
      multiControlFilesMgr() = default;
      ~multiControlFilesMgr() = default;
      multiControlFilesMgr(const multiControlFilesMgr&) = delete;
      multiControlFilesMgr &operator=(const multiControlFilesMgr&) = delete;

   private:
      struct controlFileMeta
      {
         controlFileMeta(UINT32 commitVersion, const std::string &path, UINT32 contentLen, UINT64 creationTime)
         {
            this->commitVersion = commitVersion;
            this->path = path;
            this->contentLen = contentLen;
            this->creationTime = creationTime;
         }
         
         UINT32 commitVersion = 0;
         std::string path;
         UINT32 contentLen = 0;
         UINT64 creationTime = 0;
      };

   public:
      void reset();
      // Automatically look for control files with the name prefix in the directory path and load them.
      INT32 load(const std::string &prefix, const std::string &path, BOOLEAN deleteInvalidFiles = TRUE);
      // Set the value of the number of valid control files.
      void setMaxValidFilesNum(UINT32);
      // Create control file with content with buffer.
      INT32 createFile(const CHAR *buf, UINT32 bufSize);
      // Return a list of all file meta.
      void list(ossPoolList<bson::BSONObj> &) const;
      // Return the number of valid files.
      UINT32 getValidFilesNum() const;
      INT32 getCurrentVersionFileContent(memoryBlock &) const;

   private:
      boost::filesystem::path _getPathFromVersion(UINT32 version) const;
      UINT32 _getNextVersion() const;
      BOOLEAN _validateAndExtract(const std::string &path, UINT32 &version) const;
      void _pushBackFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime);
      void _pushFrontFileMeta(UINT32 version, const std::string &path, UINT32 contentLen, UINT64 creationTime);
      // Scan files with specified prefix in specified directory.
      INT32 _scanFiles(BOOLEAN deleteInvalidFiles);
      // Deleta file with specified version.
      INT32 _deleteFile(UINT32 version);
      // Deleta the oldest files which go beyond max files number.
      INT32 _deleteDeprecatedFiles();
      // Read content of current file.

   private:
      // Whether the mgr has been loaded
      BOOLEAN _isLoaded = FALSE;
      // The number of control files. Whenever _versions.size() > _filesNum, delete the oldest files.
      UINT32 _filesNumMax = CONTROL_FILES_DEFAULT_NUMBER;
      // The name prefix of control files.
      std::string _prefix;
      // The directory where control files is located.
      std::string _dirPath;
      // The list of all file meta.
      std::list<controlFileMeta> _fileMetaList;
      
   };
} // namespace vessel
} // namespace engine