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

   enum CURSOR_TYPE
   {
      CURSOR_TYPE_INVALID = 0,
      CURSOR_TYPE_LIST_COLLECTION_SPACE = 1,
      CURSOR_TYPE_LIST_COLLECTION = 2,
      CURSOR_TYPE_SCAN_COLLECTION = 3,
   };

   class cursorKernal : public _utilPooledObject
   {
      public:
         cursorKernal(){}
         virtual ~cursorKernal();
         cursorKernal(const cursorKernal &) = delete;
         cursorKernal &operator=(const cursorKernal &) = delete;

      public:
         virtual CURSOR_TYPE getType()const = 0;

      public:
         BOOLEAN isOpen()const;
         INT32 open(vesselImpl *db,
                    IQueryFilter *filter, 
                    const cursorOptions *options);
         void close();
         ///return SDB_VESSEL_END_OF_CURSOR when hit the end.
         INT32 getNext(ISession *session, slice &content);
         
         /// push completed record
         INT32 push(const slice &content);
         INT32 push(UINT32 len, const CHAR *data);
         /// push one record with multi memory fragments
         INT32 pushFragments(std::initializer_list<std::pair<UINT32, const CHAR *>> il);

         /// mark cursor as SDB_VESSEL_END_OF_CURSOR
         void pushEnd();

         ///WARNING: At any time, you must check the rc code of "push"
         /// even "hasSpaceToPush" returns TRUE.
         BOOLEAN hasSpaceToPush(UINT32 size)const;

         OSS_INLINE IQueryFilter *getFilter()
         {
            return _filter;
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
         virtual void _close() {return;}

      private:
         INT32 extendBuf(UINT32 deltaSize);
         INT32 allocateSpaceForPushing(UINT32 dataLen);
         OSS_INLINE UINT32 getRealBufSizeOfSlice(UINT32 dataLen)const
         {
            return sizeof(UINT32) + dataLen;
         }
         OSS_INLINE BOOLEAN hasMoreDataToFetch()const
         {
            return _r < _w;
         }
         

      private:
         cursorOptions _options;
         UINT64 _usage = 0;
         UINT64 _totalPushed = 0;
         UINT32 _flags = 0;
         //UINT32 _fetchedSliceInBuf = 0;
         //const CHAR *_nextSlice = NULL;
         UINT32 _bufSize = 0;
         CHAR *_buf = NULL;
         UINT32 _w = 0; /// buffer size written.
         UINT32 _r = 0; /// buffer size read.
         vesselImpl *_db = NULL;
         IQueryFilter *_filter = NULL;
   };//class cursorKernal
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_KERNAL_H_