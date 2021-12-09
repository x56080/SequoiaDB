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

#include "vessel/cursorOptions.h"
#include "vessel/slice.h"
#include "vessel/memoryBlock.h"
#include <initializer_list>
#include "vessel/localThreadSharedPointer.h"
#include "vessel/cursorDef.h"
#include "vessel/cursorRow.h"
#include "sdbInterface.hpp"

namespace engine
{
namespace vessel
{
   class vesselImpl;
   class IQueryFilter;

   class cursorKernal : public localThreadSharedCounter
   {
      public:
         cursorKernal(){}
         virtual ~cursorKernal();
         cursorKernal(const cursorKernal &) = delete;
         cursorKernal &operator=(const cursorKernal &) = delete;

      public:
         virtual CURSOR_TYPE getType()const = 0;

         virtual INT32 getNextRow(IExecutor *executor,
                                  cursorRow *row){return SDB_VESSEL_INTERNAL_ERR;}

      public:
         BOOLEAN isOpen()const;
         INT32 open(vesselImpl *db,
                    IQueryFilter *filter, 
                    const cursorOptions *o=NULL);
         void close();
         ///return SDB_VESSEL_END_OF_CURSOR when hit the end.
         INT32 getNext(IExecutor *executor, slice &content);
         
         /// push completed record
         INT32 pushData(UINT32 len, const CHAR *data);

         /// push "ONE RECORD" with multi memory fragments
         INT32 pushDataFragments(std::initializer_list<slice> il);

         /// mark cursor as SDB_VESSEL_EOC
         void pushEnd();

         OSS_INLINE IQueryFilter *getFilter()
         {
            return _filter;
         }

         BOOLEAN isWaitingMorePushing()const;

         OSS_INLINE BOOLEAN hasRowLimit()const
         {
            return _options.hasRowCountLimit();
         }
         OSS_INLINE INT32 getRowLimit()const
         {
            return _options.rowCountLimit;
         }
      
      private:
         INT32 allocateSpaceForPushing(UINT32 dataLen);
         
         BOOLEAN hitTheEnd()const;
         OSS_INLINE UINT32 getRealBufSizeOfSlice(UINT32 dataLen)const
         {
            return sizeof(UINT32) + dataLen;
         }
         OSS_INLINE BOOLEAN hasMoreDataToFetch()const
         {
            return _read < _mb.getSize();
         }
      
      private:
         cursorOptions _options;
         UINT32 _flags = 0;
         memoryBlock _mb;
         UINT32 _pushedThisLoop = 0;
         UINT32 _read = 0; /// buffer size read.
         vesselImpl *_db = NULL;
         IQueryFilter *_filter = NULL;
   };//class cursorKernal
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_KERNAL_H_