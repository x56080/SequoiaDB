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
#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/extentStorageFile.h"
#include "vessel/storageUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class dataExtentIDMapFile;
   class dataExtentFile;
   class requestContext;
   class fsmFile;
   class deltaLogFile;
   class idxIDMapFile;
   class idxDataFile;
   class idxMBackupFile;

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
         UINT32 getLogicalID()const;
         UINT32 getMetaSegmentCount()const;
         INT32 getPagePtr(FILE_TYPE type,
                          PAGE_ID id,
                          ossValuePtr &ptr);
         INT32 getCoreArgs(FILE_TYPE type,
                           UINT32 *pageSize = NULL,
                           UINT32 *maxPageCountPerSeg = NULL,
                           UINT32 *maxSegCountPerFile = NULL)const;
         INT32 fsync(FILE_TYPE type,
                     PAGE_ID pid,
                     UINT32 count,
                     BOOLEAN sync=TRUE);

         INT32 fsync(FILE_TYPE,
                     UINT32 count,
                     const PAGE_ID *pids,
                     BOOLEAN sync=TRUE);

         INT32 getMaxPageCountInDDFile(UINT32 &count)const;

         UINT32 getDataFileCount();

         void dumpIDMapFileHead(dataIDMapFileHead &head);

         OSS_INLINE fsmFile *getFsmFilePtr()
         {
            return _fsm;
         }

      public:
         INT32 create(requestContext *context,
                      const createSUOptions &options);

         void destroy(requestContext *context);

         INT32 open(requestContext *context,
                    const strSlice &dirName);
                    
         void close();

      public:
         INT32 extendMetaFile(requestContext *context,
                              const UINT32 *segmentCount=NULL);

         INT32 createNewDataFile(requestContext *context,
                                 UINT64 sequence=STORAGE_FILE_INVALID_SEQUENCE);

         /// file must exist first.
         INT32 ensureDataFileSpace(requestContext *context,
                                   PAGE_ID pid);

         /// WARNING: you should use this interface when init smp failed.
         INT32 removeLastDataFile(requestContext *context);

         INT32 ensureCSNameFile(requestContext *context,
                                const strSlice &csName);

         INT32 ensureFsmFile(requestContext *context, fsmFile **out);

      public:
         idxIDMapFile *getIdxMetaFile()
         {
            return _idxMeta;
         }
         idxMBackupFile *getIdxMBackupFile()
         {
            return _idxMBackup;
         }
         INT32 destroyIdxMBackupFile();
         void clearFilesOfCowSUWhenRestore();

      private:
         void destoryIdxMetaFile();

      private:
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
                                    const createSUOptions &options);
         
         INT32 createMetaFile(requestContext *context,
                              const createSUOptions &options);

         INT32 openOtherFilesUnderPath(const CHAR *path,
                                       const strSlice &dirName);
         

         INT32 getDataFile(UINT32 fileSequence, dataExtentFile **file);

         INT32 crossCheckFilesWhenOpenning();

         INT32 removeCSNameFile(const strSlice &dataPath);

      private:
         INT32 extendDataFile(requestContext *context,
                              UINT32 sequence,
                              UINT32 minSegCount);

         INT32 fsyncDataPages(PAGE_ID pid, UINT32 count);

      private:
         INT32 openFile(const strSlice &fullPath,
                        const vesselFileName &fn);
         INT32 openMetaFile(const strSlice &fullPath,
                            const vesselFileName &fn);
         INT32 openDataFile(const strSlice &fullPath,
                            const vesselFileName &fn);
         INT32 openIdxMFile(const strSlice &fullPath,
                            const vesselFileName &fn);
         INT32 openIdxDFile(const strSlice &fullPath,
                            const vesselFileName &fn);
         INT32 openDeltaFile(const strSlice &fullPath,
                             const vesselFileName &fn);
         INT32 openFSMFile(const strSlice &fullPath,
                           const vesselFileName &fn);
         INT32 openIdxMetaBackupFile(const strSlice &fullPath,
                                     const vesselFileName &fn);
      private:
         INT32 buildFileFullPath(const strSlice &path,
                                   const strSlice &dir,
                                   const vesselFileName &fn,
                                   UINT32 bufferSize,
                                   CHAR *buffer);
         
      private:
         typedef ossPoolVector<dataExtentFile*> _DATA_VEC;
         typedef ossPoolVector<idxDataFile *> _INDEX_DATA_VEC;
         typedef ossPoolList<deltaLogFile *> _DELTA_LIST;


      private:
         BOOLEAN _isOpen;
         CHAR _dirName[MAX_SPACE_DIR_LEN + 1];
         
         ossSpinXLatch _extendingDDAndDMLatch;
         dataExtentIDMapFile *_meta;
         ossSpinSLatch _dataFileAccessingMutex;
         _DATA_VEC _data;

         ossSpinXLatch _fsmLatch;
         fsmFile *_fsm;

         ossSpinSLatch _cowFilesAccessingLatch;
         idxIDMapFile *_idxMeta;
         idxMBackupFile *_idxMBackup;
         _INDEX_DATA_VEC _idxDataVec;
         _DELTA_LIST _delta;
   };//class storageUnit
}//namespace vessel
}//namespace engine

#endif // VESSEL_STORAGE_UNIT_H_