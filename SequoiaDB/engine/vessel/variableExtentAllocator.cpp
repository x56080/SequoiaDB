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

   Source File Name = variableExtentAllocator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/variableExtentAllocator.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/bitmapUtils.h"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
///////////////////////////_extentCell
   variableExtentAllocator::CELL_CMP_RES
   variableExtentAllocator::_extentCell::compare(const _extentCell &o)const
   {
      UINT16 oHigh = o.getUpperBound();
      UINT16 thisHigh = getUpperBound();

      if (thisHigh < o.offset)
      {
         return CELL_CMP_RES::LOWER_WITH_HOLE;
      }
      else if (thisHigh == o.offset)
      {
         return CELL_CMP_RES::LOWER_WITH_NO_HOLE;
      }
      else if (oHigh < this->offset)
      {
         return CELL_CMP_RES::UPPER_WITH_HOLE;
      }
      else if (oHigh == this->offset)
      {
         return CELL_CMP_RES::UPPER_WITH_NO_HOLE;
      }
      else
      {
         return CELL_CMP_RES::INTERSECTIED;
      }
   }
///////////////////////////_extentCell end

///////////////////////////_segmentUnit
   variableExtentAllocator:: _segmentUnit::_segmentUnit(UINT32 capacity):
   _capacity(capacity)
   {
      SDB_ASSERT(capacity < 65536, "out of size");
      SDB_ASSERT(ossIsAligned64(capacity), "must be 64 aligned");
   }

   void variableExtentAllocator::_segmentUnit::clear()
   {
      _freePids = 0;
      _maxExtentSize = 0;
      _freeCellList.clear();
      _sme = nullptr;
      return;
   }

   void variableExtentAllocator::_segmentUnit::init(BOOLEAN allFree)
   {
      clear();
      if (allFree)
      {
         _freePids = _capacity;
         _maxExtentSize = _capacity;
         _freeCellList.emplace_back(0, _capacity);
      }
      return;
   }

   void variableExtentAllocator::_segmentUnit::initFromSme(UINT64 *sme)
   {
      SDB_ASSERT(nullptr != sme, "can not be invalid");
      clear();

      _sme = sme;
      _freePids = getNonzeroBitCount(_capacity >> 6, _sme);
      
      if (_freePids == _capacity)
      {
         _maxExtentSize = _capacity;
         _freeCellList.emplace_back(0, _capacity);
      }
      else if (0 < _freePids)
      {
         UINT16 maxExtentSize = 0;
         UINT32 bits = _capacity >> 6;
         UINT16 offset = 0;
         UINT16 size = 0;

         for (UINT16 i = 0; i < _capacity; ++i)
         {
            if (testBitIsNonzero(bits, _sme, i))
            {
               if (0 == size)
               {
                  offset = i;
                  size = 1;
               }
               else
               {
                  ++size;
               }
            }
            else if (0 < size)
            {
               _freeCellList.emplace_back(offset, size);
               if (maxExtentSize < size)
               {
                  maxExtentSize = size;
               }
               offset = 0;
               size = 0;
            }
            else
            {
               /// do nothing.
            }
         }//for (UINT16 i = 0; i < _capacity; ++i)

         if (0 < size)
         {
            _freeCellList.emplace_back(offset, size);
            if (maxExtentSize < size)
            {
               maxExtentSize = size;
            }
         }

         _maxExtentSize = maxExtentSize;
      }

      return;
   }

   INT32 variableExtentAllocator::_segmentUnit::reserveExtent(UINT32 pcnt)
   {
      SDB_ASSERT(0 < pcnt && pcnt <= _capacity, "can not be invalid");
      
      INT32 poffset = -1;
      BOOLEAN resetMaxExtent = FALSE;

      if (_maxExtentSize < pcnt)
      {
         goto done;
      }

      for (_CELL_LIST::iterator itr = _freeCellList.begin();
           itr != _freeCellList.end(); ++itr)
      {
         _extentCell &cell = *itr;
         if (pcnt <= cell.size)
         {
            UINT32 offset = cell.offset;
            resetMaxExtent = (cell.size == _maxExtentSize);
            poffset = cell.offset;
            SDB_ASSERT(pcnt <= _freePids, "impossible");
            _freePids -= pcnt;

            if (pcnt < cell.size)
            {
               cell.offset += pcnt;
               cell.size -= pcnt;
            }
            else
            {
               _freeCellList.erase(itr);
            }

            if (nullptr != _sme)
            {
               batchClearBits(getCapacity() >> 6, offset, offset + pcnt - 1, _sme);
#if defined(_DEBUG)
               BOOLEAN r = batchTestBitsAllZeroed(getCapacity() >> 6, poffset, poffset + pcnt - 1, _sme);
               SDB_ASSERT(r, "error bit set");
#endif//_DEBUG
            }
            break;
         }
      }

      if (resetMaxExtent)
      {
         _resetMaxExtentSize(_maxExtentSize);
      }

   done:
      return poffset;
   }

   void variableExtentAllocator::_segmentUnit::freeExtent(UINT32 poffset,
                                                          UINT32 pcnt)
   {
      SDB_ASSERT(poffset < _capacity  && 0 < pcnt, "can not be invalid");
      SDB_ASSERT((poffset + pcnt) <= _capacity, "out of bound");
      SDB_ASSERT((_freePids + pcnt) <= _capacity, "out ouf bound");
      
      _CELL_LIST::iterator itr = _freeCellList.begin();
      UINT16 mergedExtentSize = 0;
      _extentCell cell(poffset, pcnt);

      if (OSS_UNLIKELY(_capacity < (poffset + pcnt)))
      {
         PD_LOG(PDERROR, "extent[%d,%d] out of size", poffset, pcnt);
         goto done;
      }

      if (nullptr != _sme)
      {
         if (!batchTestBitsAllZeroed(getCapacity() >> 6, poffset, poffset + pcnt - 1, _sme))
         {
            PD_LOG(PDSEVERE, "invalid pids to be released[%d,%d]", poffset, pcnt);
            SDB_ASSERT(FALSE, "invalid pids to be released");
            goto done;
         }
      }

      for (; itr != _freeCellList.end(); ++itr)
      {
         _extentCell &c = *itr;

         CELL_CMP_RES res = c.compare(cell);
         if (CELL_CMP_RES::LOWER_WITH_HOLE == res)
         {
            continue;
         }
         else if (CELL_CMP_RES::LOWER_WITH_NO_HOLE == res)
         {
            c.size += cell.size;
            mergedExtentSize = c.size;
            /// try to merge right cells
            _CELL_LIST::iterator rightItr = itr;
            ++rightItr;
            if (_freeCellList.end() != rightItr)
            {
               res = c.compare(*rightItr);
               if (CELL_CMP_RES::LOWER_WITH_NO_HOLE == res)
               {
                  c.size += rightItr->size;
                  mergedExtentSize = c.size;
                  _freeCellList.erase(rightItr);
               }
               else
               {
                  SDB_ASSERT(CELL_CMP_RES::LOWER_WITH_HOLE == res, "invalid right cell");
               }
            }
            break;
         }
         else if (CELL_CMP_RES::UPPER_WITH_HOLE == res)
         {
            _freeCellList.insert(itr, cell);
            mergedExtentSize = cell.size;
            break;
         }
         else if (CELL_CMP_RES::UPPER_WITH_NO_HOLE == res)
         {
            c.offset = cell.offset;
            c.size += cell.size;
            mergedExtentSize = c.size;
            break;
         }
         else
         {
            PD_LOG(PDSEVERE, "first pid:%d, current cell[%d,%d], cell to released[%d, %d]",
                   poffset, c.offset, c.size, cell.offset, cell.size);
            SDB_ASSERT(FALSE, "invalid extent to be released");
            goto done;
         }
      }

      /// empty free list or greater than all extents in list.
      if (0 == mergedExtentSize)
      {
#if defined(_DEBUG)
         SDB_ASSERT(_freeCellList.empty() ||
                    _freeCellList.back().getUpperBound() < cell.offset, "impossible");
#endif//_DEBUG
         _freeCellList.push_back(cell);
         mergedExtentSize = cell.size;
      }

      _freePids += pcnt;
      if (_maxExtentSize < mergedExtentSize)
      {
         _maxExtentSize = mergedExtentSize;
      }
      if (nullptr != _sme)
      {
         batchSetBits(getCapacity() >> 6, poffset, poffset + pcnt - 1, _sme);
#if defined(_DEBUG)
         BOOLEAN r = batchTestBitsNonZeroed(getCapacity() >> 6, poffset, poffset + pcnt - 1, _sme);
         SDB_ASSERT(r, "error bit set");
#endif//_DEBUG
      }

   done:
      return;
   }

   void variableExtentAllocator::_segmentUnit::_resetMaxExtentSize(UINT32 stopWhenFound)
   {
      UINT16 maxExtentSize = 0;
      for (auto itr = _freeCellList.cbegin(); itr != _freeCellList.cend(); ++itr)
      {
         if (maxExtentSize < itr->size)
         {
            maxExtentSize = itr->size;
            if (maxExtentSize == stopWhenFound)
            {
               break;
            }
         }
      }

      _maxExtentSize = maxExtentSize;
      return;
   }  

