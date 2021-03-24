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

   Source File Name = freeSpaceMapDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FREE_SPACE_MAP_DEF_H_
#define SDB_VESSEL_FREE_SPACE_MAP_DEF_H_

#include "vessel/extentDef.h"
#include "pdTrace.hpp"
#include "vessel/recordDataPage.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   const static UINT32 FSM_STRIPING_BUCKET_COUNT = 32;
   const static UINT32 FSM_BITMAP_BITS_COUNT = 62;
   const static UINT32 FSM_SUB_BITMAP_COUNT = 8;
   const static UINT16 FSM_PAGE_VERSION = 1;
   const static UINT32 FSM_SMP_PID = 0;
   const static UINT32 FSM_PAGE_MAP_CAPAITY = 1364;

   const static UINT32 FSM_PAGE_SIZE = 32768;
   const static UINT32 FSM_PAGE_COUNT_PER_SEG = 64;
   const static UINT32 FSM_MAX_SEG_COUNT = 4096;

   const static UINT16 FSM_PAGE_TYPE_BITMAP = 1;
   const static UINT16 FSM_PAGE_TYPE_PAGEMAP = 2;

   const static UINT32 FSM_SEQ_RANGE_IN_SUB_PAGE = FSM_BITMAP_BITS_COUNT * 64;
   const static UINT32 FSM_SEQ_RANGE_IN_PAGE = FSM_SEQ_RANGE_IN_SUB_PAGE * FSM_SUB_BITMAP_COUNT;

   const static INT32 FSM_SPACE_LVL_INVALID = -1;
   const static INT32 FSM_SPACE_LVL0 = 0;
   const static INT32 FSM_SPACE_LVL1 = 1;
   const static INT32 FSM_SPACE_LVL2 = 2;
   const static INT32 FSM_SPACE_LVL3 = 3;
   const static INT32 FSM_SPACE_LVL_MIN = FSM_SPACE_LVL0;
   const static INT32 FSM_SPACE_LVL_MAX = FSM_SPACE_LVL3;
   const static UINT32 FSM_SPACE_LVL_COUNT = FSM_SPACE_LVL_MAX + 1;

   const static UINT32 FSM_LVL_DELTA_COUNT = 16;

   const static UINT32 FSM_32KB_LVL1 = 8192;
   const static UINT32 FSM_32KB_LVL2 = 16384;
   const static UINT32 FSM_32KB_LVL3 = 24576;

   const static UINT32 FSM_32KB_LVL3_DELTA_RANGE = (DMS_PAGE_SIZE32K - FSM_32KB_LVL3) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_32KB_LVL2_DELTA_RANGE = (FSM_32KB_LVL3 - FSM_32KB_LVL2) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_32KB_LVL1_DELTA_RANGE = (FSM_32KB_LVL2 - FSM_32KB_LVL1) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_32KB_LVL0_DELTA_RANGE = FSM_32KB_LVL1 / FSM_LVL_DELTA_COUNT;


   const static UINT32 FSM_64KB_LVL1 = FSM_32KB_LVL1 << 1;
   const static UINT32 FSM_64KB_LVL2 = FSM_32KB_LVL2 << 1;
   const static UINT32 FSM_64KB_LVL3 = FSM_32KB_LVL3 << 1;

   const static UINT32 FSM_64KB_LVL3_DELTA_RANGE = (DMS_PAGE_SIZE64K - FSM_64KB_LVL3) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_64KB_LVL2_DELTA_RANGE = (FSM_64KB_LVL3 - FSM_64KB_LVL2) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_64KB_LVL1_DELTA_RANGE = (FSM_64KB_LVL2 - FSM_64KB_LVL1) / FSM_LVL_DELTA_COUNT;
   const static UINT32 FSM_64KB_LVL0_DELTA_RANGE = FSM_64KB_LVL1 / FSM_LVL_DELTA_COUNT;
