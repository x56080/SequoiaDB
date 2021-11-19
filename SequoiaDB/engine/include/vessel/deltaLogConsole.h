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
#include "vessel/vesselFileName.h"
#include "vessel/deltaLogFileDef.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class idMapFile;
   class deltaLogFile;

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

      public:
         INT32 init(requestContext *context,
                    const idMapFile *base,
                    const FILE_NAME_LIST *fl);

         void fini();

         void destroy(requestContext *context);

         deltaLogFile *getOnlineFile();

         BOOLEAN hasOnlineFile()const
         {
            return _file.isOpen();
         }

         UINT32 getValidSegmentCount()const
         {
            return _validSegmentCount;
         }

         UINT64 getDeltaLogSize()const
         {
            return _validSegmentCount * deltaLogFile::FILE_SEGMENT_SIZE;
         }

         BOOLEAN hasDeltaLog()const
         {
            return 0 < _validSegmentCount;
         }

         slice getDumpedRecord(UINT32 segmentId,
                               deltaLogCheckpointRecord *out)const;

         INT32 rebase(requestContext *context,
                      UINT64 base);

         INT32 append(requestContext *context,
                      const ossPoolVector<memoryBlock> &buffers,
                      const LPS_CHECKPOINT &checkpoint);
      private:

         INT32 initLogFiles(requestContext *context,
                            const FILE_NAME_LIST *fl);

         void destroyHistoryFiles(requestContext *context);

         INT32 findOnlineFileEnding();

         INT32 createOnlineFile(requestContext *context);
   
      private:
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _secretValue = 0;
         UINT64 _base = 0;
         UINT32 _validSegmentCount = 0;
         deltaLogFile _file;
         FILE_NAME_LIST _history;
   };//class deltaLogConsole
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_CONSOLE_H_
