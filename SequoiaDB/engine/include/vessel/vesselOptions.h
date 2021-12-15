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
#include "utilCompression.hpp"
#include "vessel/collectionSpaceOptions.h"
#include "vessel/collectionOptions.h"

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
            lruOptions(){}

            lruOptions(const lruOptions &) = delete;
            lruOptions &operator=(const lruOptions &o)
            {
               ossMemcpy(this, &o, sizeof(lruOptions));
               return *this;
            }
            
                     
         public:
            UINT32 lruHotTouchCnt = 2;
            FLOAT32 lruColdPercent = 0.5;
            UINT32 lruMinSplitSize = 512;
            UINT32 lruScanDepth = 128; /// default scan depth when evicting
            FLOAT32 lruMaxScanPercent = 0.6; /// max scan depth when evicting
            INT32 lruFlushWaitLockTimeout = -1;
            UINT32 _lruColdMistakeTolerance = 10;
            UINT32 _lruTouchCountFrozenTime = 100;
      };// class lruOptions

      class freeListOptions : public SDBObject
      {
         public:
            freeListOptions(){}
            freeListOptions(const freeListOptions &) = delete;
            freeListOptions &operator=(const freeListOptions &o)
            {
               maxChunkCount = o.maxChunkCount;
               pageCountInChunk = o.pageCountInChunk;
               return *this;
            }

            std::string toString()const
            {
               return "";
            }

            UINT32 maxChunkCount = 128;
            UINT32 pageCountInChunk = 1024;
      };//class freeListOptions

      class bucketOptions : public SDBObject
      {
         public:
            bucketOptions(){}
            bucketOptions(const bucketOptions &) = delete;
            bucketOptions &operator=(const bucketOptions &o)
            {
               bucketCount = o.bucketCount;
               bucketLatchCount = o.bucketLatchCount;
               return *this;
            }

            UINT32 bucketCount = 16384;
            UINT32 bucketLatchCount = 512;
      };//class bucketOptions

      class flushOptions : public SDBObject
      {
         public:
            flushOptions(){}
            ~flushOptions(){}
            flushOptions(const flushOptions &) = delete;
            flushOptions &operator=(const flushOptions &o)
            {
               flushDirtyListThreshold = o.flushDirtyListThreshold;
               flushDirtyListTimeout = o.flushDirtyListTimeout;
               flushLruListThreshold = o.flushLruListThreshold;
               minTrimLRUDepth = o.minTrimLRUDepth;
               return *this;
            }

         public:
            FLOAT32 flushDirtyListThreshold = 1.2;
            UINT32 flushDirtyListTimeout = 300; /// seconds
            FLOAT32 flushLruListThreshold = 0.8;
            UINT32 minTrimLRUDepth = 128;
         
      };//class flushOptions

      public:
      liteCacheOptions(){}
      liteCacheOptions(const liteCacheOptions &) = delete;
      liteCacheOptions &operator=(const liteCacheOptions &o)
      {
         bucket = o.bucket;
         freelist = o.freelist;
         lru = o.lru;
         flush = o.flush;
         return *this;
      }
      
      
      bucketOptions bucket;
      freeListOptions freelist;
      lruOptions lru;
      flushOptions flush;
   }; /// end of class liteCacheOptions

   class storagePathOptions : public SDBObject
   {
      public:
         std::string dataPath;
         std::string indexPath;
         std::string lobPath;
         std::string lobMetaPath;
         std::string lsmPath;
         std::string snapshotPath;
         
         const std::string &autoGetIndexPath()const
         {
            return indexPath.empty() ? dataPath : indexPath;
         }
         const std::string &autoGetLobPath()const
         {
            return lobPath.empty() ? dataPath : lobPath;
         }

         BOOLEAN hasExclusiveIndexPath()const
         {
            return !indexPath.empty() && indexPath != dataPath;
         }
         
   };// class storageOptions

   class openDBOptions : public SDBObject
   {
      public:
      OSS_INLINE openDBOptions(){}
      OSS_INLINE ~openDBOptions(){}

      public:
         storagePathOptions path;
         
         BOOLEAN fullDumpPageLog = FALSE;
         BOOLEAN sparseExtendingFile = TRUE;

         liteCacheOptions cacheOptions;

         UINT32 cacheCleanerCount = 8;
         UINT32 commonBackgroundWorkers = 16;

         UINT32 lpidLatchMapBucketCount = 4096;
         UINT32 lpidLatchMapLatchCount = 256;

         UINT32 ridLatchMapBucketCount = 4096;
         UINT32 ridLatchMapLatchCount = 256;

         UINT32 indexLatchMapBucketCount = 4096;
         UINT32 indexLatchMapLatchCount = 256;
         
         ///invisible options.
         UINT32 _spaceLpidCacheBucketCount = 32;
         UINT32 _spaceLpidCacheBucketLatchCount = 16;

         
   }; /// end of class openDBOptions

   class closeDBOptions : public SDBObject
   {
      public:
         closeDBOptions(){}

      public:
         enum CLOSE_MODE
         {
            CLOSE_MODE_NORMAL = 0,
            CLOSE_MODE_IMMDIETE = 1,
         };//
      public:
         CLOSE_MODE closeMode = CLOSE_MODE_NORMAL;
   }; // class closeDBOptions

   class alterCSOptions : public SDBObject
   {

   }; // class alterCSOptions;

   class dropCSOptions : public SDBObject
   {

   }; // class dropCSOptions

   

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


   
} /// end of namespace vessel
} /// end of namespace engine
#endif
