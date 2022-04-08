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
      UINT16 oHigh = o.getHighBound();
      UINT16 thisHigh = getHighBound();

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
   variableExtentAllocator:: _segmentUnit::_segmentUnit(PAGE_ID firstPid,
                                                        UINT32 capacity,
                                                        UINT64 *sme,
                                                        BOOLEAN loadSme):
   _firstPid(firstPid),
   _capacity(capacity),
   _sme(sme)
   {
      SDB_ASSERT(INVALID_PAGE_ID != firstPid, "can not be invalid");
      SDB_ASSERT(capacity < 65536, "out of size");
      SDB_ASSERT(ossIsAligned64(capacity), "must be 64 aligned");

      if (!loadSme)
      {
#if defined (_DEBUG)
         if (nullptr != _sme)
         {
            SDB_ASSERT(capacity == getNonzeroBitCount(capacity >> 6, sme),
                       "must be all free");
         }
#endif//_DEBUG
         _freeCount = _capacity;
         _maxExtentSize = _capacity;
         _freeCellList.push_back(_extentCell(0, _capacity));
      }
      else
      {
         _loadSme();
      }

   }

   PAGE_ID variableExtentAllocator::_segmentUnit::reserve(UINT32 pcnt)
   {
      SDB_ASSERT(0 < pcnt && pcnt <= _capacity, "can not be invalid");
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT16 maxExtentSize = 0;

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
            pid = _firstPid + cell.offset;
            cell.offset += pcnt;
            cell.size -= pcnt;
            SDB_ASSERT(pcnt <= _freeCount, "impossible");
            _freeCount -= pcnt;
            if (0 == cell.size)
            {
               _freeCellList.erase(itr);
            }
            if (nullptr != _sme)
            {
               batchClearBits(getCapacity() >> 6, offset, offset + pcnt - 1, _sme);
            }
            break;
         }
         else if (maxExtentSize < cell.size)
         {
            maxExtentSize = cell.size;
         }
      }

      /// no suitable extent exists
      if (INVALID_PAGE_ID == pid)
      {
         _maxExtentSize = maxExtentSize;
      }

   done:
      return pid;
   }

   void variableExtentAllocator::_segmentUnit::release(PAGE_ID pid,
                                                       UINT32 pcnt)
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid && 0 < pcnt, "can not be invalid");
      _extentCell cell;
      _CELL_LIST::iterator itr = _freeCellList.begin();
      UINT16 mergedExtentSize = 0;
      UINT32 offset = 0;

      if (pid < _firstPid || getUpperBoundPid() < (pid + pcnt))
      {
         PD_LOG(PDERROR, "invalid extent");
         goto done;
      }

      offset = pid - _firstPid;
      if (nullptr != _sme)
      {
         if (!batchTestBitsAllZeroed(getCapacity() >> 6, offset, offset + pcnt - 1, _sme))
         {
            PD_LOG(PDSEVERE, "invalid pids to be released[%d,%d]", pid, pcnt);
            SDB_ASSERT(FALSE, "invalid pids to be released");
            goto done;
         }
      }

      SDB_ASSERT((_freeCount + pcnt) <= _capacity, "out of bound");

      cell.offset = static_cast<UINT16>(offset);
      cell.size = pcnt;

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
            _freeCount += pcnt;
            /// try to merge right cells
            _CELL_LIST::iterator rightItr = itr;
            ++rightItr;
            while (_freeCellList.end() != rightItr)
            {
               res = c.compare(*rightItr);
               if (CELL_CMP_RES::LOWER_WITH_NO_HOLE == res)
               {
                  mergedExtentSize += rightItr->size;
                  c.size += rightItr->size;
                  rightItr = _freeCellList.erase(rightItr);
               }
               else if (CELL_CMP_RES::LOWER_WITH_HOLE == res)
               {
                  break;
               }
               else
               {
                  PD_LOG(PDSEVERE, "first pid:%d, merged cell[%d,%d], next cell[%d, %d]",
                         _firstPid, c.offset, c.size, rightItr->offset, rightItr->size);
                  SDB_ASSERT(FALSE, "invalid extent to be released");
                  break;
               }
            }
            break;
         }
         else if (CELL_CMP_RES::UPPER_WITH_HOLE == res)
         {
            _freeCellList.insert(itr, cell);
            mergedExtentSize = cell.size;
            _freeCount += pcnt;
            break;
         }
         else if (CELL_CMP_RES::UPPER_WITH_NO_HOLE == res)
         {
            c.offset = cell.offset;
            c.size += cell.size;
            mergedExtentSize = c.size;
            _freeCount += pcnt;
            break;
         }
         else
         {
            PD_LOG(PDSEVERE, "first pid:%d, current cell[%d,%d], cell to released[%d, %d]",
                   _firstPid, c.offset, c.size, cell.offset, cell.size);
            SDB_ASSERT(FALSE, "invalid extent to be released");
            goto done;
         }
      }

      /// empty free list or greater than all extents in list.
      if (_freeCellList.end() == itr)
      {
         _freeCellList.push_back(cell);
         mergedExtentSize = cell.size;
         _freeCount += cell.size;
      }

      if (_maxExtentSize < mergedExtentSize)
      {
         _maxExtentSize = mergedExtentSize;
      }

      if (0 < mergedExtentSize && nullptr != _sme)
      {
         batchSetBits(getCapacity() >> 6, offset, offset + pcnt - 1, _sme);
      }

   done:
      return;
   }

   void variableExtentAllocator::_segmentUnit::_loadSme()
   {
      SDB_ASSERT(nullptr != _sme, "can not be null");
      _freeCount = 0;
      _maxExtentSize = 0;
      _freeCellList.clear();

      _freeCount = getNonzeroBitCount(_capacity >> 6, _sme);
      if (_capacity == _freeCount)
      {
         _maxExtentSize = _capacity;
         _freeCellList.push_back(_extentCell(0, _capacity));
      }
      else if (0 < _freeCount)
      {
         UINT32 bitsCount = _capacity >> 6;
         INT32 offset = -1;
         UINT32 count = 0;
         for (UINT32 i = 0; i < _capacity; ++i)
         {
            if (testBitIsNonzero(bitsCount, _sme, i))
            {
               ++_freeCount;

               if (offset < 0)
               {
                  offset = i;
                  count = 1;
               }
               else
               {
                  SDB_ASSERT((offset + count) == i, "must be the next one");
                  ++count;
               }
            }
            else if (0 <= offset)
            {
               _freeCellList.push_back(_extentCell(offset, count));
               if (_maxExtentSize < count)
               {
                  _maxExtentSize = count;
               }
               offset = -1;
               count = 0;
            }
            else
            {
               /// do nothing.
            }
         }

         if (0 <= offset)
         {
            _freeCellList.push_back(_extentCell(offset, count));
            if (_maxExtentSize < count)
            {
               _maxExtentSize = count;
            }
         }
      }
   }
