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

   Source File Name = dmsEngineOptions.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_ENGINE_OPTIONS_HPP_
#define SDB_DMS_ENGINE_OPTIONS_HPP_

#include "dmsEngineDef.hpp"
#include "interface/IRecordFilter.h"
#include "utilCompression.hpp"
#include "dms.hpp"
#include "../bson/bson.hpp"
#include "dmsStripingId.hpp"

namespace engine
{
   class dmsCreateCSOptions : public SDBObject
   {
      public:
         UINT32 dataPageSize = DMS_PAGE_SIZE32K;
         UINT32 idxPageSize = DMS_PAGE_SIZE32K;
         UINT32 lobdPageSize = DMS_PAGE_SIZE4K;
         DMS_STORAGE_TYPE stype = DMS_STORAGE_NORMAL;
   };//class dmsCreateCSOptions

   class dmsCreateCLOptions : public SDBObject
   {
      public:
         utilCLInnerID innerID = UTIL_UNIQUEID_NULL;
         UTIL_COMPRESSOR_TYPE compressor = UTIL_COMPRESSOR_INVALID;
         UINT8 pageMinFreePercent = 10;
   };//class dmsCreateCLOptions

   class dmsRemoveCLOptions : public SDBObject
   {

   };

   class dmsTruncateCLOptions : public SDBObject
   {
      
   };

   class dmsOpenCLOptions : public SDBObject
   {

   };////class dmsOpenCLOptions

   class dmsBuildIndexOptions : public SDBObject
   {
      public:
         dmsBuildIndexOptions &operator=(const dmsBuildIndexOptions &o)
         {
            sortBufferSize = o.sortBufferSize;
            blockDML = o.blockDML;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isSortingDisabled()const
         {
            return 0 == sortBufferSize;
         }

      public:
         UINT32 sortBufferSize = (UINT32)64 << 20; /// 64MB
         BOOLEAN blockDML = FALSE;
   };//class dmsBuildIndexOptions

   class dmsInsertRecordOptions : public SDBObject
   {
      public:
         dmsInsertRecordOptions &operator=(const dmsInsertRecordOptions &o)
         {
            stripingId = o.stripingId;
            return *this;
         }
      public:
         dmsStripingId stripingId;
   };//class dmsInsertRecordOptions

   class dmsUpdateRecordOptions : public SDBObject
   {
      public:
         dmsUpdateRecordOptions &operator=(const dmsUpdateRecordOptions &o)
         {
            stripingId = o.stripingId;
            return *this;
         }
      public:
         dmsStripingId stripingId;
   };//class dmsUpdateRecordOptions

   class dmsDeleteRecordOptions : public SDBObject
   {

   };//class dmsDeleteRecordOptions

   class dmsScanOptions : public SDBObject
   {
      public:
         dmsScanOptions &operator=(const dmsScanOptions &o)
         {
            scanFor = o.scanFor;
            rowCountLimit = o.rowCountLimit;
            filter = o.filter;
            pageStep = o.pageStep;
            return *this;
         }

      public:
         DMS_SCAN_FOR scanFor = DMS_SCAN_FOR_NONE;
         INT64 rowCountLimit = -1;
         IRecordFilter *filter = NULL;
         INT32 pageStep = -1;
   };//class dmsScanOptions

   class dmsIndexScanOptions : public SDBObject
   {
      public:
         dmsIndexScanOptions &operator=(const dmsIndexScanOptions &o)
         {
            scanFor = o.scanFor;
            rowCountLimit = o.rowCountLimit;
            indexCovered = o.indexCovered;
            forward = o.forward;
            return *this;
         }
         
      public:
         DMS_SCAN_FOR scanFor = DMS_SCAN_FOR_NONE;
         INT64 rowCountLimit = -1;
         BOOLEAN indexCovered = FALSE;
         BOOLEAN forward = TRUE;
      
   };//class dmsIndexScanOptions

   class dmsCreateDataSnapshotOptions
   {
      public:

   };//class dmsCreateDataSnapshotOptions

   class dmsListLobChunkOptions : public SDBObject
   {
      public:
         INT32 chunkId = -1;
   };//class dmsListLobChunkOptions
} // namespace engine


#endif//SDB_DMS_ENGINE_OPTIONS_HPP_
