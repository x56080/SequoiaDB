/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = runtimePageBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RUNTIME_PAGE_BUFFER_H_
#define VESSEL_RUNTIME_PAGE_BUFFER_H_

#include "vessel/mmapPagePointer.h"
#include "vessel/globalPageID.h"
#include "vessel/slice.h"
#include "vessel/strictBuffer.h"
#include "vessel/liteIOBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;

   class runtimePageBuffer : public SDBObject
   {
      friend class logicalPageSpace;
      public:
         runtimePageBuffer() = default;
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
            return _iob.isValid();
         }

      private:/// for logicalPageSpace
         void initWithMmap(const GLOBAL_PAGE_ID &gpid,
                           UINT32 pageSize,
                           const mmapPagePointer &ptr);

         /// iob will be reset
         void initWithBuffer(const GLOBAL_PAGE_ID &gpid,
                             UINT32 pageSize,
                             liteIOBuffer &iob);

      public:
         void commit(DPS_LSN_OFFSET lsn);

         void fini();

         BOOLEAN isCommitted()const;
         BOOLEAN isWritingPrepared()const;;

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
         liteIOBuffer _iob;
         ossValuePtr _buffer = 0;
         DPS_LSN_OFFSET _committedLsn = DPS_INVALID_LSN_OFFSET;
   };//class runtimePageBuffer
}//namespace vessel
}//namespace engine

#endif//VESSEL_RUNTIME_PAGE_BUFFER_H_