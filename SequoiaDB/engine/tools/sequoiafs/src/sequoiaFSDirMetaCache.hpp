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

   Source File Name = sequoiaFSDirMetaCache.cpp

   Descriptive Name = sequoiafs file meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSDIRMETACACHE_HPP__
#define __SEQUOIAFSDIRMETACACHE_HPP__

#include "sequoiaFSMetaHashBucket.hpp"

#include "sequoiaFSLRUList.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSDao.hpp"

#include "ossRWMutex.hpp"

using namespace sdbclient;
using namespace engine;

namespace sequoiafs
{
   class fsMetaCache;
   class dirMetaCache : public SDBObject
   {
      public:
         dirMetaCache(fsMetaCache* mCache);
         ~dirMetaCache();
         INT32 init(sdbConnectionPool* ds, string dirCLName, INT32 capacity, BOOLEAN standalone);
         
         INT32 getDirInfo(INT64 parentId, const CHAR* dirName, dirMeta &meta);
         
         void releaseAllLocalCache(BOOLEAN isReleseLock);
         INT32 releaseLocalDirCache(INT64 parentId, const CHAR* dirName);
         sequoiaFSMetaHashBucket* getMetaHashBucket(){return &_dirHashBucket;} 
         LRUList* getLRUList(){return &_dirList;}

      private:
         INT32 _addDirInfo(dirMeta &meta);
         INT32 _delDirInfo(dirMetaNode* meta);
         INT32 _updateConnection();
         void _releaseLock(INT64 parentId, const CHAR* dirName);
         
      private:      
         sdbConnectionPool*         _ds;
         string _dirCLName;
         BOOLEAN _standalone;

         fsMetaCache*           _mCache;
         LRUList                _dirList;
         sequoiaFSMetaHashBucket _dirHashBucket;
         ossSpinXLatch          _listMutex;
         fsConnectionDao*       _dirDB;
         sdbCollection          _dMetaCL;
         
         ossSpinXLatch _dirDBMutex;
   };
}

#endif
