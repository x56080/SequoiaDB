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
#include "utilPooledObject.hpp"

namespace engine
{
namespace vessel
{
   class liteCache;
   class liteCachePageTag;
   class requestContext;

   ///WARNING: Should not share tulpe in multi threads.
   class liteCacheTuple
   {
      friend class liteCache;
      public:
         OSS_INLINE liteCacheTuple()
         {}

         OSS_INLINE ~liteCacheTuple()
         {
            release();
         }

         liteCacheTuple &operator=(const liteCacheTuple &);
         liteCacheTuple(const liteCacheTuple &);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _tag;
         }

         void release();

         void commit(UINT64 lsn);

         ossValuePtr getReadableBuffer()const;

         ossValuePtr getWritableBuffer()const;

         INT32 prepareToWrite(requestContext *context);

      private:
         class _sharedStatus : public _utilPooledObject
         {
            public:
               _sharedStatus(){}
               ~_sharedStatus(){}
               _sharedStatus(const _sharedStatus &) = delete;
               _sharedStatus &operator=(const _sharedStatus &) = delete;

            public:
               OSS_INLINE void setLockingMode(OSS_SHARED_LATCH_MODE mode)
               {
                  _lockingMode = mode;
               }
               OSS_INLINE OSS_SHARED_LATCH_MODE getLockingMode()const
               {
                  return (OSS_SHARED_LATCH_MODE)_lockingMode;
               }
               OSS_INLINE void incSharedCnt()
               {
                  ++_sharedCnt;
               }
               OSS_INLINE UINT32 decSharedCnt()
               {
                  return --_sharedCnt;
               }
               OSS_INLINE UINT32 getSharedCnt()const
               {
                  return _sharedCnt;
               }
               OSS_INLINE void setWritingPrepared()
               {
                  OSS_BIT_SET(_flags, 0x01);
               }
               OSS_INLINE BOOLEAN isWritingPrepared()const
               {
                  return 0 != OSS_BIT_TEST(_flags, 0x01);
               }

            private:
               UINT32 _sharedCnt = 0;
               UINT16 _lockingMode = OSS_SHARED_LATCH_MODE_NONE;
               UINT16 _flags = 0;
         };//class _sharedStatus
      
      private:/// for liteCache
         INT32 init(liteCachePageTag *tag,
                    OSS_SHARED_LATCH_MODE mode,
                    liteCache *pool,
                    BOOLEAN isWritingPrepared);

      private:
         ///All are null or All are not null.
         liteCachePageTag *_tag = NULL;
         liteCache *_pool = NULL;
         _sharedStatus *_status = NULL;
   };//class liteCacheTuple

} /// end of namespace vessel
} /// end of namespace engine

#endif
