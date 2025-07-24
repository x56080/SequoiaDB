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

   Source File Name = fileNameLoader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/15/2022  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
#include "vessel/fileNameLoader.h"

namespace engine
{
namespace vessel
{
   namespace fs = boost::filesystem;
   fileNameList::fileNameList(const std::string &root)
   {
      _root = root;
   }

   fileNameList::fileNameList(fileNameList &&o)
   {
      _root = std::move(o._root);
      _dirNames = std::move(o._dirNames);
      _fileNames = std::move(o._fileNames);
   }

   fileNameList &fileNameList::operator=(fileNameList &&o)
   {
      _root = std::move(o._root);
      _dirNames = std::move(o._dirNames);
      _fileNames = std::move(o._fileNames);
      return *this;
   }

   ossPoolList<std::string> fileNameList::getFullDirPaths() const
   {
      ossPoolList<std::string> output;
      fs::path root(_root.empty() ? fs::current_path() : _root);
      for (auto it = _dirNames.begin(); it != _dirNames.end(); it++)
      {
         fs::path dirName(*it);
         output.emplace_back((root / dirName).string());
      }
      return std::move(output);
   }

   ossPoolList<std::string> fileNameList::getFullFilePaths(const std::string &prefix) const
   {
      ossPoolList<std::string> output;
      fs::path root(_root.empty() ? fs::current_path() : _root);
      for (auto it = _fileNames.begin(); it != _fileNames.end(); it++)
      {
         if (0 == it->compare(0, prefix.size(), prefix))
         {
            fs::path fileName(*it);
            output.emplace_back((root / fileName).string());
         }
      }
      return std::move(output);
   }

   void fileNameList::reset()
   {
      _root.clear();
      _dirNames.clear();
      _fileNames.clear();
   }

   void fileNameList::resetPaths()
   {
      _dirNames.clear();
      _fileNames.clear();
   }

   void fileNameList::setRoot(const std::string &root)
   {
      _root = root;
   }

   const std::string &fileNameList::getRoot() const
   {
      return _root;
   }

   void fileNameList::appendDirName(const std::string &dirName)
   {
      _dirNames.emplace_back(dirName);
   }

   void fileNameList::appendFileName(const std::string &fileName)
   {
      _fileNames.emplace_back(fileName);
   }

   /////////////////////////////////////////////////////////////////////////////
   // Begin fileNameLoader

   INT32 fileNameLoader::loadFileNameList(const std::string &rootPath, fileNameList &l)
   {
      INT32 rc = SDB_OK;
      l.reset();
      if (rootPath.empty())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "root path can not be empty string");
         goto error;
      }
      else if (!fs::exists(rootPath) || !fs::is_directory(rootPath))
      {
         rc = SDB_FNE;
         PD_LOG(PDERROR, "the directory does not exist, dir: %s", rootPath.c_str());
         goto error;
      }
      else
      {
         fs::path dirPath(rootPath);
         l.setRoot(rootPath);
         for (fs::directory_iterator it(dirPath); it != fs::directory_iterator(); it++)
         {
            if (fs::is_regular_file(it->status()))
            {
               l.appendFileName(it->path().filename().string());
            }
            else if (fs::is_directory(it->status()))
            {
               l.appendDirName(it->path().filename().string());
            }
         }
      }
   done:
      return rc;
   error:
      l.reset();
      goto done;
   }
} // namespace vessel
} // namespace engine