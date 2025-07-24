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

   Source File Name = fileNameLoader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/15/2022  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
#include "oss.hpp"
#include "ossMemPool.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
namespace engine
{
namespace vessel
{
   class fileNameList : SDBObject
   {
   public:
      fileNameList() = default;
      fileNameList(const std::string &root);
      fileNameList(fileNameList &&o);
      fileNameList &operator=(fileNameList &&o);
      ossPoolList<std::string> getFullDirPaths() const;
      ossPoolList<std::string> getFullFilePaths(const std::string &prefix= "") const;
      void reset();
      void resetPaths();
      void setRoot(const std::string &root);
      const std::string& getRoot() const;
      void appendDirName(const std::string &);
      void appendFileName(const std::string &);

   private:
      std::string _root;
      ossPoolList<std::string> _dirNames;
      ossPoolList<std::string> _fileNames;
   };

   class fileNameLoader : public SDBObject
   {
   public:
      INT32 loadFileNameList(const std::string &rootPath, fileNameList &);
   };
} // namespace vessel
} // namespace engine