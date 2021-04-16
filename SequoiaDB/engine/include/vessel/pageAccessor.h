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

   Source File Name = pageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_PAGE_ACCESSOR_H_
#define VESSEL_PAGE_ACCESSOR_H_

#include "vessel/globalPageID.h"
#include "vessel/pageDef.h"
#include "vessel/liteCacheTuple.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   const UINT32 PAGE_ACCESSOR_FLAG_NONE = 0;
   const UINT32 PAGE_ACCESSOR_FLAG_DIRECT = 0x01;  /// mmap accessing
   const UINT32 PAGE_ACCESSOR_FLAG_NON_READONLY = 0x02;
   const UINT32 PAGE_ACCESSOR_FLAG_INIT_PAGE = 0x04;
   const UINT32 PAGE_ACCESSOR_FLAG_OPLIST_HEAD = 0x08;
   const UINT32 PAGE_ACCESSOR_FLAG_OPLIST_TAIL = 0x10;

   class storageUnit;
   class logRecordContext;

   class pageAccessor : public SDBObject
   {
      public:
         OSS_INLINE pageAccessor():
         _size(0),
         _flags(0),
         _status(0),
         _ptr(0),
         _su(NULL),
         _fullDumpBuf(NULL),
         _fullDumpSize(0),
         _oplist(DPS_INVALID_LSN_OFFSET)
         {

         }
         virtual ~pageAccessor();

      public:
         pageAccessor(const pageAccessor &) = delete;
         pageAccessor &operator=(const pageAccessor &) = delete;

      public:
         OSS_INLINE const GLOBAL_PAGE_ID &getGPID()const
         {
            return _gpid;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _size;
         }
         OSS_INLINE UINT32 getPageBodySize()const
         {
            if (OSS_LIKELY((PAGE_HEAD_LEN + PAGE_TAIL_LEN) <= _size))
            {
               return _size - PAGE_TAIL_LEN - PAGE_HEAD_LEN;
            }
            else
            {
               return 0;
            }
         }

         OSS_INLINE UINT32 getFlags()const
         {
            return _flags;
         }
         OSS_INLINE DPS_LSN_OFFSET getOplist() const
         {
            return _oplist;
         }

      public:
         INT32 init(requestContext *context,
                     FILE_TYPE type,
                     PAGE_ID pid,
                     UINT32 flags = 0,
                     storageUnit *su = NULL,
                     DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET);

         INT32 initWithDirectMode(requestContext *context,
                                 FILE_TYPE type,
                                 PAGE_ID pid,
                                 UINT32 pageSize,
                                 ossValuePtr ptr,
                                 BOOLEAN pageTypeCheck = TRUE,
                                 BOOLEAN readOnly = TRUE);

         void abortToWrite();
         INT32 prepareToWrite(requestContext *context);

         void commit(requestContext *context, DPS_LSN_OFFSET lsn);

         void fini(requestContext *context);

         virtual PAGE_TYPE getPageType()const = 0;

      protected:
         /// WARNING: will overwrite a initialized page head.
         INT32 initCommonPageHeadAndTail(PAGE_ID lpid=INVALID_PAGE_ID);

         INT32 memsetPageBody(CHAR v);
         
         INT32 getReadPtrOfHead(const pageHead **head);
         INT32 getWritePtrOfHead(pageHead **head);
         INT32 getWritePtrOfPageBody(UINT32 offset, UINT32 len, CHAR **ptr);
         INT32 getReadPtrOfPageBody(UINT32 offset, UINT32 len, const CHAR **ptr);

         INT32 readPageBody(UINT32 offset, UINT32 len, CHAR *buf);
         INT32 writePageBody(UINT32 offset, UINT32 len, const CHAR *buf);
         INT32 getPidFromDisk(PAGE_ID &pid);

      protected:
         template <typename T>
         INT32 getReadPtrOfPageBody(UINT32 offset, const T **ptr)
         {
            const CHAR *tmp = NULL;
            INT32 rc = getReadPtrOfPageBody(offset, sizeof(T), &tmp);
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
         INT32 getWritePtrOfPageBody(UINT32 offset, T **ptr)
         {
            CHAR *tmp = NULL;
            INT32 rc = getWritePtrOfPageBody(offset, sizeof(T), &tmp);
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

         template <typename T>
         INT32 getReadableUserHeadPtr(const T **head)
         {
            INT32 rc = SDB_OK;
            const CHAR *tmpHead = NULL;
            SDB_ASSERT(0 == (sizeof(T) & 0x03), "must be 4bytes aligned");

            if (OSS_UNLIKELY(NULL == head))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            rc = getReadPtrOfPageBody(0, sizeof(T), &tmpHead);
            if (SDB_OK != rc)
            {
               goto error;
            }

            *head = (const T *)tmpHead;
         done:
            return rc;
         error:
            goto done;
         }

         template <typename T>
         INT32 getWritableUserHeadPtr(T **head)
         {
            INT32 rc = SDB_OK;
            CHAR *tmpHead = NULL;
            SDB_ASSERT(0 == (sizeof(T) & 0x03), "must be 4bytes aligned");

            if (OSS_UNLIKELY(NULL == head))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            else if (!fullAccessing())
            {
               rc  = SDB_INVALIDARG;
               goto error;
            }

            rc = getWritePtrOfPageBody(0, sizeof(T), &tmpHead);
            if (SDB_OK != rc)
            {
               goto error;
            }

            *head = (T *)tmpHead;
         done:
            return rc;
         error:
            goto done;
         }

      protected:
         INT32 prepareLogDone(requestContext *context,
                              logRecordContext *lrc);

      protected:
         OSS_INLINE const CHAR *getFullDumpBuffer()const
         {
            return _fullDumpBuf;
         }

         OSS_INLINE UINT32 getFullDumpSize()const
         {
            return _fullDumpSize;
         }
         OSS_INLINE BOOLEAN isInOplist()const
         {
            return DPS_INVALID_LSN_OFFSET != _oplist;
         }
         OSS_INLINE BOOLEAN isOplistHead()const
         {
            return OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_OPLIST_HEAD);
         }
         OSS_INLINE BOOLEAN isOplistTail()const
         {
            return OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_OPLIST_TAIL);
         }
         BOOLEAN fullAccessing()const;
      private:
         INT32 validateMMapPageHeadAndTail();
         BOOLEAN accessing()const;

         INT32 beginToAccess(requestContext *context, UINT32 flags);
         INT32 beginToAccessByMMap();
         INT32 beginToAccessByCache(requestContext *context);

         INT32 prepareToWriteByCache(requestContext *context);

         void endToAccess(requestContext *context);
         void endToAccessByMMap();
         void endToAccessByCache(requestContext *context);
        
         INT32 getMMapWritePtrOfPage(UINT32 offset, UINT32 len, CHAR **ptr);
         INT32 getMMapReadPtrOfPage(UINT32 offset, UINT32 len, const CHAR **ptr);

         INT32 writeTail(UINT64 v);
         INT32 readTail(UINT64 &value);
         OSS_INLINE BOOLEAN validMMapPtr(UINT32 offset, UINT32 len)
         {
            return (offset + len) <= _size;
         }
      private:
         GLOBAL_PAGE_ID _gpid;
         UINT32 _size;
         /// if some one update flags to uint64, should
         /// update isOplistHead. BOOLEAN is 4bytes.
         UINT32 _flags;
         UINT32 _status;
         ossValuePtr _ptr;
         liteCacheTuple _lcTuple;
         storageUnit *_su;
         CHAR *_fullDumpBuf;  /// buffer size always be page size
         UINT32 _fullDumpSize;/// data size dumped.
         DPS_LSN_OFFSET _oplist;
   };//class pageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_PAGE_ACCESSOR_H_