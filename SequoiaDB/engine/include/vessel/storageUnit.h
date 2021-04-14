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

   Source File Name = storageUnit.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_UNIT_H_
#define VESSEL_STORAGE_UNIT_H_

#include "dms.hpp"
#include "vessel/vesselDef.h"
#include "vessel/extentStorageFile.h"
#include "vessel/storageUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include <vector>

namespace engine
{
namespace vessel
{
   class dataExtentIDMapFile;
   class dataExtentFile;
   class requestContext;
   class fsmFile;

   class storageUnit : public SDBObject
   {
      public:
         storageUnit();
         ~storageUnit();
         storageUnit(const storageUnit &o) = delete;
         storageUnit &operator=(const storageUnit &o) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }
         OSS_INLINE const CHAR *getDirName()const
         {
            return _dirName;
         }

      public:
         SPACE_ID getSpaceID()const;
         UINT32 getMetaSegmentCount()const;
         INT32 getPagePtr(SPACE_TYPE type,
                          PAGE_ID id,
                          ossValuePtr &ptr);
         INT32 getCoreArgs(SPACE_TYPE type,
                           UINT32 *pageSize = NULL,
                           UINT32 *maxPageCountPerSeg = NULL,
                           UINT32 *maxSegCountPerFile = NULL);
         INT32 fsync(SPACE_TYPE type,
                     PAGE_ID pid,
                     UINT32 count,
                     BOOLEAN sync=TRUE);

         INT32 fsync(SPACE_TYPE,
                     UINT32 count,
                     const PAGE_ID *pids,
                     BOOLEAN sync=TRUE);

         INT32 getMaxPageCountInFile(UINT32 &count);

         UINT32 getDataFileCount();

         void dumpIDMapFileHead(dataIDMapFileHead &head);

         OSS_INLINE fsmFile *getFsmFilePtr()
         {
            return _fsm;
         }

      public:
         INT32 create(requestContext *context,
                      const createSUOptions &options);

         ///Whatever error code returns, su object will
         ///be reset.
         INT32 destroy(requestContext *context);

         INT32 open(requestContext *context,
                    const strSlice &dirName);
                    
         void close(requestContext *context);

      public:
         INT32 extendMetaFile(requestContext *context,
                              const UINT32 *segmentCount=NULL);

         INT32 createDataFile(requestContext *context,
                              UINT32 *sequenceOfNewFile);

         /// file must exist first.
         INT32 ensureDataFileSpace(requestContext *context,
                                   PAGE_ID pid);

         /// WARNING: you should use this interface when init smp failed.
         INT32 removeLastDataFile(requestContext *context);

         INT32 ensureSUNameFile(requestContext *context,
                                const strSlice &csName);

         INT32 createFsmFile(requestContext *context);

      private:
         void close();
         BOOLEAN validateSUOptions(const createSUOptions &options);

         INT32 testAllDirsBeforeCreating(const storagePathOptions &path,
                                         const strSlice &dir);
         INT32 testAllDirsBeforeOpenning(const storagePathOptions &path,
                                         const strSlice &dirName,
                                         BOOLEAN &impossibleCrashed);
         INT32 testDir(const CHAR *fullPath,
                       UINT32 &subCount);
         INT32 createAllDirs(const storagePathOptions &path,
                             const strSlice &dir);
         INT32 removeDir(const CHAR *fullPath, BOOLEAN mustBeEmpty);
         INT32 removeAllDirs(const storagePathOptions &path,
                             const CHAR *dir,
                             BOOLEAN mustBeEmpty);
         INT32 createNecessaryFiles(requestContext *context,
                                    const createSUOptions &options,
                                    const storagePathOptions &path);
         INT32 createMetaFile(requestContext *context,
                              const CHAR *dir,
                              const createSUOptions &options);
         INT32 openMetaFile(const CHAR *storagePath,
                            const strSlice &dirName,
                            SPACE_ID sid);

         INT32 openOtherFilesUnderPath(const CHAR *path,
                                       const strSlice &dirName);
         INT32 openFile(const CHAR *fullPath,
                        const storageFileName &fn);
         
         INT32 removeSUNameFile(const CHAR *dataPath,
                                SPACE_ID sid);

         INT32 getDataFile(UINT32 fileSequence, dataExtentFile **file);

         INT32 crossCheckFilesWhenOpenning();

      private:
         INT32 extendDataFile(requestContext *context,
                              UINT32 sequence,
                              UINT32 minSegCount);

         INT32 fsyncDataPages(PAGE_ID pid, UINT32 count);

      private:
         typedef ossPoolVector<dataExtentFile*> _DATA_VEC;

      private:
         BOOLEAN _isOpen;
         CHAR _dirName[MAX_SU_DIR_LEN + 1];
         
         ossSpinXLatch _extendingMetaLatch;
         dataExtentIDMapFile *_meta;

         ossSpinXLatch _extendingDataLatch;
         ossSpinSLatch _dataFileAccessingMutex;
         _DATA_VEC _data;

         extentStorageFile *_idxMeta;
         std::vector<extentStorageFile *> _idx;

         fsmFile *_fsm;
         
   };//class storageUnit
}//namespace vessel
}//namespace engine

#endif // VESSEL_STORAGE_UNIT_H_