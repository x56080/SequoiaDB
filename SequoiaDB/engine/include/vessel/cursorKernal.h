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

   Source File Name = cursorKernal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CURSOR_KERNAL_H_
#define VESSEL_CURSOR_KERNAL_H_

#include "vessel/vesselOptions.h"
#include "vessel/slice.h"
#include "utilPooledObject.hpp"
#include <initializer_list>

namespace engine
{
namespace vessel
{
   class vesselImpl;
   class IQueryFilter;
   class ISession;

   class cursorKernal : public _utilPooledObject
   {
      public:
         cursorKernal():
         _usage(0),
         _flags(0),
         _totalSliceInBuf(0),
         _fetchedSliceInBuf(0),
         _nextSlice(NULL),
         _bufSize(0),
         _usedBufSize(0),
         _buf(NULL),
         _db(NULL),
         _filter(NULL)
         {}

         virtual ~cursorKernal();

      public:
         virtual CURSOR_TYPE getType()const = 0;

      public:
         BOOLEAN isOpen()const;
         INT32 open(vesselImpl *db,
                    IQueryFilter *filter, 
                    const cursorOptions *options);
         INT32 close();
         ///return SDB_VESSEL_END_OF_CURSOR when hit the end.
         INT32 getNext(ISession *session, slice &content);
         
         /// push complete data
         INT32 push(const slice &content);
         INT32 push(UINT32 len, const CHAR *data);
         /// push one record with multi memory fragments
         INT32 pushFragments(std::initializer_list<std::pair<UINT32, const CHAR *>> il);

         /// mark cursor as SDB_VESSEL_END_OF_CURSOR
         void pushEnd();

         ///WARNING: At any time, you must check the rc code of "push" when "hasNoSpaceToPush" return FALSE.
         BOOLEAN hasNoSpaceToPush(UINT32 size)const;

         OSS_INLINE IQueryFilter *getFilter()
         {
            return _filter;
         }
         OSS_INLINE UINT32 getTotalSlice()const
         {
            return _totalSliceInBuf;
         }
         OSS_INLINE UINT64 getUsageCount()const
         {
            return _usage;
         }
         OSS_INLINE void incUsageCount()
         {
            ++_usage;
         }
         OSS_INLINE UINT64 decUsageCount()
         {
            return --_usage;
         }

      private:
         virtual INT32 _open() {return SDB_OK;}
         virtual INT32 _close() {return SDB_OK;}

      private:
         INT32 extendBuf(UINT32 deltaSize);
         INT32 allocateSpaceForPushing(UINT32 dataLen);

         OSS_INLINE UINT32 getFreeBufSize()const
         {
            return _bufSize - _usedBufSize;
         }
         OSS_INLINE UINT32 getRealBufSizeOfSlice(UINT32 dataLen)const
         {
            return sizeof(UINT32) + dataLen;
         }

      private:
         cursorOptions _options;
         UINT64 _usage;
         UINT32 _flags;
         UINT32 _totalSliceInBuf;
         UINT32 _fetchedSliceInBuf;
         const CHAR *_nextSlice;
         UINT32 _bufSize;
         UINT32 _usedBufSize;
         CHAR *_buf;
         vesselImpl *_db;
         IQueryFilter *_filter;
   };//class cursorKernal
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_KERNAL_H_