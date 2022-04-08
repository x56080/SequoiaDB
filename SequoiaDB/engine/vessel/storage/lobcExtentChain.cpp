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
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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
      else if (OSS_UNLIKELY(!isValidAccessing(offset, size)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < _chain.size(); ++i)
      {
         const lextentDescriptor &desc = _chain.at(i);
         if ((desc.size + seek) <= offset)
         {
            seek += desc.size;
         }
         else
         {
            UINT32 offsetInExtent = offset - seek;
            UINT32 pos = 0;
            UINT32 seekInExtent = 0;
            UINT32 sizeInExtent = desc.size;

            while ((seekInExtent + _pageSize) <= offsetInExtent)
            {
               ++pos;
               seekInExtent += _pageSize;
               SDB_ASSERT(_pageSize < sizeInExtent, "impossible");
               sizeInExtent -= _pageSize;
            }

            sizeInExtent = std::min(sizeInExtent, size);
            roadmap._pageSize = _pageSize;
            roadmap._pid = desc.pid + pos;
            roadmap._boffset = offsetInExtent - seekInExtent;
            roadmap._eoffset = roadmap._boffset + sizeInExtent;
            roadmap._pcnt = ossAlignX(roadmap._eoffset, _pageSize) / _pageSize;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
