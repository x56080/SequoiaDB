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

   Source File Name = variableExtentAllocator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_
#define VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_

#include "ossMemPool.hpp"
#include "vessel/pageIdentifier.h"
#include "ossLatch.hpp"
#include <mutex> //c++11

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class variableExtentAllocator : public SDBObject
   {
      public:
         variableExtentAllocator(){}
         ~variableExtentAllocator();
         variableExtentAllocator(const variableExtentAllocator &) = delete;
         variableExtentAllocator &operator=(const variableExtentAllocator &) = delete;

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
            _extentCell(){}
            explicit _extentCell(UINT16 o, UINT16 s):
            offset(o),
            size(s){}
            _extentCell(const _extentCell &o):
            offset(o.offset),
            size(o.size){}
            ~_extentCell(){}
            _extentCell &operator=(const _extentCell &o)
            {
               offset = o.offset;
               size = o.size;
               return *this;
            }

            OSS_INLINE UINT16 getHighBound()const
            {
               return offset + size;
            }

            CELL_CMP_RES compare(const _extentCell &o)const;

            UINT16 offset = 0;
            UINT16 size = 0;
         };//struct _extentCell

         typedef class ossPoolList<_extentCell> _CELL_LIST;

         typedef std::pair<UINT32, UINT32> _SEG_PROFILE;
         struct _FREE_MAP_CMP
         {
            OSS_INLINE BOOLEAN operator()(const _SEG_PROFILE &l,
                                          const _SEG_PROFILE &r)const
            {
               if (l.first < r.first)
               {
                  return TRUE;
               }
               else if (l.first > r.first)
               {
                  return FALSE;
               }
               else
               {
                  return l.second < r.second;
               }
            }
         };//struct _FREE_MAP_CMP

         typedef ossPoolSet<_SEG_PROFILE, _FREE_MAP_CMP> _FREE_MAP;

         class _segmentUnit : public SDBObject
         {
            public:
               explicit _segmentUnit(PAGE_ID firstPid,
                                     UINT32 capacity,
                                     UINT64 *sme,
                                     BOOLEAN loadSme);
               ~_segmentUnit(){}
               _segmentUnit(const _segmentUnit &) = delete;
               _segmentUnit &operator=(const _segmentUnit &) = delete;

            public:
               OSS_INLINE std::mutex &getMutex(){return _mutex;}
               OSS_INLINE UINT32 getCapacity()const {return _capacity;}
               OSS_INLINE PAGE_ID getFirstPid()const {return _firstPid;}
               OSS_INLINE PAGE_ID getUpperBoundPid()const {return _firstPid + _capacity;}
               OSS_INLINE UINT32 getMaxExtentSize()const {return _maxExtentSize;}
               OSS_INLINE UINT32 getFreeCount()const {return _freeCount;}
               
               PAGE_ID reserve(UINT32 pcnt);
               void release(PAGE_ID pid, UINT32 pcnt);

            public:
               OSS_INLINE BOOLEAN isRegistered()const
               {
                  return _FREE_MAP::iterator() != pos;
               }

               _FREE_MAP::iterator pos;

            private:
               void _loadSme();

            private:
               std::mutex _mutex;
               const PAGE_ID _firstPid = INVALID_PAGE_ID;
               const UINT16 _capacity = 0;
               UINT16 _freeCount = 0;

               /// we only update _maxExtentSize in time
               /// when failed to reserve or release extent.
               UINT16 _maxExtentSize = 0;
               UINT16 _flags = 0;
               _CELL_LIST _freeCellList;
               UINT64 *_sme = nullptr;
         };//class _segmentUnit

         typedef std::vector<_segmentUnit *> _SEGMENT_VEC;


      public:
         OSS_INLINE UINT32 peekSegmentCount()const
         {
            return _units.size();
         }
      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _segmentPageCnt;}
         void clear();

         void init(UINT32 pageCntPerSeg, UINT32 maxExtentSize);

         INT32 depositWithSme(UINT64 *sme, BOOLEAN ensuredAllFree);
         INT32 deposit();

         PAGE_ID reserve(UINT32 pcnt, UINT32 *currentSegCount=nullptr);

         void release(PAGE_ID pid, UINT32 pcnt);

      private:
         void _clear();
         void _eraseFromMap(_segmentUnit *segment);
         void _insertIntoMap(_segmentUnit *segment, UINT32 segmentId);
         void _reinsertIntoMap(_segmentUnit *segment);
         _segmentUnit *_findFromMap(UINT32 pcnt, UINT32 &segmentId);

      private:
         static constexpr UINT32 MIN_FREE_COUNT_TO_REGISTER = 16; 

      private:
         ossSpinSLatchPOSIX _latch;
         UINT32 _segmentPageCnt = 0;
         UINT32 _maxExtentSize = 0; /// max page count of extent
         _SEGMENT_VEC _units;
         ossSpinSLatchPOSIX _mapLatch;
         _FREE_MAP _freeMap;
   };//class variableExtentAllocator

#pragma pack()
} // namespace vesel

} // namespace engine


#endif//VESSEL_VARIABLE_EXTENT_ALLOCATOR_H_