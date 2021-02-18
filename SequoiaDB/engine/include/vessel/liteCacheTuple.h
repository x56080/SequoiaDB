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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

#include "vessel/lcExtentTagHolder.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   class liteCache;
   class requestContext;
   class logRecordContext;

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
            
         }

      private:
         OSS_INLINE liteCacheTuple(const liteCacheTuple &r):
         _holder(r._holder),
         _pool(r._pool),
         _writingPrepared(r._writingPrepared)
         { 

         }
         OSS_INLINE liteCacheTuple &operator=(const liteCacheTuple &r)
         {
            _holder = r._holder;
            _pool = r._pool;
            _writingPrepared = r._writingPrepared;
            return *this;
         }
         
      public:
         OSS_INLINE BOOLEAN valid()const
         {
            return NULL != _pool && _holder.valid();
         }

         INT32 prepareToWrite(requestContext *context);

         INT32 getReadPtr(UINT32 offset, UINT32 len, const CHAR **ptr)const;

         INT32 read(UINT32 offset, UINT32 len, CHAR *buf) const;

         INT32 getWritePtr(UINT32 offset, UINT32 len, CHAR **ptr);

         /// len can not be 0
         INT32 write(UINT32 offset,
                     UINT32 len,
                     const CHAR *data);

         INT32 getMinLSN(DPS_LSN_OFFSET &lsn);

      private:
         INT32 validateRW(UINT32 offset, UINT32 len, const CHAR *buf, BOOLEAN readonly)const;

         INT32 validateRWPtr(UINT32 offset, UINT32 len, BOOLEAN readonly)const;

         void writePages(UINT32 pageSize, lcChunkPage *pages,
                         UINT32 offset, UINT32 len, const CHAR *data);

         void readPages(UINT32 pageSize, const lcChunkPage *pages,
                        UINT32 offset, UINT32 len, CHAR *buf)const;

      private:
         lcExtentTagHolder _holder;
         liteCache *_pool;
         BOOLEAN _writingPrepared;
   };

} /// end of namespace vessel
} /// end of namespace engine

#endif