#pragma pack(4)
   struct fsmStats
   {
      OSS_INLINE fsmStats():
      totalPageCount(0)
      {
         for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
         {
            lvln[i] = 0;
         }
      }

      OSS_INLINE ~fsmStats(){}
      OSS_INLINE fsmStats &operator=(const fsmStats &o)
      {
         totalPageCount = o.totalPageCount;
         for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
         {
            lvln[i] = o.lvln[i];
         }
         return *this;
      }
      OSS_INLINE void reset()
      {
         for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
         {
            lvln[i] = 0;
         }
         totalPageCount = 0;
         return;
      }
      OSS_INLINE void merge(const fsmStats &o)
      {
         totalPageCount += o.totalPageCount;
         for (UINT32 i = 0; i < FSM_SPACE_LVL_COUNT; ++i)
         {
            lvln[i] += o.lvln[i];
         }
         return;
      }
      OSS_INLINE INT32 getLvl(UINT32 i)
      {
         return i < FSM_SPACE_LVL_COUNT ? lvln[i] : -1;
      }
      OSS_INLINE void decLvl(UINT32 i)
      {
         if (i < FSM_SPACE_LVL_COUNT)
         {
            --lvln[i];
         }
      }

   public:
      UINT32 totalPageCount;
      INT32 lvln[FSM_SPACE_LVL_COUNT];
   };// struct fsmStats

   struct fsmPageHead
   {
      UINT16 version;
      UINT16 type;
      UINT32 flags;
      UINT32 prePage;
      UINT32 nextPage;
      CHAR pad[8];
   };//struct fsmPageHead
   const static UINT32 FSM_PAGE_HEAD_SIZE = sizeof(fsmPageHead);

   struct fsmPageMapSlot
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_PAGE_ID != pid;
      }
      UINT32 pid;
      fsmStats stat;
   };//struct fsmPageMapSlot

   struct fsmPageMapPage
   {
      fsmPageHead head;
      UINT16 count;
      UINT16 flags;
      UINT32 pad;
      fsmPageMapSlot pages[FSM_PAGE_MAP_CAPAITY];
   };//struct fsmPageMapPage
   const static UINT32 FSM_PMAP_PAGE_SIZE = sizeof(fsmPageMapPage);

   struct fsmBitMapSubPage
   {
      OSS_INLINE UINT64 *getBits(UINT32 lvl)
      {
         return &(lvln[FSM_BITMAP_BITS_COUNT * lvl]);
      }
      OSS_INLINE UINT32 getBitsCount()const
      {
         return FSM_BITMAP_BITS_COUNT;
      }
      fsmPageHead head;
      fsmStats stat;
      CHAR pad[84];
      UINT64 lvln[FSM_BITMAP_BITS_COUNT * 4];
      UINT64 deltas[FSM_BITMAP_BITS_COUNT * 4];
   };//struct fsmBitMapSubPage

   struct fsmBitMapPage
   {
      fsmBitMapSubPage pages[FSM_SUB_BITMAP_COUNT];
   };//struct fsmBitMapPage
   const static UINT32 FSM_BIT_MAP_PAGE_SIZE = sizeof(fsmBitMapPage);


   struct fsmCLEntry
   {
      OSS_INLINE fsmCLEntry():
      root(INVALID_PAGE_ID),
      logicalID(DMS_INVALID_LOGICCLID){}

      OSS_INLINE ~fsmCLEntry()
      {}

      OSS_INLINE fsmCLEntry &operator=(const fsmCLEntry &o)
      {
         root = o.root;
         logicalID = o.logicalID;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return INVALID_PAGE_ID != root &&
                DMS_INVALID_LOGICCLID != logicalID;
      }

      UINT32 root;
      UINT32 logicalID;
   };//struct fsmCLEntry
   static const UINT32 FSM_CL_ENTRY_SIZE = sizeof(fsmCLEntry);

   const static UINT32 FSM_ENTRY_SLOT_COUNT = FSM_PAGE_SIZE / FSM_CL_ENTRY_SIZE;
   const static UINT32 FSM_ENTRY_PAGE_COUNT = 65536 / FSM_ENTRY_SLOT_COUNT;


   static const UINT8 FSM_CANDIDATE_FLAG_FILLBACK = 0x01;
   class fsmCandidate : public SDBObject
   {
      public:
         OSS_INLINE fsmCandidate():
         seq(INVALID_CL_PAGE_SEQ),
         lpid(INVALID_PAGE_ID),
         free(0),
         flags(0),
         bucket(-1){}

         OSS_INLINE fsmCandidate(CL_PAGE_SEQ s,
                                 PAGE_ID l,
                                 UINT16 f):
         seq(s),
         lpid(l),
         free(f),
         flags(0),
         bucket(-1){}

         OSS_INLINE ~fsmCandidate(){}

         OSS_INLINE fsmCandidate(const fsmCandidate &o):
         seq(o.seq),
         lpid(o.lpid),
         free(o.free),
         flags(o.flags),
         bucket(o.bucket){}

         OSS_INLINE fsmCandidate &operator=(const fsmCandidate &o)
         {
            seq = o.seq;
            lpid = o.lpid;
            free = o.free;
            flags = o.flags;
            bucket = o.bucket;
            return *this;
         }

         OSS_INLINE void reset()
         {
            seq = INVALID_CL_PAGE_SEQ;
            lpid = INVALID_PAGE_ID;
            free = 0;
            flags = 0;
            bucket = -1;
            return;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_CL_PAGE_SEQ != seq;
         }

         OSS_INLINE BOOLEAN testFillBackFlag()const
         {
            return OSS_BIT_TEST(flags, FSM_CANDIDATE_FLAG_FILLBACK);
         }
         OSS_INLINE void setFillBack()
         {
            OSS_BIT_SET(flags, FSM_CANDIDATE_FLAG_FILLBACK);
         }
         OSS_INLINE void clearFillBack()
         {
            OSS_BIT_CLEAR(flags, FSM_CANDIDATE_FLAG_FILLBACK);
         }

         OSS_INLINE void setBucketNo(INT8 bucketNo)
         {
            bucket = bucketNo;
            return;
         }
         OSS_INLINE BOOLEAN hasBucketNo()const
         {
            return 0 <= bucket;
         }
         OSS_INLINE void resetBucket()
         {
            bucket = -1;
         }
      public:
         CL_PAGE_SEQ seq;
         PAGE_ID lpid;
         UINT16 free;
         UINT8 flags;
         INT8 bucket;
   };//class fsmCandidate
   
