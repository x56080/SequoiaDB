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

   Source File Name = copyOnWriteSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COPY_ON_WRITE_SPACE_H_
#define VESSEL_COPY_ON_WRITE_SPACE_H_

#include "vessel/pageDef.h"
#include "vessel/inMemBitMap.h"
#include "ossLatch.hpp"
#include "vessel/copyOnWriteSpaceControlFile.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class storageUnit;

   class copyOnWriteSpace : public SDBObject
   {
      public:
         copyOnWriteSpace();
         ~copyOnWriteSpace();

         copyOnWriteSpace(const copyOnWriteSpace &) = delete;
         copyOnWriteSpace &operator=(const copyOnWriteSpace &) = delete;

      public:
         INT32 open(requestContext *context,
                    storageUnit *su);

         INT32 preallocateNewIndexPages(requestContext *context,
                                        UINT32 count,
                                        PAGE_ID *lpids,
                                        PAGE_ID *pids);

         INT32 allocateNewIndexPages(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *lpids,
                                     const PAGE_ID *pids);

      private:
         void fini();

         INT32 initControlFile(requestContext *context,
                               const strSlice &path,
                               const strSlice &dirPath);

         INT32 initInMemBitMap(UINT32 idxMetaPageSize,
                               UINT32 idxDataPageSize);

         INT32 restoreSpaceToLastCheckpoint(requestContext *context,
                                            storageUnit *su);
      private:
         ossSpinSLatch _latch;
         copyOnWriteSpaceControlFile _controlFile;
         storageUnit *_su = NULL;
         inMemBitMap _inMemLpidPool;
         inMemBitMap _inMemPidPool;

   };//class copyOnWriteSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_COPY_ON_WRITE_SPACE_H_