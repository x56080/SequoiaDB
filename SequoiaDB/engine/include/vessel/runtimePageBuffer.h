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

      public:
         struct options
         {
            OSS_INLINE options(){}
            OSS_INLINE ~options(){}

            UINT32 toFlags()const;
         };//struct options

      private:/// for logicalPageSpace
         /// init with mmap
         INT32 init(const GLOBAL_PAGE_ID &gpid,
                    UINT32 pageSize,
                    const mmapPagePointer &ptr,
                    const options &o = options());

         /// init with cache tuple
         INT32 init(const GLOBAL_PAGE_ID &gpid,
                    UINT32 pageSize,
                    liteCacheTuple &tuple,
                    const options &o = options());

      public:
         void commit(DPS_LSN_OFFSET lsn);

         void abort();

         void fini();

         BOOLEAN isCommitted()const;
         BOOLEAN isAborted()const;
         BOOLEAN isWritingFinished()const;
         BOOLEAN isWritingPrepared()const;
         BOOLEAN hasRuntimeFlags()const;

      public:
         INT32 prepareToWrite(requestContext *context);

      public:
         const void * getReadOnlyBuffer()const
         {
            return (const void *)_buffer;
         }
         void *getBuffer()const
         {
            return isWritingPrepared() ? (void *)_buffer : NULL;
         }
         const pageHead *getPageHead()const
         {
            return (const pageHead *)_buffer;
         }
   
         INT32 getReadablePtrOfBodyWithRc(UINT32 offset, UINT32 size, ossValuePtr &ptr)const;
         const void *getReadablePtrOfBody(UINT32 offset, UINT32 size)const;

         INT32 getWritablePtrOfBodyWithRc(UINT32 offset, UINT32 size, ossValuePtr &ptr)const;
         void *getWritablePtrOfBody(UINT32 offset, UINT32 size)const;

         template <typename T>
         const T *getReadablePtrOfBody(UINT32 offset)const
         {
            INT32 rc = SDB_OK;
            const T *ptr = NULL;
            rc = getReadablePtrOfBody(offset, &ptr);
            if (SDB_OK != rc)
            {
               ptr = NULL;
               goto error;
            }
         done:
            return ptr;
         error:
            goto done;
         }

         template <typename T>
         INT32 getReadablePtrOfBody(UINT32 offset, const T **ptr)const
         {
            INT32 rc = SDB_OK;
            ossValuePtr tmp = 0;
            if (OSS_UNLIKELY(NULL == ptr))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = getReadablePtrOfBodyWithRc(offset, sizeof(T), &tmp);
            if (SDB_OK != rc)
            {
               goto error;
            }
            *ptr = (const T *)tmp;
         done:
            return rc;
         error:
            goto done;
         }

         template <typename T>
         T *getWritablePtrOfBody(UINT32 offset)const
         {
            INT32 rc = SDB_OK;
            T *ptr = NULL;
            rc = getWritablePtrOfBody(offset, &ptr);
            if (SDB_OK != rc)
            {
               ptr = NULL;
               goto error;
            }
         done:
            return ptr;
         error:
            goto done;
         }

         template <typename T>
         INT32 getWritablePtrOfBody(UINT32 offset, T **ptr)const
         {
            INT32 rc = SDB_OK;
            ossValuePtr tmp = 0;
            if (OSS_UNLIKELY(NULL == ptr))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = getWritablePtrOfBodyWithRc(offset, sizeof(T), &tmp);
            if (SDB_OK != rc)
            {
               goto error;
            }

            *ptr = (T *)tmp;
         done:
            return rc;
         error:
            goto done;
         }

      private:
         BOOLEAN isValidPtrOfPageBody(UINT32 offset, UINT32 size)const;
      
      private:
         void setCommitted();
         void setWritingPrepared();
         void setAborted();
         
      private:
         GLOBAL_PAGE_ID _gpid;
         UINT32 _pageSize = 0;
         UINT32 _flags = 0;
         UINT32 _runtimeFlags = 0;
         liteCacheTuple _tuple;
         ossValuePtr _buffer = 0;
   };//class runtimePageBuffer
}//namespace vessel
}//namespace engine

#endif//VESSEL_RUNTIME_PAGE_BUFFER_H_