#pragma pack()
   

   

   
   
   /*
   const static UINT32 FMS_32KB_LVLS_SIZE[FSM_SPACE_LVL_MAX] = {0, FSM_32KB_LVL2, FSM_32KB_LVL3, FSM_32KB_LVL4};
   const static UINT32 FSM_32KB_LVLS_DELTA_RANGE[FSM_SPACE_LVL_MAX] = {FSM_32KB_LVL1_DELTA_RANGE,
                                                                       FSM_32KB_LVL2_DELTA_RANGE,
                                                                       FSM_32KB_LVL3_DELTA_RANGE,
                                                                       FSM_32KB_LVL4_DELTA_RANGE};
   const static UINT32 FMS_64KB_LVLS_SIZE[FSM_SPACE_LVL_MAX] = {0, FSM_64KB_LVL2, FSM_64KB_LVL3, FSM_64KB_LVL4};
   const static UINT32 FSM_64KB_LVLS_DELTA_RANGE[FSM_SPACE_LVL_MAX] = {FSM_64KB_LVL1_DELTA_RANGE,
                                                                       FSM_64KB_LVL2_DELTA_RANGE,
                                                                       FSM_64KB_LVL3_DELTA_RANGE,
                                                                       FSM_64KB_LVL4_DELTA_RANGE};
*/


   BOOLEAN isWorthToScanDisk(UINT32 needLvl,
                             UINT32 totalCnt,
                             INT32 lvl0,
                             INT32 lvl1,
                             INT32 lvl2,
                             INT32 lvl3,
                             const FLOAT32 *minPercent=NULL);
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FREE_SPACE_MAP_DEF_H_