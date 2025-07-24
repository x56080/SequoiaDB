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

   Source File Name = multiControlFilesMgr.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/16/2022  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
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
   constexpr CHAR * const CONTROL_FILE_NAME_TYPE = "control";
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
      INT32 init(const std::string &prefix, const std::string &path, UINT32 maxValidFilesNum = CONTROL_FILES_DEFAULT_NUMBER);
      void reset();
      // Automatically look for control files with the name prefix in the directory path and load them.
      INT32 reload(BOOLEAN deleteInvalidFiles = TRUE);
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
      INT32 _deleteFile(const std::string &path);
      // Deleta the oldest files which go beyond max files number.
      void _deleteDeprecatedFiles();
      // Read content of current file.

   private:
      // The number of control files. Whenever _versions.size() > _filesNum, delete the oldest files.
      UINT32 _maxValidFilesNum = CONTROL_FILES_DEFAULT_NUMBER;
      // The name prefix of control files.
      std::string _prefix;
      // The directory where control files is located.
      std::string _dirPath;
      // The list of all file meta.
      ossPoolList<controlFileMeta> _fileMetaList;
      
   };
} // namespace vessel
} // namespace engine