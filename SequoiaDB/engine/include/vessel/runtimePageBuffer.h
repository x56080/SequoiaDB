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

   Source File Name = runtimePageBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RUNTIME_PAGE_BUFFER_H_
#define VESSEL_RUNTIME_PAGE_BUFFER_H_

#include "vessel/liteCacheTuple.h"
#include "vessel/mmapPagePointer.h"
#include "vessel/globalPageID.h"
#include "vessel/slice.h"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;

   class runtimePageBuffer : public SDBObject
   {
      friend class logicalPageSpace;
      public:
         runtimePageBuffer();
         ~runtimePageBuffer();
         runtimePageBuffer(runtimePageBuffer &&);
         runtimePageBuffer &operator=(runtimePageBuffer &&);
         runtimePageBuffer(const runtimePageBuffer &) = delete;
         runtimePageBuffer &operator=(const runtimePageBuffer &) = delete;

      public:
         OSS_INLINE const GLOBAL_PAGE_ID &getGlobalPid()const
         {
            return _gpid;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _pageSize;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return _gpid.isValid();
         }
         ///WARNING: Should always check isValid first.
         OSS_INLINE BOOLEAN isCacheBuffer()const
         {
            return _tuple.isValid();
         }

      private:/// for logicalPageSpace
         /// init with mmap
         INT32 init(const GLOBAL_PAGE_ID &gpid,
                    UINT32 pageSize,
                    const mmapPagePointer &ptr);

         /// init with cache tuple
         INT32 init(const GLOBAL_PAGE_ID &gpid,
                    UINT32 pageSize,
                    liteCacheTuple &tuple);

      public:
         void commit(DPS_LSN_OFFSET lsn);

         void fini();

         BOOLEAN isCommitted()const;
         BOOLEAN isWritingPrepared()const;

      public:
         INT32 prepareToWrite(requestContext *context);

      public:
         const pageHead *getPageHead()const;
         strictBuffer getReadableBuffer()const;
         strictBuffer getWritableBuffer();
         strictBuffer getReadableBodyBuffer()const;
         strictBuffer getWritableBodyBuffer();
         slice getSlice()const
         {
            return slice(_pageSize, (const void *)_buffer);
         }
      private:
         void setWritingPrepared();
         
      private:
         GLOBAL_PAGE_ID _gpid;
         UINT32 _pageSize = 0;
         UINT32 _flags = 0;
         liteCacheTuple _tuple;
         ossValuePtr _buffer = 0;
         DPS_LSN_OFFSET _commitedLsn = DPS_INVALID_LSN_OFFSET;
   };//class runtimePageBuffer
}//namespace vessel
}//namespace engine

#endif//VESSEL_RUNTIME_PAGE_BUFFER_H_