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

#include "dpsDef.hpp"
#include "ossLatch.hpp"
#include "vessel/lcPageTagHolder.h"

namespace engine
{
namespace vessel
{
   class liteCache;
   class requestContext;

   ///WARNING: Should not share tulpe in multiple threads.
   class liteCacheTuple : public SDBObject
   {
      friend class liteCache;
      public:
         OSS_INLINE liteCacheTuple()
         {}

         OSS_INLINE ~liteCacheTuple()
         {
            release();
         }

         liteCacheTuple(liteCacheTuple &&o):
         _pool(NULL),
         _flags(0)
         {
            if (o.isValid())
            {
               _pool = o._pool;
               _holder = o._holder;
               _flags = o._flags;
               o._pool = NULL;
               o._holder.reset(NULL);
               o._flags = 0;
            }
         }

         liteCacheTuple &operator=(liteCacheTuple &&o)
         {
            release();
            if (o.isValid())
            {
               _pool = o._pool;
               _holder = o._holder;
               _flags = o._flags;
               o._pool = NULL;
               o._holder.reset(NULL);
               o._flags = 0;
            }
            return *this;
         }

         liteCacheTuple &operator=(const liteCacheTuple &) = delete;
         liteCacheTuple(const liteCacheTuple &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _pool;
         }

         void release();

         void commit(UINT64 lsn);

         ossValuePtr getReadableBuffer()const;

         ossValuePtr getWritableBuffer()const;

         INT32 prepareToWrite(requestContext *context);
      
      private:/// for liteCache
         INT32 init(liteCachePageTag *tag,
                    const ossSharedLatchMode &mode,
                    liteCache *pool,
                    BOOLEAN isWritingPrepared);

         BOOLEAN isWritingPrepared()const;

      private:
         liteCache *_pool = NULL;
         lcPageTagHolder _holder;
         UINT32 _flags = 0;
   };//class liteCacheTuple

} /// end of namespace vessel
} /// end of namespace engine

#endif
