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

   Source File Name = freeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FREE_SPACE_MAP_H_
#define VESSEL_FREE_SPACE_MAP_H_

#include "vessel/stripingGroupFile.h"
#include "vessel/bitMapFile.h"
#include "ossLatch.hpp"
#include "vessel/stripingGroupFileDef.h"

namespace engine
{
namespace vessel
{
   const static UINT32 FSM_SMP_COUNT = 8;
   const static UINT32 FSM_MAX_CANDIDATES = 4;
   const static UINT32 FSM_SG_PAGE_LATCH_COUNT = 16;

   struct bitMapPage;
   class requestContext;

   class freeSpaceMap : public SDBObject
   {
      public:
         freeSpaceMap();
         ~freeSpaceMap();

      public:
         class candidates : public SDBObject
         {
            public:
               OSS_INLINE candidates():
               _count(0)
               {}

               OSS_INLINE ~candidates()
               {}

            public:
               OSS_INLINE void reset()
               {
                  _sg.reset();
                  _count = 0;
                  return;
               }
               
               OSS_INLINE BOOLEAN addCandidate(PAGE_ID lpid, PAGE_ID bmPid, UINT32 lvl)
               {
                  if (_count < FSM_MAX_CANDIDATES)
                  {
                     _lpids[_count] = lpid;
                     _bmPids[_count] = bmPid;
                     _lvls[_count] = lvl;
                     ++_count;
                     return TRUE;
                  }
                  return FALSE;
               }

               OSS_INLINE UINT32 count()const
               {
                  return _count;
               }

               OSS_INLINE BOOLEAN needNoMore()const
               {
                  return FSM_MAX_CANDIDATES == _count;
               }

               OSS_INLINE const stripingGroup &getSG()const
               {
                  return _sg;
               }

               OSS_INLINE void setSG(const stripingGroup &sg)
               {
                  _sg = sg;
                  return;
               }

               OSS_INLINE BOOLEAN getCandidate(UINT32 i,
                                               PAGE_ID &lpid,
                                               PAGE_ID &bmPid,
                                               UINT32 &lvl)
               {
                  if (i < _count)
                  {
                     lpid = _lpids[i];
                     bmPid = _bmPids[i];
                     lvl = _lvls[i];
                     return TRUE;
                  }
                  return FALSE;
               }
            private:
               stripingGroup _sg;
               UINT32 _count;
               PAGE_ID _lpids[FSM_MAX_CANDIDATES];
               PAGE_ID _bmPids[FSM_MAX_CANDIDATES];
               UINT32 _lvls[FSM_MAX_CANDIDATES];

         };//class candidates
      public:
         BOOLEAN isOpen()const;
         INT32 create(requestContext *context,
                      const CHAR * dir,
                      UINT32 secretValue,
                      UINT32 logicalCS);

         INT32 open(requestContext *context,
                    const CHAR *dir);

         INT32 close();

         /// should be under mb exclusive latch
         INT32 createCL(requestContext *context,
                        UINT32 clLogicalID,
                        UINT16 minStripingId,
                        UINT16 maxStripingId);

         /// should be under mb exclusive latch
         INT32 createCL(requestContext *context,
                        UINT32 clLogicalID);

         /// should be under mb exclusive latch
         INT32 dropCL(requestContext *context);

         INT32 addNewPages(CL_MB_ID mbid,
                           STRIPING_ID striping,
                           UINT32 count,
                           PAGE_ID lpid,
                           UINT32 lvl=BIT_MAP_BIT_LVL3);

         INT32 find(requestContext *context,
                    UINT32 logicalID,
                    STRIPING_ID striping,
                    UINT32 lvl,
                    candidates &c);

         INT32 find(requestContext *context,
                    UINT32 logicalID,
                    UINT32 lvl,
                    candidates &c);

         INT32 getStripingGroup(requestContext *context,
                                UINT32 logicalID,
                                UINT16 slot,
                                stripingGroupOnDisk &sg);

      private:
         INT32 createSGFile(requestContext *context,
                            const CHAR * dir,
                            UINT32 secretValue,
                            UINT32 logicalCS);

         INT32 createBitMapFile(requestContext *context,
                                const CHAR * dir,
                                UINT32 secretValue,
                                UINT32 logicalCS);
         
         INT32 extendSGFile(UINT32 minSegCount);

         INT32 initSGPage(CL_MB_ID mbid,
                          UINT32 clLogicalID,
                          UINT16 minStripingId,
                          UINT16 maxStripingId);

         INT32 resetSGPage(CL_MB_ID mbid);

         INT32 initBitMapSMP(PAGE_ID pid);

         BOOLEAN smpCrashed(ossValuePtr ptr);

         BOOLEAN sgpCrashed(ossValuePtr ptr);

         INT32 getSG(CL_MB_ID mbid,
                     UINT32 logicalID,
                     UINT16 slot,
                     stripingGroupOnDisk &sg);
         
         INT32 findSG(CL_MB_ID mbid,
                      UINT32 logicalID,
                      STRIPING_ID striping,
                      stripingGroup &sg);

         INT32 search(STRIPING_ID striping,
                      UINT32 count,
                      const stripingGroupOnDisk *begin,
                      stripingGroup &sg);

         INT32 findCandidates(requestContext *context,
                              UINT32 logicalID,
                              const stripingGroup &sg,
                              pageFreeSpaceLevel lvl,
                              candidates &c);

         INT32 findCandidates(CL_MB_ID mbid,
                              UINT32 logicalID,
                              UINT64 seed,
                              ossValuePtr ptr,
                              pageFreeSpaceLevel lvl,
                              candidates &c);

         UINT32 getMinSegmentCount(CL_MB_ID mbid);

         UINT32 getSGPCountPerFilePage();

         INT32 getSGPagePtr(CL_MB_ID mbid, ossValuePtr &ptr, PAGE_ID *pid=NULL);

      private:
         ossSpinXLatch _sgExtendingLatch;
         stripingGroupFile _sgFile;
         bitMapFile _bitmap;
   };//class freeSpaceMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_FREE_SPACE_MAP_H_