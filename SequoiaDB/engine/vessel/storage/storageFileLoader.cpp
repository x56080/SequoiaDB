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

   Source File Name = storageFileLoader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileLoader.h"
#include "ossLikely.hpp"
#include "ossIO.hpp"

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
      _ALL_FILE_MAP::const_iterator itr = _all.begin();
      for (; itr != _all.end(); ++itr)
      {
         FILES_WITH_SPACE_TYPE *filesWithType = itr->second;
         if (nullptr != filesWithType)
         {
            delete filesWithType;
         }
      }
      _all.clear();
      return;
   }

   INT32 storageFileLoader::load(const strSlice &dir)
   {
      INT32 rc = SDB_OK;
      clear();
      if (OSS_UNLIKELY(dir.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _load(dir, INVALID_SPACE_TYPE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load files under dir[%s], rc:%d",
                dir.str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      clear();
      goto done;
   }

   INT32 storageFileLoader::_load(const strSlice &dir, SPACE_TYPE specifiedType)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");

   
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
         FILES_WITH_SPACE_TYPE *fileMap = nullptr;
         _ALL_FILE_MAP::iterator itr;
         storageFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDDEBUG, "not regular file", fileName.c_str());
            continue;
         }

         if (!fn.extract(fileNameSlice, TRUE))
         {
            //PD_LOG(PDDEBUG, "not storage file name:%s", fileName.c_str());
            continue;
         }
         else if (INVALID_SPACE_TYPE != specifiedType &&
                  fn.getSpaceType() != specifiedType)
         {
            PD_LOG(PDDEBUG, "not target file, file name:%s", fileName.c_str());
            continue;
         }


         if (_removeTmpFile &&
             FILE_SHADOW_SUFFIX_TMP == fn.getShadowSuffix())
         {
            PD_LOG(PDINFO, "remove tmp vessel file:%s",
                   dir_iter->path().string().c_str());
            //fs::remove(dir_iter->path());

            /// Use ossDelete to avoid exception catching.
            INT32 r = ossDelete(dir_iter->path().string().c_str());
            if (SDB_OK != r)
            {
               PD_LOG(PDERROR, "failed to create tmp file:%s, rc:%d",
                      dir_iter->path().string().c_str(), rc);
            }
            continue;
         }

         itr = _all.find(fn.getSpaceType());
         if (_all.end() == itr)
         {
            FILES_WITH_SPACE_TYPE *files = new FILES_WITH_SPACE_TYPE();
            if (OSS_UNLIKELY(nullptr == files))
            {
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }

            _all[fn.getSpaceType()] = files;
            fileMap = files;
         }
         else
         {
            fileMap = itr->second;
            SDB_ASSERT(nullptr != fileMap, "impossible");
         }
         
         (*fileMap)[fn.getFileType()].push_back(fn);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileLoader::append(const strSlice &dir, SPACE_TYPE filter)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(dir.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _load(dir, filter);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append files under[%s], rc:%d",
                dir.str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   const STORAGE_FILE_NAME_LIST *storageFileLoader::getFileList(SPACE_TYPE stype,
                                                                FILE_TYPE ftype)const
   {
      const STORAGE_FILE_NAME_LIST *fl = nullptr;
      _ALL_FILE_MAP::const_iterator itr = _all.find(stype);
      if (_all.end() != itr)
      {
         FILES_WITH_SPACE_TYPE *fileMap = itr->second;
         FILES_WITH_SPACE_TYPE::const_iterator subItr = fileMap->find(ftype);
         if (fileMap->end() != subItr && !subItr->second.empty())
         {
            fl = &(subItr->second);
         }
      }
      return fl;
   }
}//namespace vessel
}//namespace engin