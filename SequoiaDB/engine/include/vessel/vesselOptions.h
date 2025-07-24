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

   Source File Name = vesselOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VESSEL_OPTIONS_H_
#define VESSEL_VESSEL_OPTIONS_H_

#include "utilCompression.hpp"
#include "vessel/collectionOptions.h"
#include "vessel/bufferPoolOptions.h"
#include "vessel/lsm/lsmDBOptions.h"
#include "vessel/hitRateLimitOptions.h"

namespace engine
{
namespace vessel
{
   struct storagePathOptions : public SDBObject
   {
      std::string dataPath;
      std::string indexPath;
      std::string lobdPath;
      std::string lobmPath;
      std::string lsmPath;
      std::string snapshotPath;

      const std::string &autoGetIndexPath()const
      {
         return indexPath.empty() ? dataPath : indexPath;
      }         
      const std::string &autoGetLobmPath()const
      {
         return lobmPath.empty() ? dataPath : lobmPath;
      }
      const std::string &autoGetLobdPath()const
      {
         return lobdPath.empty() ? dataPath : lobdPath;
      }
   };// class storageOptions

   class openDBOptions : public SDBObject
   {
      public:
         storagePathOptions path;
         
         BOOLEAN fullDumpPageLog = FALSE;

         liteBufferPoolOptions bufferPoolOptions;

         UINT32 cacheCleanerCount = 8;
         UINT32 hitTransferWorkerCount = 4;

         UINT32 lpidLatchMapBucketCount = 4096;
         UINT32 lpidLatchMapLatchCount = 256;

         UINT32 ridLatchMapBucketCount = 4096;
         UINT32 ridLatchMapLatchCount = 256;

         UINT32 indexLatchMapBucketCount = 4096;
         UINT32 indexLatchMapLatchCount = 256;

         UINT32 lobcLatchBucketCount = 4096;
         UINT32 lobcLatchMapLatchCount = 256;

         lobcBufferPoolOptions lobcPoolOptions;
         lsmDBOptions lsmOptions;
         hitRateLimitOptions limitOptions;
   }; /// end of class openDBOptions

   class closeDBOptions : public SDBObject
   {
      public:
         enum CLOSE_MODE
         {
            CLOSE_MODE_NORMAL = 0,
            CLOSE_MODE_IMMDIETE = 1,
         };//
      public:
         CLOSE_MODE closeMode = CLOSE_MODE_NORMAL;
   }; // class closeDBOptions
   
} /// end of namespace vessel
} /// end of namespace engine
#endif
