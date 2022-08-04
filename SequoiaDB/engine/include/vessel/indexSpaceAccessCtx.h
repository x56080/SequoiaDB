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

   Source File Name = indexSpaceAccessCtx.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SPACE_ACCESS_CTX_H_
#define VESSEL_INDEX_SPACE_ACCESS_CTX_H_

#include "ossRWMutex.hpp"
#include "ossMemPool.hpp"
#include "vessel/pageIdentifier.h"
#include "vessel/sparsePidBitmap.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexSpace;

   class indexSpaceAccessCtx : public SDBObject
   {
      friend class indexSpace;
      public:
         indexSpaceAccessCtx() = default;
         ~indexSpaceAccessCtx();
         indexSpaceAccessCtx(const indexSpaceAccessCtx &) = delete;
         indexSpaceAccessCtx &operator=(const indexSpaceAccessCtx &) = delete;
         indexSpaceAccessCtx(indexSpaceAccessCtx &&);
         indexSpaceAccessCtx &operator=(indexSpaceAccessCtx &&);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _mutex &&
                   nullptr != _rctx &&
                   nullptr != _is;
         }

         OSS_INLINE requestContext *getReqCtx() {return _rctx;}
         OSS_INLINE indexSpace *getIndexSpace() {return _is;}
         OSS_INLINE UINT64 getPSN()const {return _psn;}

         void reset();

         void abort();

         BOOLEAN isPrivate(PAGE_ID lpid);
         BOOLEAN isReserved(PAGE_ID lpid);
         BOOLEAN isRemapped(PAGE_ID lpid);

      private:
         // explicit indexSpaceAccessCtx(ossRWMutex *mutex,
         //                              requestContext *rctx,
         //                              indexSpace *is);
         void init(ossRWMutex *mutex,
                   requestContext *rctx,
                   indexSpace *is);
      private:
         ossRWMutex *_mutex = nullptr;
         requestContext *_rctx = nullptr;
         indexSpace *_is = nullptr;
         UINT64 _psn = 0;
         sparsePidBitmap _reserved; /// brandnew lpids
         sparsePidBitmap _remapped;///lpids which remapped or removed
   };//class indexSpaceAccessCtx
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SPACE_ACCESS_CTX_H_