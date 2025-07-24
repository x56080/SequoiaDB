/*******************************************************************************

<<<<<<< HEAD
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
=======

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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   Source File Name = sequoiaFSHashBucket.hpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/
<<<<<<< HEAD
#ifndef __SEQUOIAFSHASHBUCKET_HPP__
#define __SEQUOIAFSHASHBUCKET_HPP__

#include <vector>
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"

#include "sequoiaFSDataCache.hpp"

using namespace engine;

namespace sequoiafs
{
   struct hashElem
   {
      public:
         INT64 _offset;
         INT64 _flId;

      public:
         hashElem(INT32 flId, INT64 offset)
         {
            _flId = flId;
            _offset = offset;
         }
   };
   
   struct dataHashItem
   {
      ossSpinSLatch _hashMutex;
      dataCache* hashHead;
   };

   class sequoiaFSFileLobMgr;
   class sequoiaFSHashBucket : public SDBObject
   {
      public:
         sequoiaFSHashBucket(sequoiaFSFileLobMgr * flMgr, UINT32 size = 1024)
         {
            _buckets.resize(size);
            _size = size;
            _flMgr = flMgr;
         }
         ~sequoiaFSHashBucket(){}
         UINT32 hash(INT32 flId, INT64 offset);
         void lockBucketR(UINT32 key);    
         BOOLEAN tryLockBucketR(UINT32 key);
         void unLockBucketR(UINT32 key);
         void lockBucketW(UINT32 key);    
         BOOLEAN tryLockBucketW(UINT32 key);
         void unLockBucketW(UINT32 key);
         BOOLEAN check(UINT32 key, dataCache* node);
         dataCache* get(UINT32 key, INT32 flId, INT64 offset);
         void add(UINT32 key, dataCache* node );
         void del(UINT32 key, dataCache* node );
         
         void cleanOverTimeCache();
         void setCacheClean(){needClean = TRUE;}
         INT32 getCallCount(){return _callCount;}
         INT32 getConflictCount(){return _conflictCount;}
         void cleanDataHashCounts()
         {
            _countMutex.get();
            _callCount = 0;
            _conflictCount = 0;
            _countMutex.release();
         }

      private:
         vector<dataHashItem> _buckets;
         UINT32 _size;
         sequoiaFSFileLobMgr * _flMgr;
         BOOLEAN needClean;

         INT32 _callCount;
         INT32 _conflictCount;
         ossSpinXLatch _countMutex;
   };
}

#endif
=======

#ifndef __SEQUOIAFSHASHBUCKET_HPP__
#define __SEQUOIAFSHASHBUCKET_HPP__

#include <vector>
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"

#include "sequoiaFSDataCache.hpp"

using namespace engine;

namespace sequoiafs
{
   struct hashElem
   {
      public:
         INT64 _offset;
         INT64 _flId;

      public:
         hashElem(INT32 flId, INT64 offset)
         {
            _flId = flId;
            _offset = offset;
         }
   };
   
   struct dataHashItem
   {
      ossSpinSLatch _hashMutex;
      dataCache* hashHead;
   };

   class sequoiaFSFileLobMgr;
   class sequoiaFSHashBucket : public SDBObject
   {
      public:
         sequoiaFSHashBucket(sequoiaFSFileLobMgr * flMgr, UINT32 size = 1024)
         {
            _buckets.resize(size);
            _size = size;
            _flMgr = flMgr;
         }
         ~sequoiaFSHashBucket(){}
         UINT32 hash(INT32 flId, INT64 offset);
         void lockBucketR(UINT32 key);    
         BOOLEAN tryLockBucketR(UINT32 key);
         void unLockBucketR(UINT32 key);
         void lockBucketW(UINT32 key);    
         BOOLEAN tryLockBucketW(UINT32 key);
         void unLockBucketW(UINT32 key);
         BOOLEAN check(UINT32 key, dataCache* node);
         dataCache* get(UINT32 key, INT32 flId, INT64 offset);
         void add(UINT32 key, dataCache* node );
         void del(UINT32 key, dataCache* node );
         
         void cleanOverTimeCache();
         void setCacheClean(){needClean = TRUE;}
         INT32 getCallCount(){return _callCount;}
         INT32 getConflictCount(){return _conflictCount;}
         void cleanDataHashCounts()
         {
            _countMutex.get();
            _callCount = 0;
            _conflictCount = 0;
            _countMutex.release();
         }

      private:
         vector<dataHashItem> _buckets;
         UINT32 _size;
         sequoiaFSFileLobMgr * _flMgr;
         BOOLEAN needClean;

         INT32 _callCount;
         INT32 _conflictCount;
         ossSpinXLatch _countMutex;
   };
}

#endif
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