///////////////////////////_segmentUnit end

///////////////////////////_fileUnit
   variableExtentAllocator::_fileUnit::_fileUnit(PAGE_ID firstPid,
                                                 const options *o,
                                                 std::atomic_int *stats):
   _firstPid(firstPid),
   _o(o),
   _globalSegStats(stats)
   {
      SDB_ASSERT(INVALID_PAGE_ID != _firstPid, "can not be invalid");
      SDB_ASSERT(nullptr != _o, "can not be invalid");
      SDB_ASSERT(nullptr != _globalSegStats, "can not be invalid");
   }

   variableExtentAllocator::_fileUnit::~_fileUnit()
   {
      for (UINT32 i = 0; i < _segments.size(); ++i)
      {
         if (nullptr != _segments.at(i))
         {
            SDB_OSS_DEL _segments.at(i);
         }
      }
   }

   void variableExtentAllocator::_fileUnit::reset()
   {
      for (UINT32 i = 0; i < _segments.size(); ++i)
      {
         if (nullptr != _segments.at(i))
         {
            SDB_OSS_DEL _segments.at(i);
         }
      }
      _segments.clear();
      _segments.shrink_to_fit();
      _freebits.clear();
      _maxFreeExtentSize.store(0, std::memory_order_relaxed);
      return;
   }

   INT32 variableExtentAllocator::_fileUnit::depositSegmentFromSme(UINT64 *sme)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != sme, "can not be null");
      _segmentUnit *segment = nullptr;

      std::unique_lock<std::mutex> guard(_mutex);
      if (OSS_UNLIKELY(nullptr == sme))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_segments.size() == _o->maxSegmentCountPerFile))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _reserveSegment();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      segment = SDB_OSS_NEW _segmentUnit(_o->maxPageCountPerSegment);
      if (OSS_UNLIKELY(nullptr == segment))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      segment->initFromSme(sme);
      if (_o->minSegFreeCntReused <= segment->getFreePidCount())
      {
         _freebits.set(_segments.size());
         if (getMaxFreeExtentSize() < segment->getMaxFreeExtentSize())
         {
            _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(), std::memory_order_relaxed);
         }
         _globalSegStats->fetch_add(1, std::memory_order_relaxed);
      }
      _segments.push_back(segment);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 variableExtentAllocator::_fileUnit::depositSegment(BOOLEAN allFree)
   {
      INT32 rc = SDB_OK;
      _segmentUnit *segment = nullptr;

      std::unique_lock<std::mutex> guard(_mutex);
      if (OSS_UNLIKELY(_segments.size() == _o->maxSegmentCountPerFile))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _reserveSegment();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      segment = SDB_OSS_NEW _segmentUnit(_o->maxPageCountPerSegment);
      if (OSS_UNLIKELY(nullptr == segment))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      segment->init(allFree);
      if (allFree)
      {
         _freebits.set(_segments.size());
         _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(), std::memory_order_relaxed);
         _globalSegStats->fetch_add(1, std::memory_order_relaxed);
      }
      _segments.push_back(segment);
   done:
      return rc;
   error:
      goto done;
   }

   PAGE_ID variableExtentAllocator::_fileUnit::reserveExtent(UINT32 pcnt)
   {
      SDB_ASSERT(0 < pcnt && pcnt <= _o->maxPageCountPerSegment, "can not be invalid");
      PAGE_ID pid = INVALID_PAGE_ID;
      BOOLEAN resetMaxFreeExtentSize = FALSE;
      std::unique_lock<std::mutex> guard(_mutex);
      std::size_t pos = 0;

      UINT32 maxExtentSize = getMaxFreeExtentSize();

      if (maxExtentSize < pcnt)
      {
         goto done;
      }

      pos = _freebits.find_first();
      while (boost::dynamic_bitset<>::npos != pos)
      {
         _segmentUnit *segment = _segments.at(pos);
         if (pcnt <= segment->getMaxFreeExtentSize())
         {
            UINT32 oldMaxFreeExtentSize = segment->getMaxFreeExtentSize();
            INT32 poffset = segment->reserveExtent(pcnt);
            SDB_ASSERT(0 <= poffset, "impossible");
            if (0 == segment->getMaxFreeExtentSize())
            {
               _freebits.reset(pos);
               _globalSegStats->fetch_sub(1, std::memory_order_relaxed);
            }
            pid = _firstPid + poffset + (pos * _o->maxPageCountPerSegment);
            resetMaxFreeExtentSize = (oldMaxFreeExtentSize == maxExtentSize) &&
                                     (oldMaxFreeExtentSize != segment->getMaxFreeExtentSize());
            break;
         }

         pos = _freebits.find_next(pos);
      }

      if (resetMaxFreeExtentSize)
      {
         _resetMaxFreeExtentSize(maxExtentSize);
      }

   done:
      return pid;
   }

   void variableExtentAllocator::_fileUnit::freeExtent(PAGE_ID pid, UINT32 pcnt)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid && 0 < pcnt, "can not be invalid");
      SDB_ASSERT(_firstPid <= pid && pcnt <= _o->maxPageCountPerSegment, "can not be invalid");

      UINT32 pidOffset = pid - _firstPid;
      UINT32 segmentId = pidOffset / _o->maxPageCountPerSegment;
      UINT32 poffset = pidOffset % _o->maxPageCountPerSegment;
      _segmentUnit *segment = nullptr;

      std::unique_lock<std::mutex> guard(_mutex);
      if (OSS_UNLIKELY(_segments.size() <= segmentId))
      {
         SDB_ASSERT(FALSE, "out of segment size");
         goto done;
      }
      
      segment = _segments.at(segmentId);
      segment->freeExtent(poffset, pcnt);
      if (!_freebits.test(segmentId) &&
           _o->minSegFreeCntReused <= segment->getFreePidCount())
      {
         _freebits.set(segmentId);
         _globalSegStats->fetch_add(1, std::memory_order_relaxed);
      }

      if (_freebits.test(segmentId) &&
          getMaxFreeExtentSize() < segment->getMaxFreeExtentSize())
      {
         _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(),
                                  std::memory_order_relaxed);
      }

   done:
      return;
   }

   INT32 variableExtentAllocator::_fileUnit::_reserveSegment()
   {
      INT32 rc = SDB_OK;
      try
      {
         _segments.reserve(1);
         _freebits.resize(_segments.size() + 1, FALSE);
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to reserve element:%s", e.what());
         rc = SDB_OOM;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void variableExtentAllocator::_fileUnit::_resetMaxFreeExtentSize(UINT32 stopWhenFound)
   {
      UINT32 maxExtentSize = 0;
      std::size_t pos = _freebits.find_first();
      while (boost::dynamic_bitset<>::npos != pos)
      {
         _segmentUnit *segment = _segments.at(pos);
         if (maxExtentSize < segment->getMaxFreeExtentSize())
         {
            maxExtentSize = segment->getMaxFreeExtentSize();
            if (maxExtentSize == stopWhenFound)
            {
               break;
            }
         }

         pos = _freebits.find_next(pos);
      }

      _maxFreeExtentSize.store(maxExtentSize, std::memory_order_relaxed);
      return;
   }

///////////////////////////_fileUnit end

   variableExtentAllocator::~variableExtentAllocator()
   {
      for (UINT32 i = 0; i < _funits.size(); ++i)
      {
         if (nullptr != _funits[i])
         {
            SDB_OSS_DEL _funits[i];
         }
      }
   }

   void variableExtentAllocator::reset()
   {
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _reset();
   }

   void variableExtentAllocator::_reset()
   {
      for (UINT32 i = 0; i < _funits.size(); ++i)
      {
         if (nullptr != _funits[i])
         {
            SDB_OSS_DEL _funits[i];
         }
      }
      _funits.clear();
      _funits.shrink_to_fit();
      _totalSegmentCount = 0;
      _freeSegments.store(0, std::memory_order_relaxed);
      _o = options();
      return;
   }

   void variableExtentAllocator::init(const options &o)
   {
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _reset();
      SDB_ASSERT(0 < o.maxPageCountPerSegment, "can not be invalid");
      SDB_ASSERT(0 < o.maxSegmentCountPerFile, "can not be invalid");
      SDB_ASSERT(o.minSegFreeCntReused < o.maxPageCountPerSegment, "out of bound");

      _o = o;
   }

   INT32 variableExtentAllocator::deposit(BOOLEAN allFree)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _o.maxSegmentCountPerFile, "can not be invalid");
      ossSLatchGuard guard(&_latch, EXCLUSIVE);

      if (0 == _totalSegmentCount % _o.maxSegmentCountPerFile)
      {
         rc = _depositNewFileUnit();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to deposit new file unit:%d", rc);
            goto error;
         }
      }

      rc = _funits.back()->depositSegment(allFree);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to deposit new segment:%d", rc);
         goto error;
      }

      ++_totalSegmentCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 variableExtentAllocator::depositWithSme(UINT64 *sme)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _o.maxSegmentCountPerFile, "can not be invalid");
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      if (OSS_UNLIKELY(nullptr == sme))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (0 == _totalSegmentCount % _o.maxSegmentCountPerFile)
      {
         rc = _depositNewFileUnit();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to deposit new file unit:%d", rc);
            goto error;
         }
      }

      rc = _funits.back()->depositSegmentFromSme(sme);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to deposit new segment:%d", rc);
         goto error;
      }

      ++_totalSegmentCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 variableExtentAllocator::reserveExtent(UINT32 pcnt,
                                                PAGE_ID &pid,
                                                UINT32 *currentSegCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < pcnt && pcnt <= _o.maxPageCountPerSegment, "can not be invalid");
      pid = INVALID_PAGE_ID;
      ossSLatchGuard guard(&_latch, SHARED);

      if (nullptr != currentSegCount)
      {
         *currentSegCount = _totalSegmentCount;
      }

      if (OSS_UNLIKELY(0 == pcnt || _o.maxPageCountPerSegment < pcnt))
      {
         SDB_ASSERT(FALSE, "pcnt out of size");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (getFreeSegStats() <= 0)
      {
         goto done;
      }

      for (auto ritr = _funits.rbegin(); ritr != _funits.rend(); ++ritr)
      {
         _fileUnit *funit = *ritr;
         if (pcnt <= funit->getMaxFreeExtentSize())
         {
            pid = funit->reserveExtent(pcnt);
            if (INVALID_PAGE_ID != pid)
            {
               break;
            }
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void variableExtentAllocator::freeExtent(PAGE_ID pid, UINT32 pcnt)
   {
      UINT32 fileId = 0;
      ossSLatchGuard guard(&_latch, SHARED);
      
      if (!_isValidExtentToFree(pid, pcnt))
      {
         goto done;
      }

      fileId = pid / (_o.maxPageCountPerSegment * _o.maxSegmentCountPerFile);
      SDB_ASSERT(fileId < _funits.size(), "impossible");
      _funits.at(fileId)->freeExtent(pid, pcnt);
   
   done:
      return;
   }

   INT32 variableExtentAllocator::_depositNewFileUnit()
   {
      INT32 rc = SDB_OK;
      PAGE_ID firstPid = _funits.size() *
                         _o.maxPageCountPerSegment * _o.maxSegmentCountPerFile;
      _fileUnit *funit = SDB_OSS_NEW _fileUnit(firstPid, &_o, &_freeSegments);
      if (OSS_UNLIKELY(nullptr == funit))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      try
      {
         _funits.push_back(funit);
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to reserve file:%s", e.what());
         rc = SDB_OOM;
         goto error;
      }
      
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(funit);
      goto done;
   }

   BOOLEAN variableExtentAllocator::_isValidExtentToFree(PAGE_ID pid, UINT32 pcnt)const
   {
      BOOLEAN r = FALSE;
      if (INVALID_PAGE_ID != pid && 0 < pcnt)
      {
         /// make sure that extent not out of segment size
         if (((pid % _o.maxPageCountPerSegment) + pcnt) <= _o.maxPageCountPerSegment)
         {
            /// make sure that not out of total segment count
            if ((pid / _o.maxPageCountPerSegment) < _totalSegmentCount)
            {
               r = TRUE;
            }
         }
      }

      if (!r)
      {
         PD_LOG(PDERROR, "invalid extent[%d,%d] to free", pid, pcnt);
      }
      return r;
   }
} // namespace vessel

} // namespace engine
