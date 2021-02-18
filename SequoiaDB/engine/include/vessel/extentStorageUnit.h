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

   Source File Name = extentStorageUnit.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_EXTENT_STORAGE_UNIT_H_
#define VESSEL_EXTENT_STORAGE_UNIT_H_

#include "dms.hpp"
#include "vessel/vesselDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/extentStorageFile.h"
#include "vessel/stoargeUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/inMemBitMap.h"
#include <vector>

namespace engine
{
namespace vessel
{
   class dataExtentIDMapFile;
   class dataExtentFile;
   class requestContext;

   class extentStorageUnit : public SDBObject
   {
      public:
         extentStorageUnit();
         ~extentStorageUnit();

      private:
         extentStorageUnit(const extentStorageUnit &o);
         extentStorageUnit &operator=(const extentStorageUnit &o);

      public:
         enum SU_STATUS
         {
            CLOSED = 0,
            OPEN,
         };
      public:
         OSS_INLINE BOOLEAN closed()const
         {
            return CLOSED == _status;
         }
         OSS_INLINE const CHAR *getDirName()const
         {
            return _dirName;
         }
         OSS_INLINE SU_STATUS getStatus()const
         {
            return _status;
         }

         OSS_INLINE UINT32 getIDMapPageCapacity()const
         {
            return _idMapCapacity;
         }

         OSS_INLINE UINT32 getCLRecordPageCapacity()const
         {
            return _capacityOfCLRecordPage;
         }

      public:
         INT32 open(requestContext *context,
                    const strSlice &dirName);

         INT32 create(requestContext *context,
                      const createSUOptions &options);

         INT32 close(requestContext *context);

         INT32 destroy(requestContext *context);

         INT32 extendMetaFile(requestContext *context,
                              PAGE_ID *newPidInFile=NULL);

         INT32 getPagePtr(SPACE_TYPE type, PAGE_ID id, ossValuePtr &ptr);

         SPACE_ID getSpaceID()const;
         INT32 getCoreArgs(SPACE_TYPE type,
                           UINT32 *pageSize = NULL,
                           UINT32 *maxPageCountPerSeg = NULL,
                           UINT32 *maxSegCountPerFile = NULL);

         INT32 fsync(SPACE_TYPE type, PAGE_ID pid, UINT32 count, BOOLEAN sync=TRUE);

         /// call this function only after normally started or redo.
         INT32 initInMemBitMapWhenOpening(requestContext *context,
                                          BOOLEAN rebuild);

      public:
         INT32 getLpidOfClRecord(CL_MB_ID mbID, PAGE_ID &lpid)const;
         INT32 getDataPhysicalPid(requestContext *context,
                                  PAGE_ID lpid,
                                  PAGE_ID &ppid,
                                  SNAPSHOT_ID &snap);
         INT32 getDataSMPPId(PAGE_ID pid, PAGE_ID &smpID);

         /// preallocating will set free pages as busy in memory.
         /// no data will be written to the disk smp by preallocating.
         /// but new file may be automatically created, or one created file may be extended.
         /// you can release these pages before writing on disk.
         INT32 preallocateDataPages(requestContext *context,
                                    UINT32 count,
                                    PAGE_ID *pids);

         INT32 releaseDataPagesPreallocated(requestContext *context,
                                            UINT32 count,
                                            const PAGE_ID *pids);

         INT32 remapLpid(requestContext *context,
                         PAGE_ID lpid,
                         PAGE_ID pid,
                         const DPS_LSN_OFFSET *oplist);

      private:
         PAGE_ID getDataIMPPid(PAGE_ID lpid)const;

      private:
         INT32 close();

         INT32 openOtherFilesUnderPath(const CHAR *path,
                                       const strSlice &dirName);
         INT32 openMetaFile(const CHAR *storagePath,
                            const strSlice &dirName,
                            SPACE_ID sid);
         INT32 openFile(const CHAR *fullPath,
                        const storageFileName &fn);
         INT32 crossCheckFilesWhenOpenning();

         INT32 testAllDirsBeforeCreating(const storagePathOptions &path,
                                         const CHAR *dirName);

         INT32 testAllDirsBeforeOpenning(const storagePathOptions &path,
                                         const strSlice &dirName,
                                         BOOLEAN &impossibleCrashed);

         INT32 testDir(const CHAR *fullPath,
                       UINT32 &subCount);

         INT32 createAllDirs(const storagePathOptions &path,
                             const CHAR *dirName);

         INT32 removeAllDirs(const storagePathOptions &path,
                             const CHAR *dirName,
                             BOOLEAN mustBeEmpty);

         INT32 removeDir(const CHAR *fullPath, BOOLEAN mustBeEmpty);

         INT32 createNecessaryFiles(requestContext *context,
                                    const createSUOptions &options,
                                    const CHAR *path);

         INT32 createSUNameFile(const CHAR *dir,
                                SPACE_ID sid,
                                const strSlice &csName);

         INT32 removeSUNameFile(const CHAR *dir,
                                SPACE_ID sid);

         INT32 createMetaFile(requestContext *context,
                              const CHAR *dir,
                              const createSUOptions &csOptions);

         INT32 firstExtendMetaFile(requestContext *context);

         INT32 initNecessaryPagesWhenCreating(requestContext *context,
                                              const createSUOptions &options);

         INT32 initGMP(requestContext *context,
                       const strSlice &csName,
                       const createCSOptions &options);

         INT32 initSystemIMP(requestContext *context);

         /// createDataFile should always be protected by extending latch.
         INT32 createDataFile(requestContext *context,
                              UINT32 *sequenceOfNewFile);

         INT32 firstExtendDataFile(requestContext *context,
                                   dataExtentFile *file);

         /// just for rollback createDataFile when failed to extend in-mem bitmap.
         /// do not use this func unless you clearly know what you are doing!!!
         INT32 removeLastDataFile(requestContext *context);

         INT32 getDataFile(UINT32 fileSequence, dataExtentFile **file);

         INT32 ensureDataFileSpace(requestContext *context,
                                   UINT32 sequence,
                                   PAGE_ID pidInFile);

         INT32 initInMemBitMapWhenCreating();

         INT32 saveBitMapWhenClosing(requestContext *context);

      private:
         SU_STATUS _status;
         CHAR _dirName[MAX_SU_DIR_LEN + 1];
         UINT32 _idMapCapacity;
         UINT32 _capacityOfCLRecordPage;

         ossSpinXLatch _extendingMetaAndDataLatch;
         dataExtentIDMapFile *_meta;

         ossSpinSLatch _dataFileAccessingMutex;
         std::vector<dataExtentFile *> _data;

         extentStorageFile *_idxMeta;
         std::vector<extentStorageFile *> _idx;

         inMemBitMap _inMemDataSMP;
   };//class extentStorageUnit
}//namespace vessel
}//namespace engine

#endif // VESSEL_EXTENT_STORAGE_UNIT_H_