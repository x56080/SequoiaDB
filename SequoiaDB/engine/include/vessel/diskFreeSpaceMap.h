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

   Source File Name = diskFreeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DISK_FREE_SPACE_MAP_H_
#define VESSEL_DISK_FREE_SPACE_MAP_H_

#include "vessel/extentDef.h"
#include "vessel/fsmFile.h"
#include "ossLatch.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmSizeLvl.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   struct bitMapPage;

   class diskFreeSpaceMap : public SDBObject
   {
      public:
         diskFreeSpaceMap();
         ~diskFreeSpaceMap();
      public:
         BOOLEAN isOpen()const
         {
            return _entry.isValid();
         }

         INT32 create(fsmFile *file,
                      CL_MB_ID mbID,
                      UINT32 logicalID);

         INT32 open(fsmFile *file,
                    CL_MB_ID mbID,
                    UINT32 logicalID,
                    BOOLEAN autoRecreateEntry=TRUE);

         void close();

         INT32 find(const fsmSizeLvl &lvl,
                    CL_PAGE_SEQ &canditate,
                    fsmSizeLvl &realLvl,
                    FLOAT32 worthToScan=0.05);

         /// count of pages should always be 8
         INT32 addNewPages(CL_PAGE_SEQ sequence,
                           UINT32 count);

         INT32 updatePageFreeSizeLvL(CL_PAGE_SEQ sequence,
                                     const fsmSizeLvl &lvl);
      private:
         INT32 getBitMapPageByPageNo(UINT32 pageNo,
                                     fsmBitMapPage **page,
                                     fsmPageMapSlot **slot);

         BOOLEAN findFromBitMapPage(const fsmSizeLvl &lvl,
                                    UINT32 subPageNo,
                                    fsmBitMapPage *page,
                                    CL_PAGE_SEQ &candidate,
                                    fsmSizeLvl &realLvl);

         BOOLEAN findFromSubBitMapPage(const fsmSizeLvl &lvl,
                                       fsmBitMapSubPage *page,
                                       UINT32 &offset,
                                       fsmSizeLvl &realLvl);

         BOOLEAN findFromLvLn(fsmBitMapSubPage *page,
                              UINT32 lvl,
                              UINT32 delta,
                              UINT32 &offset,
                              UINT16 &realDelta);

      private:
         INT32 initBitMapPage(PAGE_ID pid);
         INT32 initPageMapPage(PAGE_ID pid, PAGE_ID pre);
         INT32 updateEntrySlot(CL_MB_ID mbID, const fsmCLEntry &entry, BOOLEAN sync);
         INT32 readEntrySlot(CL_MB_ID mbID, UINT32 logicalID, fsmCLEntry &entry);
         INT32 cachePMapPids();
         PAGE_ID getEntryPid(CL_MB_ID mbID);
         fsmCLEntry *getEntryFromPagePtr(ossValuePtr ptr, CL_MB_ID mbID);
         INT32 getPageHead(PAGE_ID pid, const fsmPageHead **head);
         INT32 getPageHead(PAGE_ID pid, fsmPageHead **head);
         OSS_INLINE UINT32 getPageNo(CL_PAGE_SEQ seq)
         {
            SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "impossible");
            return seq / FSM_SEQ_RANGE_IN_PAGE;
         }

         OSS_INLINE UINT32 getSubPageOffset(CL_PAGE_SEQ seq)
         {
            return (seq / FSM_SEQ_RANGE_IN_SUB_PAGE) & (FSM_SUB_BITMAP_COUNT - 1);
         }

         INT32 getRootBitMap(fsmBitMapPage **page);
         INT32 getPMap(UINT32 mapPageNo, fsmPageMapPage **page);

      private:
         INT32 createStats(fsmStats &stats);
         void createStats(const fsmPageHead *head, fsmStats &stats);

      private:
         UINT32 getDelta(const UINT64 *deltas,
                         UINT32 offset);
         void setDelta(UINT64 *deltas,
                       UINT32 offset,
                       UINT32 value);
         BOOLEAN setDeltaWithCAS(UINT64 *deltas,
                                 UINT32 offset,
                                 UINT32 value);

         void decStatWithCAS(fsmStats &stats, UINT32 lvl);
         void incStatWithCAS(fsmStats &stats, UINT32 lvl);

      private:
         INT32 ensurePMapPage(UINT32 count);

         INT32 createNewBitMapPage(fsmFile *file, PAGE_ID &pid);

         INT32 addNewPagesToBitMapPage(PAGE_ID pid,
                                       CL_PAGE_SEQ seq,
                                       UINT32 count);

         INT32 addNewPagesToPageMapPage(PAGE_ID pid,
                                        CL_PAGE_SEQ seq,
                                        UINT32 count);

         INT32 updateSizeLvl(CL_PAGE_SEQ seq,
                             const fsmSizeLvl &lvl,
                             fsmBitMapPage *page,
                             fsmSizeLvl &old);

      private:
         typedef ossPoolVector<PAGE_ID> _PAGE_MAP_PIDS;
         struct _scanCursor
         {
            _scanCursor()
            {
               reset();
            }

            OSS_INLINE void reset()
            {
               for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
               {
                  pages[i] = 0;
               }
               return;
            }

            UINT16 *get(UINT32 i)
            {
               return &(pages[i]);
            }

            UINT16 pages[FSM_SPACE_LVL_COUNT];
         };//struct _scanCursor

      private:
         fsmFile *_fsmFile;
         fsmCLEntry _entry;
         _PAGE_MAP_PIDS _pmapPids;
         _scanCursor _cursor;
         fsmStats _stats;
   };//class diskFreeSpaceMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_DISK_FREE_SPACE_MAP_H_