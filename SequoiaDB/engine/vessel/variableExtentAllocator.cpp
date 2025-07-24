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

   Source File Name = variableExtentAllocator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   BOOLEAN variableExtentAllocator::_segmentUnit::test(UINT32 offset)const
   {
      SDB_ASSERT(offset < (UINT32)_capacity, "out of bound");
      SDB_ASSERT(nullptr != _sme, "can not be null");
      return testBitIsNonzero(getCapacity() >> 6, _sme, offset);
   }


///////////////////////////_segmentUnit end

///////////////////////////_fileUnit
   variableExtentAllocator::_fileUnit::_fileUnit(UINT32 fileId,
                                                 PAGE_ID firstPid,
                                                 const options *o,
                                                 std::atomic_ullong *fbits):
   _fileId(fileId),
   _firstPid(firstPid),
   _o(o),
   _fbits(fbits)
   {
      SDB_ASSERT(INVALID_PAGE_ID != _firstPid, "can not be invalid");
      SDB_ASSERT(nullptr != _o, "can not be invalid");
      SDB_ASSERT(nullptr != _fbits, "can not be invalid");
      _fbit = (UINT64)1 << (_fileId % 64);
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

      _fbit = 0;
      _fbits = nullptr;
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
      _segments.push_back(segment);
      if (_o->minSegFreeCntReused <= segment->getFreePidCount())
      {
         _freebits.set(_segments.size() - 1);
         if (getMaxFreeExtentSize() < segment->getMaxFreeExtentSize())
         {
            _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(),
                                     std::memory_order_relaxed);
         }

         _setFileBit();
      }
      
      
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
      _segments.push_back(segment);
      if (allFree)
      {
         _freebits.set(_segments.size() - 1);
         _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(),
                                  std::memory_order_relaxed);
         _setFileBit();
      }
      
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
      BOOLEAN segmentOutOfSpace = FALSE;
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
               segmentOutOfSpace = TRUE;
            }
            pid = _firstPid + poffset + (pos * _o->maxPageCountPerSegment);
            resetMaxFreeExtentSize = (oldMaxFreeExtentSize == maxExtentSize) &&
                                     (oldMaxFreeExtentSize != segment->getMaxFreeExtentSize());
            break;
         }

         pos = _freebits.find_next(pos);
      }

      if (segmentOutOfSpace && _freebits.none())
      {
         _clearFileBit();
         _maxFreeExtentSize.store(0, std::memory_order_relaxed);
      }
      else if (resetMaxFreeExtentSize)
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
      BOOLEAN segmentBit = FALSE;
      BOOLEAN reused = FALSE;

      std::unique_lock<std::mutex> guard(_mutex);
      if (OSS_UNLIKELY(_segments.size() <= segmentId))
      {
         SDB_ASSERT(FALSE, "out of segment size");
         goto done;
      }
      
      segment = _segments.at(segmentId);
      segment->freeExtent(poffset, pcnt);
      segmentBit = _freebits.test(segmentId);

      if (!segmentBit &&
           _o->minSegFreeCntReused <= segment->getFreePidCount())
      {
         _freebits.set(segmentId);
         segmentBit = TRUE;
         reused = TRUE;
      }

      if (segmentBit &&
          getMaxFreeExtentSize() < segment->getMaxFreeExtentSize())
      {
         _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(),
                                  std::memory_order_relaxed);
      }

      if (reused)
      {
         _setFileBit();
      }

   done:
      return;
   }

   void variableExtentAllocator::_fileUnit::freePids(UINT32 size,
                                                     const PAGE_ID *pids)
   {
      SDB_ASSERT(0 < size && nullptr != pids, "can not be invalid");
      BOOLEAN reused = FALSE;
      std::unique_lock<std::mutex> guard(_mutex);
      for (UINT32 i = 0; i < size; ++i)
      {
         PAGE_ID pid = pids[i];
         SDB_ASSERT(INVALID_PAGE_ID != pid && _firstPid <= pid, "can not be invalid");
         UINT32 pidOffset = pid - _firstPid;
         UINT32 segmentId = pidOffset / _o->maxPageCountPerSegment;
         UINT32 poffset = pidOffset % _o->maxPageCountPerSegment;
         _segmentUnit *segment = _segments.at(segmentId);
         segment->freeExtent(poffset, 1);
         BOOLEAN segmentBit = _freebits.test(segmentId);

         if (!segmentBit &&
             _o->minSegFreeCntReused <= segment->getFreePidCount())
         {
            _freebits.set(segmentId);
            segmentBit = TRUE;
            reused = TRUE;
         }

         if (segmentBit &&
             getMaxFreeExtentSize() < segment->getMaxFreeExtentSize())
         {
            _maxFreeExtentSize.store(segment->getMaxFreeExtentSize(),
                                     std::memory_order_relaxed);
         }
      }

      if (reused)
      {
         _setFileBit();
      }

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

   void variableExtentAllocator::_fileUnit::_setFileBit()
   {
      UINT64 oldVal = _fbits->load(std::memory_order_relaxed);
      do
      {
         UINT64 newVal = oldVal;
         OSS_BIT_SET(newVal, _fbit);
         if (_fbits->compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      return;
   }

   void variableExtentAllocator::_fileUnit::_clearFileBit()
   {
      UINT64 oldVal = _fbits->load(std::memory_order_relaxed);
      do
      {
         UINT64 newVal = oldVal;
         OSS_BIT_CLEAR(newVal, _fbit);
         if (_fbits->compare_exchange_weak(oldVal, newVal))
         {
            break;
         }
      } while (TRUE);
      return;
   }

   BOOLEAN variableExtentAllocator::_fileUnit::test(PAGE_ID pid)
   {
      UINT32 pidOffset = pid - _firstPid;
      UINT32 segmentId = pidOffset / _o->maxPageCountPerSegment;
      UINT32 poffset = pidOffset % _o->maxPageCountPerSegment;
      std::unique_lock<std::mutex> guard(_mutex);
      if (_segments.size() <= segmentId)
      {
         SDB_ASSERT(FALSE, "out of bound");
         return TRUE;
      }
      else
      {
         return _segments.at(segmentId)->test(poffset);
      }
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
      for (UINT32 i = 0; i < _fbits.size(); ++i)
      {
         if (nullptr != _fbits[i])
         {
            delete _fbits[i];
         }
      }

      _funits.clear();
      _funits.shrink_to_fit();
      _totalSegmentCount = 0;
      _fbits.clear();
      _fbits.shrink_to_fit();
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

      for (UINT32 bitsPos = 0; bitsPos < _fbits.size(); ++bitsPos)
      {
         UINT64 fbits = _fbits.at(bitsPos)->load(std::memory_order_relaxed);

         while (0 != fbits)
         {
            UINT64 mask = 1;
            INT32 pos = ossGetLowestBit1From64Bits(fbits);
            SDB_ASSERT(0 <= pos, "impossible");
            UINT32 unitId = (bitsPos << 6) + pos;
            SDB_ASSERT(unitId < _funits.size(), "out of bound");
            _fileUnit *funit = _funits.at(unitId);
            if (pcnt <= funit->getMaxFreeExtentSize())
            {
               pid = funit->reserveExtent(pcnt);
               if (INVALID_PAGE_ID != pid)
               {
                  goto done;
               }
            }

            mask <<= pos;
            OSS_BIT_CLEAR(fbits, mask);
         }//while (0 != fbits)
      }//for (UINT32 bitsPos = 0; bitsPos < _fbits.size(); ++bitsPos)

   done:
      return rc;
   error:
      goto done;
   }

   void variableExtentAllocator::freeExtent(PAGE_ID pid, UINT32 pcnt)
   {
      UINT32 fileId = 0;
      ossSLatchGuard guard(&_latch, SHARED);
      
      if (OSS_UNLIKELY(!_isValidExtentToFree(pid, pcnt)))
      {
         SDB_ASSERT(FALSE, "invalid extent to free");
         goto done;
      }

      fileId = pid / (_o.maxPageCountPerSegment * _o.maxSegmentCountPerFile);
      SDB_ASSERT(fileId < _funits.size(), "impossible");
      _funits.at(fileId)->freeExtent(pid, pcnt);
   
   done:
      return;
   }

   void variableExtentAllocator::freePids(UINT32 size, const PAGE_ID *pids)
   {
      SDB_ASSERT(nullptr != pids, "can not be null");
      UINT32 batchCount = 0;
      const PAGE_ID *batch = nullptr;
      UINT32 scanPos = 0;
      UINT32 batchFd = 0;
      UINT32 maxFilePcnt = _o.maxPageCountPerSegment * _o.maxSegmentCountPerFile;
      ossSLatchGuard guard(&_latch, SHARED);
   
      while (scanPos < size)
      {
         UINT32 pos = scanPos++;
         PAGE_ID pid = pids[pos];
         UINT32 fd = pid / maxFilePcnt;
         if (_isValidExtentToFree(pid, 1))
         {
            if (0 < batchCount && fd == batchFd)
            {
               ++batchCount;
            }
            else if (0 == batchCount)
            {
               batchCount = 1;
               batch = pids + pos;
               batchFd = fd;
            }
            else
            {
               _funits.at(batchFd)->freePids(batchCount, batch);
               batchFd = fd;
               batchCount = 1;
               batch = pids + pos;
            }
         }
         else
         {
            if (0 < batchCount)
            {
               _funits.at(batchFd)->freePids(batchCount, batch);
               batchFd = 0;
               batchCount = 0;
               batch = nullptr;
            }
         }
      }//while (scanPos < size)

      if (0 < batchCount)
      {
         _funits.at(batchFd)->freePids(batchCount, batch);
      }

      return;
   }

   void variableExtentAllocator::freePids(const sparseBitmap32 &bm)
   {
      constexpr UINT32 BATCH_SIZE = 16;
      std::array<PAGE_ID, BATCH_SIZE> batch;
      UINT32 size = 0;
      UINT32 fd = 0;
      const UINT32 maxFilePcnt = _o.maxPageCountPerSegment * _o.maxSegmentCountPerFile;
      ossSLatchGuard guard(&_latch, SHARED);
      sparseBitmap32::iterator itr;
      while (bm.next(itr))
      {
         PAGE_ID pid = itr.get();
         UINT32 currentFd = pid / maxFilePcnt;
         if (OSS_UNLIKELY(!_isValidExtentToFree(pid, 1)))
         {
            SDB_ASSERT(FALSE, "invalid pid to free");
            continue;
         }

         if (0 == size)
         {
            batch[size++] = pid;
            fd = currentFd;
         }
         else if (fd == currentFd)
         {
            batch[size++] = pid;
            if (BATCH_SIZE == size)
            {
               _funits[fd]->freePids(size, batch.data());
               size = 0;
               fd = 0;
            }
         }
         else
         {
            SDB_ASSERT(0 < size, "impossible");
            _funits[fd]->freePids(size, batch.data());
            size = 0;
            fd = currentFd;
            batch[size++] = pid;
         }
      }

      if (0 < size)
      {
         _funits[fd]->freePids(size, batch.data());
      }

      return;
   }

   BOOLEAN variableExtentAllocator::test(PAGE_ID pid)
   {
      UINT32 maxFilePcnt = _o.maxPageCountPerSegment * _o.maxSegmentCountPerFile;
      UINT32 fd = pid / maxFilePcnt;
      ossSLatchGuard guard(&_latch, SHARED);
      SDB_ASSERT(fd < _funits.size(), "out of bound");
      return _funits.at(fd)->test(pid);
   }


   INT32 variableExtentAllocator::_depositNewFileUnit()
   {
      INT32 rc = SDB_OK;
      PAGE_ID firstPid = INVALID_PAGE_ID;
      _fileUnit *funit = nullptr;
      std::atomic_ullong *bits = nullptr;
      std::atomic_ullong *bitsToRegister = nullptr;
      UINT32 bitsPos = _funits.size() >> 6;

      try
      {
         _funits.reserve(1);
         if (bitsPos == _fbits.size())
         {
            _fbits.reserve(1);
         }  
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to reserve file:%s", e.what());
         rc = SDB_OOM;
         goto error;
      }

      if (bitsPos == _fbits.size())
      {
         bits = new(std::nothrow) std::atomic_ullong(0);
         if (nullptr == bits)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         bitsToRegister = bits;
      }
      else
      {
         bitsToRegister = _fbits.at(bitsPos);
      }
      
      SDB_ASSERT(nullptr != bitsToRegister, "impossible");
      firstPid = _funits.size() *
                 _o.maxPageCountPerSegment * _o.maxSegmentCountPerFile;
      funit = SDB_OSS_NEW _fileUnit(_funits.size(), firstPid, &_o, bitsToRegister);
      if (OSS_UNLIKELY(nullptr == funit))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      /// do not goto error from here
      _funits.push_back(funit);
      if (nullptr != bits)
      {
         _fbits.push_back(bits);
      }
      
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(funit);
      if (nullptr != bits)
      {
         delete bits;
      }
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
