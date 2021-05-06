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
   const UINT32 PAGE_ACCESSOR_FLAG_CACHE_MODE = 0x01;  /// buffer pool accessing
   const UINT32 PAGE_ACCESSOR_FLAG_NON_READONLY = 0x02;
   const UINT32 PAGE_ACCESSOR_FLAG_NO_PAGE_VALIDATION = 0x04;
   const UINT32 PAGE_ACCESSOR_FLAG_OPLIST_HEAD = 0x08;
   const UINT32 PAGE_ACCESSOR_FLAG_OPLIST_TAIL = 0x10;

   class storageUnit;
   class logRecordContext;

   class pageAccessor : public SDBObject
   {
      public:
         OSS_INLINE pageAccessor(){}
         virtual ~pageAccessor();

      public:
         pageAccessor(const pageAccessor &) = delete;
         pageAccessor &operator=(const pageAccessor &) = delete;

      public:
         struct options
         {
            OSS_INLINE options(){}
            OSS_INLINE ~options(){}
            OSS_INLINE options(const options &o):
            cacheMode(o.cacheMode),
            readOnly(o.readOnly),
            pageValidation(o.pageValidation),
            oplistHead(o.oplistHead),
            oplistTail(o.oplistTail){}
            options &operator=(const options &o)
            {
               cacheMode = o.cacheMode;
               readOnly = o.readOnly;
               pageValidation = o.pageValidation;
               oplistHead = o.oplistHead;
               oplistTail = o.oplistTail;
               return *this;
            }
            OSS_INLINE UINT32 getFlags()const
            {
               UINT32 flags = 0;
               if (cacheMode)
               {
                  OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_CACHE_MODE);
               }
               if (!readOnly)
               {
                  OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_NON_READONLY);
               }
               if (!pageValidation)
               {
                  OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_NO_PAGE_VALIDATION);
               }
               if (oplistHead)
               {
                  OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_OPLIST_HEAD);
               }
               if (oplistTail)
               {
                  OSS_BIT_SET(flags, PAGE_ACCESSOR_FLAG_OPLIST_TAIL);
               }
               return flags;
            }


            BOOLEAN cacheMode = FALSE;
            BOOLEAN readOnly = TRUE;
            BOOLEAN pageValidation = TRUE;
            BOOLEAN oplistHead = FALSE;
            BOOLEAN oplistTail = FALSE;
         };//struct options

      public:
         OSS_INLINE const GLOBAL_PAGE_ID &getGPID()const
         {
            return _gpid;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _pageSize;
         }

         OSS_INLINE UINT32 getFlags()const
         {
            return _flags;
         }
         OSS_INLINE DPS_LSN_OFFSET getOplist() const
         {
            return _oplist;
         }
         OSS_INLINE BOOLEAN isCacheMode()const
         {
            return 0 != OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_CACHE_MODE);
         }
         OSS_INLINE BOOLEAN isReadOnly()const
         {
            return 0 == OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_NON_READONLY);
         }
         UINT32 getPageBodySize()const;
         OSS_INLINE BOOLEAN isInitailized()const
         {
            return !_gpid.invalid();
         }

      public:
         ///init accessor by flags to choose mode.
         INT32 initUniversally(requestContext *context,
                               FILE_TYPE type,
                               PAGE_ID pid,
                               UINT32 flags = 0,
                               storageUnit *su = NULL,
                               DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET);

         INT32 initWithOptions(requestContext *context,
                               FILE_TYPE type,
                               PAGE_ID pid,
                               const options &o,
                               storageUnit *su = NULL,
                               DPS_LSN_OFFSET oplist = DPS_INVALID_LSN_OFFSET);

         INT32 initWithMMapMode(requestContext *context,
                                FILE_TYPE type,
                                PAGE_ID pid,
                                UINT32 pageSize,
                                ossValuePtr ptr,
                                BOOLEAN pageValidation = TRUE,/// set it as false when first init page.
                                BOOLEAN readOnly = TRUE);

         void fini(requestContext *context);

         virtual PAGE_TYPE getPageType()const = 0;

      protected:
         virtual INT32 prepareToWrite(requestContext *context);
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

         void abortToWrite();
         void commit(requestContext *context, DPS_LSN_OFFSET lsn);
         INT32 prepareLogDone(requestContext *context,
                              logRecordContext *lrc);

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
         OSS_INLINE BOOLEAN isInOplist()const
         {
            return DPS_INVALID_LSN_OFFSET != _oplist;
         }
         OSS_INLINE BOOLEAN isOplistHead()const
         {
            return 0 != OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_OPLIST_HEAD);
         }
         OSS_INLINE BOOLEAN isOplistTail()const
         {
            return OSS_BIT_TEST(_flags, PAGE_ACCESSOR_FLAG_OPLIST_TAIL);
         }
         BOOLEAN fullAccessing()const;
      private:
         INT32 validateMMapPageHeadAndTail();
         BOOLEAN accessing()const;

         INT32 beginToAccess(requestContext *context,
                             storageUnit *su);
         INT32 beginToAccessByMMap(storageUnit *su);
         INT32 beginToAccessByCache(requestContext *context);

         INT32 prepareToWriteByCache(requestContext *context);
         INT32 getMMapWritePtrOfPage(UINT32 offset, UINT32 len, CHAR **ptr);
         INT32 getMMapReadPtrOfPage(UINT32 offset, UINT32 len, const CHAR **ptr);

         INT32 writeTail(UINT64 v);
         INT32 readTail(UINT64 &value);
         OSS_INLINE BOOLEAN validMMapPtr(UINT32 offset, UINT32 len)
         {
            return (offset + len) <= _pageSize;
         }

      private:
         enum ACCESSOR_STATUS
         {
            ACCESSOR_STATUS_INVALID = 0,
            ACCESSOR_STATUS_READONLY_ACCESSING = 1,
            ACCESSOR_STATUS_FULL_ACCESSING = 2,
         };//enum ACCESSOR_STATUS
      private:
         GLOBAL_PAGE_ID _gpid;
         UINT32 _pageSize = 0;
         UINT32 _flags = 0;
         UINT32 _status = 0;
         ossValuePtr _ptr = 0;
         liteCacheTuple _lcTuple;
         DPS_LSN_OFFSET _oplist = DPS_INVALID_LSN_OFFSET;
   };//class pageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_PAGE_ACCESSOR_H_