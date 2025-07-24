/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = variableExtentAllocator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_
#define VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_

#include "ossMemPool.hpp"
#include "vessel/pageIdentifier.h"
#include "ossLatch.hpp"
#include "vessel/sparseBitmap32.h"

#include <mutex> //c++11
#include <atomic> //c++11
#include <boost/dynamic_bitset.hpp>

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class variableExtentAllocator : public SDBObject
   {
      public:
         variableExtentAllocator() = default;
         ~variableExtentAllocator();
         variableExtentAllocator(const variableExtentAllocator &) = delete;
         variableExtentAllocator &operator=(const variableExtentAllocator &) = delete;

      public:
         struct options : public SDBObject
         {
            UINT32 maxPageCountPerSegment = 0;
            UINT32 maxSegmentCountPerFile = 0;
            UINT32 minSegFreeCntReused = 8;
         };//struct options

      private:
         enum class CELL_CMP_RES : INT32
         {
            LOWER_WITH_HOLE = -2,
            LOWER_WITH_NO_HOLE = -1,
            INTERSECTIED = 0,
            UPPER_WITH_NO_HOLE = 1,
            UPPER_WITH_HOLE = 2,
         };

         struct _extentCell : public SDBObject
         {
            _extentCell() = default;
            explicit _extentCell(UINT16 o, UINT16 s):
            offset(o),
            size(s){}
            ~_extentCell() = default;
            _extentCell(const _extentCell &o) = default;
            _extentCell &operator=(const _extentCell &o) = default;

            OSS_INLINE UINT16 getUpperBound()const
            {
               return offset + size;
            }

            CELL_CMP_RES compare(const _extentCell &o)const;

            UINT16 offset = 0;
            UINT16 size = 0;
         };//struct _extentCell

         typedef class ossPoolList<_extentCell> _CELL_LIST;

         class _segmentUnit : public SDBObject
         {
            public:
               explicit _segmentUnit(UINT32 capacity);
               ~_segmentUnit(){}
               _segmentUnit(const _segmentUnit &) = delete;
               _segmentUnit &operator=(const _segmentUnit &) = delete;

            public:
               OSS_INLINE UINT16 getCapacity()const {return _capacity;}
               OSS_INLINE UINT16 getMaxFreeExtentSize()const {return _maxExtentSize;}
               OSS_INLINE UINT16 getFreePidCount()const {return _freePids;}

            public:
               void clear();
               void initFromSme(UINT64 *sme);

               void init(BOOLEAN allFree);

               INT32 reserveExtent(UINT32 pcnt);
               void freeExtent(UINT32 poffset, UINT32 pcnt);
               BOOLEAN test(UINT32 offset)const;

            private:
               void _resetMaxExtentSize(UINT32 stopWhenFound = 0);

            private:
               const UINT16 _capacity = 0;
               UINT16 _freePids = 0;
               UINT16 _maxExtentSize = 0;
               _CELL_LIST _freeCellList;
               UINT64 *_sme = nullptr;
         };//class _segmentUnit

         class _fileUnit : public SDBObject
         {
            public:
               _fileUnit(UINT32 fileId,
                         PAGE_ID firstPid,
                         const options *o,
                         std::atomic_ullong *fbits);
               ~_fileUnit();
               _fileUnit(const _fileUnit &) = delete;
               _fileUnit &operator=(const _fileUnit &) = delete;

            public:
               OSS_INLINE UINT32 getMaxFreeExtentSize()const
               {
                  return _maxFreeExtentSize.load(std::memory_order_relaxed);
               }

            public:
               void reset();

               INT32 depositSegmentFromSme(UINT64 *sme);
               INT32 depositSegment(BOOLEAN allFree);

               PAGE_ID reserveExtent(UINT32 pcnt);
               void freeExtent(PAGE_ID pid, UINT32 pcnt);
               void freePids(UINT32 size, const PAGE_ID *pids);
               BOOLEAN test(PAGE_ID pid);
            private:
               INT32 _reserveSegment();

               void _resetMaxFreeExtentSize(UINT32 stopWhenFound=0);

               void _setFileBit();
               void _clearFileBit();

            private:
               const UINT32 _fileId = 0;
               const PAGE_ID _firstPid = INVALID_PAGE_ID;
               const options *_o = nullptr;
               UINT64 _fbit = 0;
               std::atomic_ullong *_fbits = nullptr;
               std::mutex _mutex;
               std::vector<_segmentUnit *> _segments;
               boost::dynamic_bitset<> _freebits;
               std::atomic_uint _maxFreeExtentSize = {0};
               
         };//class _fileUnit


      public:
         OSS_INLINE UINT32 peekSegmentCount()const
         {
            return _totalSegmentCount;
         }
      public:
         void reset();
         void init(const options &o);

         INT32 depositWithSme(UINT64 *sme);

         INT32 deposit(BOOLEAN allFree=TRUE);

         /// return ok but invalid pid when not found.
         INT32 reserveExtent(UINT32 pcnt,
                             PAGE_ID &pid,
                             UINT32 *currentSegCount=nullptr);

         void freeExtent(PAGE_ID pid, UINT32 pcnt);

         void freePids(UINT32 size, const PAGE_ID *pids);

         void freePids(const sparseBitmap32 &bm);

         BOOLEAN test(PAGE_ID pid);

      private:

         INT32 _depositNewFileUnit();
         void _reset();
         BOOLEAN _isValidExtentToFree(PAGE_ID pid, UINT32 pcnt)const;

      private:
         ossSpinSLatchPOSIX _latch;
         options _o;
         UINT32 _totalSegmentCount = 0;
         std::vector<_fileUnit *> _funits;
         std::vector<std::atomic_ullong *> _fbits;
         ///TODO: add atomic bitmap to speed up scan?
   };//class variableExtentAllocator

#pragma pack()
} // namespace vesel

} // namespace engine


#endif//VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_