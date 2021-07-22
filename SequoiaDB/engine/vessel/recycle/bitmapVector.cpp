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

   Source File Name = bitmapVector.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bitmapVector.h"
#include "ossLikely.hpp"
#include "vessel/bitMapUtils.h"

namespace engine
{
namespace vessel
{
   bitmapVector::bitmapVector()
   {}

   bitmapVector::~bitmapVector()
   {
      fini();
   }

   INT32 bitmapVector::init(UINT32 bitsCapacity)
   {
      INT32 rc = SDB_OK;
      fini();

      if (0 == bitsCapacity ||
          !ossIsPowerOf2(bitsCapacity))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void bitmapVector::fini()
   {
      _bitsCapacity = 0;
      for (_BITMAP_VEC::const_iterator itr = _bitmapVec.begin();
           itr != _bitmapVec.end(); ++itr)
      {
         SDB_THREAD_FREE(*itr);
      }
      _bitmapVec.clear();
      return;
   }

   INT32 bitmapVector::setNoFree(UINT32 offset, BOOLEAN &alreadyNoFree)
   {
      INT32 rc = SDB_OK;
      UINT32 count = 0;
      UINT64 *buf = NULL;

      if (OSS_UNLIKELY(0 == _bitsCapacity))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      count = offset / _bitsCapacity + 1;
      if (_bitmapVec.size() < count)
      {
         rc = extendBitmapVec(count);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend bitmap vec:%d", rc);
            goto error;
         }
      }

      buf = _bitmapVec.at(count - 1);
      alreadyNoFree = !(setNotFreeIfFree64(_bitsCapacity >> 6,
                                           buf, (offset & (_bitsCapacity - 1))));
   done:
      return rc;
   error:
      goto done;
   }

   INT32 bitmapVector::extendBitmapVec(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 < _bitsCapacity, "can not be invalid");
      UINT32 memSize = _bitsCapacity >> 3;
      UINT32 bitsCount = _bitsCapacity >> 6;

      for (UINT32 i = _bitmapVec.size(); i < size; ++i)
      {
         UINT64 *buf = (UINT64 *)SDB_THREAD_ALLOC(memSize);
         if (OSS_UNLIKELY(NULL == buf))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         resetBitMap64(bitsCount, buf, TRUE);
         _bitmapVec.push_back(buf);
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine