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

   Source File Name = indexSpaceAccessCtx.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpaceAccessCtx.h"
#include "pdTrace.hpp"
#include "vessel/indexSpace.h"

namespace engine
{
namespace vessel
{
   indexSpaceAccessCtx::~indexSpaceAccessCtx()
   {
      if (nullptr != _mutex)
      {
         _mutex->release_r();
      }
   }


   // indexSpaceAccessCtx::indexSpaceAccessCtx(ossRWMutex *mutex,
   //                                          requestContext *rctx,
   //                                          indexSpace *is):
   // _mutex(mutex),
   // _rctx(rctx),
   // _is(is)
   // {
   //    SDB_ASSERT(nullptr != mutex, "can not be invalid");
   //    SDB_ASSERT(nullptr != rctx, "can not be invalid");
   //    SDB_ASSERT(nullptr != is, "can not be invalid");
   //    _mutex->lock_r();
   // }

   indexSpaceAccessCtx::indexSpaceAccessCtx(indexSpaceAccessCtx &&o):
   _mutex(o._mutex),
   _rctx(o._rctx),
   _is(o._is),
   _reserved(std::move(o._reserved)),
   _remapped(std::move(o._remapped))
   {
      o._mutex = nullptr;
      o._rctx = nullptr;
      o._is = nullptr;
   }

   indexSpaceAccessCtx &indexSpaceAccessCtx::operator=(indexSpaceAccessCtx &&o)
   {
      reset();
      _mutex = o._mutex;
      _rctx = o._rctx;
      _is = o._is;
      _reserved = std::move(o._reserved);
      _remapped = std::move(o._remapped);
      o._mutex = nullptr;
      o._rctx = nullptr;
      o._is = nullptr;
      return *this;
   }
   

   void indexSpaceAccessCtx::init(ossRWMutex *mutex,
                                  requestContext *rctx,
                                  indexSpace *is)
   {
      reset();
      SDB_ASSERT(nullptr != mutex, "can not be invalid");
      SDB_ASSERT(nullptr != rctx, "can not be invalid");
      SDB_ASSERT(nullptr != is, "can not be invalid");
      _mutex = mutex;
      _rctx = rctx;
      _is = is;
      _mutex->lock_r();
   }

   void indexSpaceAccessCtx::reset()
   {
      if (nullptr != _mutex)
      {
         _mutex->release_r();
         _mutex = nullptr;
      }
      _rctx = nullptr;
      _is = nullptr;
      _reserved.reset();
      _remapped.reset();
      return;
   }

   void indexSpaceAccessCtx::abort()
   {
      if (isValid())
      {
         _is->abort(*this);
         /// all members reset
      }
   }

   BOOLEAN indexSpaceAccessCtx::isPrivate(PAGE_ID lpid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      return _reserved.test(lpid) || _remapped.test(lpid);
   }

   BOOLEAN indexSpaceAccessCtx::isReserved(PAGE_ID lpid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      return _reserved.test(lpid);
   }

   BOOLEAN indexSpaceAccessCtx::isRemapped(PAGE_ID lpid)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      return _remapped.test(lpid);
   }
} // namespace vessel

} // namespace engine
