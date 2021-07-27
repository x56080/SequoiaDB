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

   Source File Name = deltaLogConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_CONSOLE_H_
#define VESSEL_DELTA_LOG_CONSOLE_H_

#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "ossMemPool.hpp"
#include "vessel/vesselFileName.h"
#include "vessel/deltaLogRecord.h"
#include "vessel/logicalPageSpaceCheckpoint.h"
#include "vessel/storageFileMap.h"
#include "vessel/deltaLogScanner.h"

namespace engine
{
namespace vessel
{
   class storageFileCreater;

   class deltaLogConsole : public SDBObject
   {
      public:
         deltaLogConsole();
         ~deltaLogConsole();

      public:
         deltaLogConsole(const deltaLogConsole &) = delete;
         deltaLogConsole &operator=(const deltaLogConsole &) = delete;

      public:
         OSS_INLINE const LPS_CHECKPOINT &getCheckpoint()const
         {
            return _checkpoint;
         }
         OSS_INLINE UINT64 getNextOffset()const
         {
            return _nextRecordOffset;
         }
         OSS_INLINE BOOLEAN isReady()const
         {
            return NULL != _creater;
         }

      public:
         /// init when creating.
         INT32 init(const storageFileCreater *creater);

         /// init when opening.
         /// If begingOffset is invalid, will search from offset zero.
         /// But if log files does not exist, will return error.
         INT32 init(const storageFileCreater *creater,
                    const FILE_NAME_LIST *fl,
                    UINT64 beginOffset);

         void fini();

         void destroy();
         

         /// WARNING: Can be used only before adding new log record and
         /// valid checkpoint exists.
         /// If begingOffset is invalid, will search from offset zero.
         INT32 initReaderBeforeAddingNewRecord(UINT64 beginOffset,
                                               deltaLogScanner &reader)const;

         INT32 append(const deltaLogRecord &dlr, UINT64 *offset=NULL);

         INT32 precreateCheckpoint(UINT32 flags,
                                   DPS_LSN_OFFSET lsn);

         void abortCheckpointPrecreated();
         INT32 commitCheckpointPrecreated();


      private:
         INT32 restoreToLastCheckpoint(const FILE_NAME_LIST *fl,
                                       UINT64 beginOffset);

         INT32 findLastCheckpoint(UINT64 beginOffset,
                                  BOOLEAN &found,
                                  LPS_CHECKPOINT &checkpoint)const;

         INT32 initLogFiles(const FILE_NAME_LIST *fl);

         INT32 appendDummyLogToSegment();


         INT32 ensureFileSpace(UINT64 fileId,
                               UINT32 segmentId);

         INT32 createNewLogFile(UINT64 fileId);

         INT32 _append(const deltaLogRecord &dlr, UINT64 &offset);

         /// maxOffset is the last byte offset to be fsynced.
         INT32 fsyncDeltaLog(UINT64 maxOffset);

      private:
         INT32 initLogBuffer();
         INT32 writeLogBuffer(const deltaLogRecord &dlr,
                              DELTA_LOG_CHECKSUM checksum);

         INT32 copyDataFromBufferToFile();

         OSS_INLINE BOOLEAN isBufferReady()const
         {
            return NULL != _logBuffer;
         }

   
      private:
         const storageFileCreater *_creater = NULL;
         storageFileMap _logFiles;

         LPS_CHECKPOINT _nextCheckpoint;
         LPS_CHECKPOINT _checkpoint;

         UINT64 _nextRecordOffset = 0;
         UINT64 _minDirtyOffset = 0;
         UINT64 _fileWriteOffset = 0;
         CHAR *_logBuffer = NULL;
         UINT32 _logBufferWriteSize = 0;
   };//class deltaLogConsole
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_CONSOLE_H_
