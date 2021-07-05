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

   Source File Name = storageFileLoader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageFileLoader.h"
#include "ossLikely.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageFileLoader::storageFileLoader()
   {}

   storageFileLoader::~storageFileLoader()
   {
      clear();
   }

   void storageFileLoader::clear()
   {
      _map.clear();
      return;
   }

   INT32 storageFileLoader::load(const strSlice &dir,
                                 SPACE_ID sid,
                                 SPACE_TYPE type,
                                 BOOLEAN removeTmpFile)
   {
      INT32 rc = SDB_OK;
      clear();
      if (OSS_UNLIKELY(dir.empty() ||
                       INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
      fs::directory_iterator end_iter ;
      fs::path dataDir(dir.str());
      if (!fs::exists(dataDir) ||
          !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid dir path to load:%s", dir.str());
         rc = SDB_FNE;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         strSlice fileNameSlice(fileName.c_str(), fileName.size());
         vesselFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDDEBUG, "not regular file", fileName.c_str());
            continue;
         }

         if (!fn.extract(fileNameSlice, TRUE))
         {
            PD_LOG(PDDEBUG, "invalid file name:%s", fileName.c_str());
            continue;
         }

         if (fn.getSpaceID() != sid ||
             fn.getSpaceType() != type)
         {
            PD_LOG(PDDEBUG, "not target file, file name:%s", fileName.c_str());
            continue;
         }

         if (removeTmpFile &&
             FILE_SHADOW_SUFFIX_TMP == fn.getShadowSuffix())
         {
            PD_LOG(PDINFO, "remove tmp vessel file:%s",
                   dir_iter->path().string().c_str());
            fs::remove(dir_iter->path());
            continue;
         }
         _map[fn.getFileType()].push_back(fn);
      }
      }
   done:
      return rc;
   error:
      clear();
      goto done;
   }

   const FILE_NAME_LIST *storageFileLoader::getFileList(FILE_TYPE type)const
   {
      const FILE_NAME_LIST *fl = NULL;
      _FILES_MAP::const_iterator itr = _map.find(type);
      if (_map.end() != itr)
      {
         fl = &(itr->second);
      }
      return fl;
   }
}//namespace vessel
}//namespace engine