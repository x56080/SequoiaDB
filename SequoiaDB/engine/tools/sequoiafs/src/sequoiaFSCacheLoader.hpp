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

   Source File Name = sequoiaFSMsgHandler.hpp

   Descriptive Name = sequoiafs meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSCACHELOADER_HPP__
#define __SEQUOIAFSCACHELOADER_HPP__

#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSDataCache.hpp"

using namespace engine;

#define EVENT_FAILED           -1
#define EVENT_DOWNLOAD_SUCCESS  0
#define EVENT_ALLOC_SUCCESS     1

namespace sequoiafs
{
   class sequoiaFSFileLobMgr;

   typedef boost::shared_ptr< ossEvent >            eventPtr ;
   class cacheLoader : public SDBObject
   {
      public:
         cacheLoader(sequoiaFSFileLobMgr* mgr)
         {
            _mgr = mgr;
         }
         virtual ~cacheLoader(){}
         void init(INT32 capacity)
         {
            _capacity = capacity;
            _capaCount = capacity/4;
         };
         INT32 load(UINT32 hashKey, INT32 flId, INT64 offset);
         INT32 loadEmpty(INT32 flId, INT64 offset);
         void preLoad(INT32 flId, INT64 offset);
         BOOLEAN preLoadCheck(INT32 flId, INT64 offset);
         void unPreLoad(INT32 flId, INT64 offset);
         UINT32 hash(INT32 flId, INT64 offset);
         void releaseCache(dataCache* node);
         UINT32 getUsedCount(){return _usedCount;}
         
     private:
        dataCache* _getNewCache(INT32 allocLen);

      private:
         ossPoolMap<int, eventPtr>        _loadMap;
         ossSpinXLatch                    _loadMapMutex;
         ossPoolSet<int>                  _preReadSet;   
         ossSpinXLatch                    _preSetMutex;

         sequoiaFSFileLobMgr* _mgr;
         INT32 _capacity;
         INT32 _capaCount;
         INT32 _usedCount;
         ossSpinXLatch _countMutex;
   } ;
}

#endif
