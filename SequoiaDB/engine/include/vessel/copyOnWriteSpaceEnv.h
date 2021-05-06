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

   Source File Name = copyOnWriteSpaceEnv.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COW_SPACE_ENV_H_
#define VESSEL_COW_SPACE_ENV_H_

#include "ossLatch.hpp"
#include "vessel/deltaLpidTable.h"
#include "vessel/copyOnWriteSpaceControlFile.h"
#include "vessel/copyOnWriteSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   static const UINT32 COW_DELTA_LOG_PAGE_SIZE = 4096;
   static const UINT32 COW_DELTA_LOG_PAGE_COUNT_PER_SEG = 512;
   static const UINT32 COW_DELTA_LOG_SEG_COUNT_PER_FILE = 16;

   class requestContext;
   class storageUnit;

   class copyOnWriteSpaceEnv : public SDBObject
   {
      public:
         copyOnWriteSpaceEnv();
         ~copyOnWriteSpaceEnv();
         copyOnWriteSpaceEnv(const copyOnWriteSpaceEnv &) = delete;
         copyOnWriteSpaceEnv &operator=(const copyOnWriteSpaceEnv &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _open;
         }

      public:
         INT32 open(requestContext *context,
                    storageUnit *su);
         void close();

      public:
         INT32 allocateIdxMPages(requestContext *context,
                                 PAGE_ID first,
                                 UINT32 count);

      private:
         INT32 restoreEnvToLastCheckpoint(requestContext *context);
         INT32 restoreStorageUnit(requestContext *context,
                                  const copyOnWriteSpaceCheckpoint &oldest,
                                  const copyOnWriteSpaceCheckpoint &latest);
         INT32 restoreIdxMetaFile(requestContext *context,
                                  const copyOnWriteSpaceCheckpoint &checkpoint);
         INT32 restoreIdxMetaFileFromBackup(requestContext *context,
                                            const copyOnWriteSpaceCheckpoint &checkpoint);

      private:
         INT32 openControlFile(requestContext *context,
                               const strSlice &dataPath,
                               const strSlice &suDir);

         INT32 ensureDeltaLogBuffer();

      private:
         ossSpinSLatch _checkpointLatch;
         ossSpinXLatch _latch;
         BOOLEAN _open = FALSE;
         storageUnit *_su = NULL;
         copyOnWriteSpaceControlFile _controlFile;
         copyOnWriteSpaceCheckpoint _checkpoint;
         deltaLpidTable _idxDeltaTable;
         CHAR *_deltaLogBuffer = NULL;

   };//class copyOnWriteSpaceEnv
}//namespace vessel
}//namespace engine

#endif//VESSEL_COW_SPACE_ENV_H_