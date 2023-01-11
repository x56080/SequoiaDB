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
#include "interface/IDataProtectionService.h"
#include "interface/IRecordFilter.h"
#include "utilCompression.hpp"
#include "dms.hpp"
#include "../bson/bson.hpp"
#include "utilResult.hpp"
#include "dmsStripingId.hpp"

namespace engine
{
   class _dmsDropCSOptions;
   using dmsDropCSOptions = _dmsDropCSOptions;
   class _dmsDropCLOptions;
   using dmsDropCLOptions = _dmsDropCLOptions;
   class _dmsReturnOptions;
   using dmsReturnOptions = _dmsReturnOptions;
   class _dmsIdxTaskStatus;
   using dmsIdxTaskStatus = _dmsIdxTaskStatus;
   
   struct dmsCreateCSOptions : public SDBObject
   {
      UINT32 dataPageSize = DMS_PAGE_SIZE_DFT;
      UINT32 idxPageSize = DMS_PAGE_SIZE_DFT;
      UINT32 lobdPageSize = DMS_DEFAULT_LOB_PAGE_SZ;
      DMS_STORAGE_TYPE stype = DMS_STORAGE_NORMAL;
      DMS_ENGINE_TYPE etype = DMS_ENGINE_INVALID;
      BOOLEAN sysCall = FALSE;
      IDataProtectionService *dpsCB = nullptr;
   };//struct dmsCreateCSOptions

   struct dmsCreateCLOptions : public SDBObject
   {
      UINT32 attributes = 0;
      UTIL_COMPRESSOR_TYPE compressor = UTIL_COMPRESSOR_INVALID;
      UINT8 pageMinFreePercent = 10;
      BOOLEAN sysCall = FALSE;
      const BSONObj *shardIdxDef = nullptr;
      const BSONObj *extOptions = nullptr;
      const BSONObj *idIdxDef = nullptr;
      BOOLEAN addIdxIDIfNotExist = FALSE;
      IDataProtectionService *dpsCB = nullptr;
   };//struct dmsCreateCLOptions

   struct dmsRemoveCSOptions : public SDBObject
   {
      BOOLEAN sysCall = FALSE;
      BOOLEAN ensureEmpty = FALSE;
      dmsDropCSOptions *recycleOptions = nullptr;
      IDataProtectionService *dpsCB = nullptr;
   };

   struct dmsRemoveCLOptions : public SDBObject
   {
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL;
      dmsDropCLOptions *recycleOptions = nullptr;
      IDataProtectionService *dpsCB = nullptr;
   };

   struct dmsTruncateCLOptions : public SDBObject
   {
      
   };

   struct dmsOpenCSOptions : public SDBObject
   {

   };

   struct dmsOpenCLOptions : public SDBObject
   {
      INT32 mbLockType = -1;
   };////struct dmsOpenCLOptions

   struct dmsBuildIndexOptions : public SDBObject
   {
      OSS_INLINE BOOLEAN isSortingDisabled()const
      {
         return 0 == sortBufferSize;
      }

      BOOLEAN sysCall = FALSE;
      UINT32 sortBufferSize = (UINT32)64 << 20; /// 64MB
      BOOLEAN blockDML = FALSE;
      utilWriteResult *result = nullptr;
      dmsIdxTaskStatus *idxStatus = nullptr;
      BOOLEAN forceTransCallback = FALSE;
      BOOLEAN addUIDIfNotExist = FALSE;
      IDataProtectionService *dpsCB = nullptr;
   };//struct dmsBuildIndexOptions

   struct dmsRemoveIndexOptions
   {
      BOOLEAN sysCall = FALSE;
      dmsIdxTaskStatus *idxStatus = nullptr;
      BOOLEAN onlyStandalone = FALSE;
      IDataProtectionService *dpsCB = nullptr;
   };//struct dmsRemoveIndexOptions

   struct dmsInsertRecordOptions : public SDBObject
   {
      dmsStripingId stripingId;
   };//struct dmsInsertRecordOptions

   struct dmsUpdateRecordOptions : public SDBObject
   {
      dmsStripingId stripingId;
   };//struct dmsUpdateRecordOptions

   struct dmsDeleteRecordOptions : public SDBObject
   {

   };//struct dmsDeleteRecordOptions

   struct dmsScanOptions : public SDBObject
   {
      DMS_SCAN_FOR scanFor = DMS_SCAN_FOR::NONE;
      INT64 rowCountLimit = -1;
      IRecordFilter *filter = NULL;
      INT32 pageStep = -1;
   };//struct dmsScanOptions

   struct dmsIndexScanOptions : public SDBObject
   {
      DMS_SCAN_FOR scanFor = DMS_SCAN_FOR::NONE;
      INT64 rowCountLimit = -1;
      BOOLEAN indexCovered = FALSE;
      UINT32 stepSize = 64;
   };//struct dmsIndexScanOptions

   struct dmsCreateDataSnapshotOptions
   {


   };//struct dmsCreateDataSnapshotOptions

   struct dmsListLobChunkOptions : public SDBObject
   {
      INT32 chunkId = -1;
   };//struct dmsListLobChunkOptions
} // namespace engine


#endif//SDB_DMS_ENGINE_OPTIONS_HPP_
