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

   Source File Name = lobcExtentChain.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lobcExtentChain.h"
#include "vessel/lobExtentMetaBlock.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "dmsLobDef.hpp"

namespace engine
{
namespace vessel
{
   void lobcExtentChain::init(UINT32 pageSize)
   {
      SDB_ASSERT(isValidPageSize(pageSize), "can not be invalid");
      reset();
      _pageSize = pageSize;
   }

   BOOLEAN lobcExtentChain::isValidAccessing(UINT32 offset, UINT32 size)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return isValid() && ((offset + size) <= _size);
   }

   INT32 lobcExtentChain::pushBack(const lextentDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!desc.isValid() ||
                            0 == desc.size ||
                            desc.getCapacity(_pageSize) < desc.size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_chain.empty() &&
               _chain.back().size != _chain.back().getCapacity(_pageSize))
      {
         PD_LOG(PDERROR, "the size and capacity of pre extent should be same");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (MAX_LOB_CHUNK_SIZE < (_size + desc.size))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      _size += desc.size;
      _chain.push_back(desc);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcExtentChain::createExtentRoadmap(UINT32 offset,
                                              UINT32 size,
                                              extentRoadmap &roadmap)const
   {
      INT32 rc = SDB_OK;
      UINT32 seek = 0;
      roadmap.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidAccessing(offset, size) ||
                            0 == size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _chain.size(); ++i)
      {
         const lextentDescriptor &desc = _chain.at(i);

         /// ignore extents not to read
         if ((desc.size + seek) <= offset)
         {
            seek += desc.size;
         }
         else
         {
            UINT32 offsetInExtent = offset - seek;
            UINT32 readingSize = (desc.size - offsetInExtent) < size ?
                                 (desc.size - offsetInExtent) : size;
            UINT32 pageCountToSkip = offsetInExtent / _pageSize;

            roadmap._pageSize = _pageSize;
            roadmap._pid = desc.pid + pageCountToSkip;
            roadmap._boffset = offsetInExtent - (pageCountToSkip * _pageSize);
            roadmap._eoffset = roadmap._boffset + readingSize;
            roadmap._pcnt = ossAlignX(roadmap._eoffset, _pageSize) / _pageSize;

            break;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lobcExtentChain::createExtentRoadmaps(UINT32 offset, UINT32 size,
                                               ossPoolList<extentRoadmap> &roadmaps)const
   {
      INT32 rc = SDB_OK;
      UINT32 seek = 0;
      UINT32 createdSize = 0;
      UINT32 chainPos = 0;
      UINT32 offsetInExtent = 0;
      roadmaps.clear();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isValidAccessing(offset, size) ||
                            0 == size))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (; chainPos < _chain.size(); ++chainPos)
      {
         const lextentDescriptor &desc = _chain.at(chainPos);
         /// ignore extents not to read
         if ((desc.size + seek) <= offset)
         {
            seek += desc.size;
         }
         else
         {
            offsetInExtent = offset - seek;
            break;
         }
      }

      while (createdSize < size)
      {
         SDB_ASSERT(chainPos < _chain.size(), "out of bound");
         const lextentDescriptor &desc = _chain.at(chainPos);
   
         UINT32 readingSize = (desc.size - offsetInExtent) < size ?
                              (desc.size - offsetInExtent) : size;
         UINT32 pageCountToSkip = offsetInExtent / _pageSize;

         extentRoadmap roadmap;
         roadmap._pageSize = _pageSize;
         roadmap._pid = desc.pid + pageCountToSkip;
         roadmap._boffset = offsetInExtent - (pageCountToSkip * _pageSize);
         roadmap._eoffset = roadmap._boffset + readingSize;
         roadmap._pcnt = ossAlignX(roadmap._eoffset, _pageSize) / _pageSize;

         roadmaps.push_back(roadmap);

         seek += readingSize;
         createdSize += readingSize;
         ++chainPos;
         offsetInExtent = 0;
      }
         
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 lobcExtentChain::getCurrentCapacity()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      UINT32 totalPcnt = 0;
      for (UINT32 i = 0; i < _chain.size(); ++i)
      {
         totalPcnt += _chain.at(i).pcnt;
      }

      return _pageSize * totalPcnt;
   }

   UINT32 lobcExtentChain::extendLastExtent(UINT32 deltaSize)
   {
      SDB_ASSERT(!isEmpty(), "can not be invalid");
      UINT32 extendedSize = 0;
      lextentDescriptor &desc = _chain.back();
      UINT32 freeSize = desc.getCapacity(_pageSize) - desc.size;
      if (freeSize <= deltaSize)
      {
         _size += freeSize;
         extendedSize = freeSize;
      }
      else
      {
         _size += deltaSize;
         extendedSize = deltaSize;
      }

      desc.size += extendedSize;
      return extendedSize;
   }

   UINT32 lobcExtentChain::getFreeSizeInLastExtent()const
   {
      UINT32 freeSize = 0;
      if (!isEmpty())
      {
         const lextentDescriptor &desc = _chain.back();
         freeSize = desc.getCapacity(_pageSize) - desc.size;
      }
      return freeSize;
   }
} // namespace vessel

} // namespace engine
