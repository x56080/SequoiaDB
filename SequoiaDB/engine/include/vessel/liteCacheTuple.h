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

   Source File Name = liteCacheTuple.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_CACHE_TUPLE_H_
#define VESSEL_LITE_CACHE_TUPLE_H_

#include "vessel/lcPageTagHolder.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   class liteCache;
   class requestContext;

   class liteCacheTuple
   {
      friend class liteCache;
      public:
         OSS_INLINE liteCacheTuple():
         _pool(NULL),
         _writingPrepared(FALSE)
         {}
         OSS_INLINE ~liteCacheTuple()
         {
            release();
         }

         liteCacheTuple &operator=(const liteCacheTuple &) = delete;
         liteCacheTuple(const liteCacheTuple &r) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _pool && _holder.valid();
         }

         void release();

         void commit(UINT64 lsn);

         ossValuePtr getReadableBuffer()const;

         ossValuePtr getWritableBuffer()const;

         INT32 prepareToWrite(requestContext *context);

         void swap(liteCacheTuple &o);

      private:
         lcPageTagHolder _holder;
         liteCache *_pool;
         BOOLEAN _writingPrepared;
   };//class liteCacheTuple

} /// end of namespace vessel
} /// end of namespace engine

#endif