///////////////////////////_segmentUnit end

   variableExtentAllocator::~variableExtentAllocator()
   {
      _clear();
   }

   void variableExtentAllocator::clear()
   {
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _clear();
   }

   void variableExtentAllocator::_clear()
   {
      _freeMap.clear();
      for (UINT32 i = 0; i < _units.size(); ++i)
      {
         if (nullptr != _units[i])
         {
            SDB_OSS_DEL _units[i];
         }
      }
      _units.clear();
      _segmentPageCnt = 0;
      _maxExtentSize = 0;
   }

   void variableExtentAllocator::init(UINT32 pageCntPerSeg, UINT32 maxExtentSize)
   {
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _clear();
      SDB_ASSERT(0 < pageCntPerSeg, "can not be invalid");
      SDB_ASSERT(ossIsAligned64(pageCntPerSeg), "must be 64 aligned");
      SDB_ASSERT(0 < maxExtentSize, "can not be invalid");
      SDB_ASSERT(maxExtentSize <= pageCntPerSeg, "can not be invalid");
      _segmentPageCnt = pageCntPerSeg;
      _maxExtentSize = maxExtentSize;
   }

   INT32 variableExtentAllocator::depositWithSme(UINT64 *sme, BOOLEAN ensuredAllFree)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == sme))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         ossSLatchGuard guard(&_latch, EXCLUSIVE);
         PAGE_ID pid = _units.size() * _segmentPageCnt;
         _segmentUnit *segment = SDB_OSS_NEW _segmentUnit(pid, _segmentPageCnt,
                                                          sme, !ensuredAllFree);
         if (OSS_UNLIKELY(nullptr == segment))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         if (MIN_FREE_COUNT_TO_REGISTER <= segment->getFreeCount())
         {
            _SEG_PROFILE profile = std::make_pair(segment->getMaxExtentSize(), _units.size());
            std::pair<_FREE_MAP::iterator, BOOLEAN> res = _freeMap.insert(profile);
            SDB_ASSERT(res.second, "impossible");
            segment->pos = res.first;
         }
         _units.push_back(segment);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 variableExtentAllocator::deposit()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         ossSLatchGuard guard(&_latch, EXCLUSIVE);
         PAGE_ID pid = _units.size() * _segmentPageCnt;
         _segmentUnit *segment = SDB_OSS_NEW _segmentUnit(pid, _segmentPageCnt,
                                                          nullptr, FALSE);
         if (OSS_UNLIKELY(nullptr == segment))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         {
            _SEG_PROFILE profile = std::make_pair(segment->getMaxExtentSize(), _units.size());
            std::pair<_FREE_MAP::iterator, BOOLEAN> res = _freeMap.insert(profile);
            SDB_ASSERT(res.second, "impossible");
            segment->pos = res.first;
         }
         _units.push_back(segment);
      }
   done:
      return rc;
   error:
      goto done;
   }

   PAGE_ID variableExtentAllocator::reserve(UINT32 pcnt, UINT32 *currentSegCount)
   {
      PAGE_ID pid = INVALID_PAGE_ID;
      SDB_ASSERT(isValid(), "can not be invalid");
      constexpr UINT32 _MAX_LOOP = 32768;
      UINT32 i = 0;
      ossSLatchGuard guard(&_latch, SHARED, FALSE);

      if (OSS_UNLIKELY(0 == pcnt || _segmentPageCnt < pcnt ||
                       _maxExtentSize < pcnt))
      {
         SDB_ASSERT(FALSE, "invalid pcnt");
         goto done;
      }

      guard.lock();

      do
      {
         UINT32 segmentId = 0;
         _segmentUnit *segment = _findFromMap(pcnt, segmentId);
         if (nullptr == segment)
         {
            break;
         }
         else
         {
            std::unique_lock<std::mutex> segmentLock(segment->getMutex());
            if (!segment->isRegistered())
            {
               continue;
            }

            pid = segment->reserve(pcnt);
            if (INVALID_PAGE_ID != pid)
            {
               break;
            }
            else if (0 == segment->getMaxExtentSize())
            {
               _eraseFromMap(segment);
               continue;
            }
            else if (segment->getMaxExtentSize() < segment->pos->first)
            {
               _reinsertIntoMap(segment);
               continue;
            }
            else
            {
               /// do no thing. it may be updated by pre-allocator.
               continue;
            }
         }
         
      } while (++i < _MAX_LOOP);

      if (nullptr != currentSegCount)
      {
         *currentSegCount = _units.size();
      }
      
   done:
      return pid;
   }

   void variableExtentAllocator::release(PAGE_ID pid, UINT32 pcnt)
   {
      UINT32 count = 0;
      ossSLatchGuard guard(&_latch, SHARED, FALSE);
      
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == pcnt))
      {
         SDB_ASSERT(FALSE, "invalid extent");
         goto done;
      }

      guard.lock();
      
      if (_units.size() <= ((pid + pcnt - 1) / _segmentPageCnt))
      {
         PD_LOG(PDSEVERE, "pids to be released[%d,%d] out of max segment count:%d",
                pid, pcnt, _units.size());
         SDB_ASSERT(FALSE, "out of bound");
         goto done;
      }

      do
      {
         UINT32 size = _segmentPageCnt - ((pid + count) % _segmentPageCnt);
         size = std::min(size, pcnt - count);
         UINT32 segmentId = (pid + count) / _segmentPageCnt;
         _segmentUnit *segment = _units.at(segmentId);

         std::unique_lock<std::mutex> segmentLock(segment->getMutex());
         segment->release(pid + count, size);
         if (!segment->isRegistered())
         {
            if (MIN_FREE_COUNT_TO_REGISTER <= segment->getFreeCount())
            {
               _insertIntoMap(segment, segmentId);
            }
         }
         else if (segment->pos->first < segment->getMaxExtentSize() &&
                  segment->pos->first < _maxExtentSize)
         {
            _reinsertIntoMap(segment);
         }

         segmentLock.unlock();
         count += size;
          
      } while (count < pcnt);
   
   done:
      return;
   }

   void variableExtentAllocator::_eraseFromMap(_segmentUnit *segment)
   {
      SDB_ASSERT(nullptr != segment, "can not be null");
      
      if (_FREE_MAP::iterator() != segment->pos)
      {
         ossSLatchGuard guard(&_mapLatch, EXCLUSIVE);
         _freeMap.erase(segment->pos);
         segment->pos = _FREE_MAP::iterator();
      }
   }

   void variableExtentAllocator::_insertIntoMap(_segmentUnit *segment, UINT32 segmentId)
   {
      SDB_ASSERT(nullptr != segment, "can not be null");
      SDB_ASSERT(0 < segment->getMaxExtentSize(), "can not be zero");
      SDB_ASSERT(!segment->isRegistered(), "already in map");
      ossSLatchGuard guard(&_mapLatch, EXCLUSIVE);
      _SEG_PROFILE profile = std::make_pair(segment->getMaxExtentSize(), segmentId);
      std::pair<_FREE_MAP::iterator, BOOLEAN> res = _freeMap.insert(profile);
      SDB_ASSERT(res.second, "impossible");
      segment->pos = res.first;
   }

   variableExtentAllocator::_segmentUnit *
   variableExtentAllocator::_findFromMap(UINT32 pcnt, UINT32 &segmentId)
   {
      SDB_ASSERT(0 < pcnt, "can not e zero");
      _segmentUnit *segment = nullptr;
      _SEG_PROFILE profile = std::make_pair(pcnt, 0);

      ossSLatchGuard guard(&_mapLatch, SHARED);
      _FREE_MAP::iterator itr = _freeMap.lower_bound(profile);
      if (_freeMap.end() != itr)
      {
         SDB_ASSERT(itr->second < _units.size(), "impossible");
         segment = _units.at(itr->second);
         segmentId = itr->second;
      }
      return segment;
   }

   void variableExtentAllocator::_reinsertIntoMap(_segmentUnit *segment)
   {
      SDB_ASSERT(nullptr != segment, "can not be null");
      SDB_ASSERT(0 < segment->getMaxExtentSize(), "can not be zero");
      SDB_ASSERT(segment->isRegistered(), "already in map");
      ossSLatchGuard guard(&_mapLatch, EXCLUSIVE);
      UINT32 segmentId = segment->pos->second;
      _freeMap.erase(segment->pos);
      _SEG_PROFILE profile = std::make_pair(segment->getMaxExtentSize(), segmentId);
      std::pair<_FREE_MAP::iterator, BOOLEAN> res = _freeMap.insert(profile);
      SDB_ASSERT(res.second, "impossible");
      segment->pos = res.first;
   }
} // namespace vessel

} // namespace engine
