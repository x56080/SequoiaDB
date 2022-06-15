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

   Source File Name = fileNameLoader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/15/2022  ZHY Initial Draft
   Last Changed =

******************************************************************************/

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