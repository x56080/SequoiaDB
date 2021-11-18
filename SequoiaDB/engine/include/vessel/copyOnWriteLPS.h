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

   Source File Name = copyOnWriteLPS.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COPY_ON_WRITE_LPS_H_
#define VESSEL_COPY_ON_WRITE_LPS_H_

#include "vessel/logicalPageSpace.h"
#include "vessel/idMapFile.h"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class copyOnWriteLPS : public logicalPageSpace
   {
      public:
         copyOnWriteLPS();
         virtual ~copyOnWriteLPS();

      private:
         virtual UINT32 getIdMapFileHeadFlags()const
         {
            return ID_MAP_FILE_FLAG_COPY_ON_WRITE;
         }
         virtual BOOLEAN validateIdMapFileHeadFlags(UINT32 flags)const
         {
            return ID_MAP_FILE_FLAG_COPY_ON_WRITE == flags;
         }

         virtual void _close();
         virtual void _destroy();

         virtual inMemBitmap::options getStorageAllocatorOptions()const;

      private:
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            const ossSharedLatchMode &mode,
                                            runtimePageBuffer &rpb);

         virtual INT32 getRuntimePageBufferToReset(requestContext *context,
                                                   PAGE_ID pid,
                                                   runtimePageBuffer &rpb);

         virtual INT32 copyPageAndReinitBuffer(requestContext *context,
                                               PAGE_SNAPSHOT_VERION psv,
                                               PAGE_ID newPid,
                                               runtimePageBuffer &rpb);

      protected:
         virtual BOOLEAN isCopyOnWrite()const {return TRUE;}

         virtual INT32 map(requestContext *context,
                           PAGE_SNAPSHOT_VERION psv,
                           UINT32 count,
                           const mappedLogicalPageId *mpids);

         virtual INT32 remap(requestContext *context,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 count,
                             const mappedLogicalPageId *mpids,
                             const PAGE_ID *oldPids,
                             BOOLEAN releaseOld);

         virtual INT32 unmap(requestContext *context,
                             UINT32 count,
                             const mappedLogicalPageId *mpids,
                             BOOLEAN releasePid);

         virtual INT32 releasePids(requestContext *context,
                                   UINT32 count,
                                   const PAGE_ID *pids);

      private:
         virtual INT32 prepareToCreateCheckpoint(requestContext *context,
                                                 BOOLEAN fullCheckpoint);
         
         virtual void endToCreateCheckpoint(requestContext *context);
      private:
         void insertIntoReleasingBlock(UINT32 count,
                                       const PAGE_ID *pids);

         void mergeBlocksToReadyList();

         void fini();

      private:
         typedef std::pair<UINT32, CHAR *> _RELEASING_BLOCK;

         ///list always be extended by user threads but
         /// cleared by background workers.  Should we use std::list here?
         typedef ossPoolList<_RELEASING_BLOCK> _BLOCK_LIST;

         static const UINT32 _BLOCK_CAPACITY = 64;

         ossSpinXLatch _blockLatch;
         CHAR *_block = NULL;
         UINT32 _size = 0;
         _BLOCK_LIST _waitingList;
         _BLOCK_LIST _readyList;
   };//class copyOnWriteLPS
}//namespace vessel
}//namespace engine

#endif//VESSEL_COPY_ON_WRITE_LPS_H_