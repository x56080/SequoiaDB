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
#include "vessel/storageFileName.h"
#include "vessel/deltaLogFileDef.h"
#include "vessel/logicalPageSpaceCheckpoint.h"
#include "vessel/deltaLogRecord.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class idMapFile;
   class deltaLogFile;
   class logicalPageIdCache;

   class deltaLogConsole : public SDBObject
   {
      public:
         deltaLogConsole();
         ~deltaLogConsole();

      public:
         deltaLogConsole(const deltaLogConsole &) = delete;
         deltaLogConsole &operator=(const deltaLogConsole &) = delete;

      public:
         OSS_INLINE BOOLEAN isReady()const
         {
            return INVALID_SPACE_TYPE != _type;
         }
         OSS_INLINE UINT32 getBaseSequence()const
         {
            return _baseSequence;
         }
         OSS_INLINE const LPS_CHECKPOINT &getLastCheckpoint()const
         {
            return _lastCheckpoint;
         }

         OSS_INLINE PAGE_ID getLastCheckpointPid()const
         {
            return _lastCheckpointPid;
         }

         OSS_INLINE const storageFile &getWorkingFile()const
         {
            return _workingFile;
         }
      public:
         INT32 init(requestContext *context,
                    const idMapFile *base,
                    const STORAGE_FILE_NAME_LIST *fl);

         void fini();

         UINT64 getDeltaLogSize()const;


         void rebase(requestContext *context,
                     UINT32 base,
                     BOOLEAN destroyHistoryFileAtOnce);

         INT32 append(requestContext *context,
                      const deltaLogRecord &dlr);

         INT32 reserveCheckpoint(requestContext *context);

         void commit(const LPS_CHECKPOINT &checkpoint);

         void destroy(requestContext *context);

         void destroyHistoryFiles(requestContext *context);
      private:

         INT32 load(requestContext *context,
                    const STORAGE_FILE_NAME_LIST *fl);

         INT32 createNewFile(requestContext *context);

         INT32 resumeToLastCheckpoint();

         INT32 findLastCheckpointPid(const storageFile &file,
                                     PAGE_ID &pid)const;

         INT32 ensureFileAndBuffer(requestContext *context);

         void flushBufferAndShiftWritingPid();

         INT32 _append(requestContext *context,
                       const deltaLogRecord &dlr);

         void fsyncDirtyPages()const;
   
      private:
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _secretValue = 0;
         UINT32 _baseSequence = 0;

         storageFile _workingFile;
         memoryBlock _buffer;
         PAGE_ID _writingPid = INVALID_PAGE_ID;
         deltaLogFilePage *_page = NULL;
         LPS_CHECKPOINT _lastCheckpoint;
         PAGE_ID _lastCheckpointPid = INVALID_PAGE_ID;
         STORAGE_FILE_NAME_LIST _history;
         PAGE_ID _checkpointReserved = INVALID_PAGE_ID;
   };//class deltaLogConsole
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_CONSOLE_H_
