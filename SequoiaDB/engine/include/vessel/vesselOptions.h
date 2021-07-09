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

   Source File Name = vesselOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VESSEL_OPTIONS_H_
#define VESSEL_VESSEL_OPTIONS_H_

#include "vessel/storageFileDef.h"
#include "vessel/collectionDef.h"
#include "utilCompression.hpp"
#include "vessel/insertOptions.h"

namespace engine
{
namespace vessel
{
   class liteCacheOptions : public SDBObject
   {
      public:
      class lruOptions : public SDBObject
      {
         public:
            OSS_INLINE lruOptions():
                     lruIncTouchCntWhenReadOnly(FALSE),
                     lruHotTouchCnt(2),
                     lruColdPercent(0.4),
                     lruMinSplitSize(512),
                     lruScanDepth(128),
                     lruMaxScanPercent(0.6),
                     lruFlushWaitLockTimeout(-1),
                     _lruColdMistakeTolerance(10){}
                     
         public:
            BOOLEAN lruIncTouchCntWhenReadOnly;
            UINT16 lruHotTouchCnt;
            FLOAT32 lruColdPercent;
            UINT32 lruMinSplitSize;
            UINT32 lruScanDepth; /// default scan depth when evicting or flushing
            FLOAT32 lruMaxScanPercent; /// max scan depth when evicting
            INT32 lruFlushWaitLockTimeout;
            UINT32 _lruColdMistakeTolerance;
      };// class lruOptions

      class freeListOptions : public SDBObject
      {
         public:
            OSS_INLINE freeListOptions():
            maxChunkCount(128),
            pageCountInChunk(1024)
            {

            }

            std::string toString()const
            {
               return "";
            }

            UINT32 maxChunkCount;
            UINT32 pageCountInChunk;
      };//class freeListOptions

      class bucketOptions : public SDBObject
      {
         public:
            OSS_INLINE bucketOptions():
            bucketCount(16384),
            bucketLatchCount(4096),
            minRecycleCount(16){}

            UINT32 bucketCount;
            UINT32 bucketLatchCount;
            UINT32 minRecycleCount;
      };//class bucketOptions

      public:
      OSS_INLINE liteCacheOptions()
      {}
      
      /// buckets
      
      bucketOptions bucket;
      freeListOptions freelist;
      lruOptions lru;
   }; /// end of class liteCacheOptions

   class storagePathOptions : public SDBObject
   {
      public:
         std::string dataPath;
         std::string indexPath;
         std::string lobPath;
         std::string lobMetaPath;

         const std::string &autoGetIndexPath()const
         {
            return indexPath.empty() ? dataPath : indexPath;
         }
         const std::string &autoGetLobPath()const
         {
            return lobPath.empty() ? dataPath : lobPath;
         }
   };// class storageOptions

   class openDBOptions : public SDBObject
   {
      public:
      OSS_INLINE openDBOptions(){}
      OSS_INLINE ~openDBOptions(){}

      public:
         storagePathOptions path;
         std::string snapshotPath;
         std::string lsmPath;

         BOOLEAN fullDumpPageLog = FALSE;
         BOOLEAN sparseExtendingFile = TRUE;

         liteCacheOptions cacheOptions;

         UINT32 lpidLatchMapBucketCount = 4096;
         UINT32 lpidLatchMapLatchCount = 4096;

         UINT32 ridLatchMapBucketCount = 4096;
         UINT32 ridLatchMapLatchCount = 4096;
         
         ///invisible options.
         UINT32 _spaceLpidCacheBucketCount = 64;
         UINT32 _spaceLpidCacheBucketLatchCount = 16;

         
   }; /// end of class openDBOptions

   class closeDBOptions : public SDBObject
   {
      public:
         closeDBOptions():
         flushDirtyList(TRUE)
         {}

      public:
         BOOLEAN flushDirtyList;
   }; // class closeDBOptions

   class createCSOptions : public SDBObject
   {
      public:
         OSS_INLINE createCSOptions(){}
         OSS_INLINE ~createCSOptions(){}
         BOOLEAN isValid()const;

         UINT32 dataPageSize = DMS_PAGE_SIZE32K;
         UINT32 dataSegSize = STORAGE_FILE_SEGMENT_SIZE_128MB;
         UINT32 idxPageSize = DMS_PAGE_SIZE16K;
         UINT32 idxSegSize = STORAGE_FILE_SEGMENT_SIZE_128MB;
         UINT32 lobPageSize = DMS_PAGE_SIZE256K;
         UINT32 lobSegSize = STORAGE_FILE_SEGMENT_SIZE_128MB;
         utilCSUniqueID uniqueID = UTIL_INVALID_CS_UNIQUE_ID;
      
   };/// end of class createCSOptions

   class alterCSOptions : public SDBObject
   {

   }; // class alterCSOptions;

   class dropCSOptions : public SDBObject
   {

   }; // class dropCSOptions

   class createCLOptions
   {
      public:
      OSS_INLINE createCLOptions():
      type(COLLECTION_TYPE_NORMAL),
      freeSizeReserved(4096),
      multiStripingBucket(FALSE),
      compressionType(UTIL_COMPRESSOR_INVALID),
      minStriping(INVALID_STRIPING_ID),
      maxStriping(INVALID_STRIPING_ID)
      {}

      OSS_INLINE ~createCLOptions(){}

      OSS_INLINE BOOLEAN isValid()const
      {
         return COLLECTION_TYPE_NORMAL == type;
      }

      public:
      UINT16 type;
      UINT16 freeSizeReserved;
      BOOLEAN multiStripingBucket;
      UTIL_COMPRESSOR_TYPE compressionType;
      STRIPING_ID minStriping;
      STRIPING_ID maxStriping; 
   };/// end of class createCLOptions

   class alterCLOptions
   {

   }; // class alterCLOptions

   class dropCLOptions
   {

   }; // class dropCLOptions;

   class openCLOptions : public SDBObject
   {
      public:
         OSS_INLINE openCLOptions()
         {}

         OSS_INLINE ~openCLOptions()
         {}

      public:
   };

   class updateOptions
   {

   }; /// end of class updateOptions

   class scanIndexOptions
   {}; // class scanIndexOptions

   class cursorOptions : public SDBObject
   {
      public:
         cursorOptions(){}
          ~cursorOptions(){}
         cursorOptions(const cursorOptions &) = delete;
         cursorOptions &operator=(const cursorOptions &o)
         {
            maxBufSize = o.maxBufSize;
            initBufSize = o.initBufSize;
            limit = o.limit;
            return *this;
         }

         ///cursor will try to extend buf only when the buf can not hold at
         /// least one slice.
         UINT32 maxBufSize = 16777216; /// 16MB
         UINT32 initBufSize = 65536;   /// 64KB
         UINT64 limit = OSS_UINT64_MAX;
   };
} /// end of namespace vessel
} /// end of namespace engine
#endif
