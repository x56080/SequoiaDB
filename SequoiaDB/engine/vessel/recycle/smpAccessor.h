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

   Source File Name = smpAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SMP_ACCESSOR_H_
#define VESSEL_SMP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class spaceManagementPageHead;

   class smpAccessor : public pageAccessor
   {
      public:
         smpAccessor();
         virtual ~smpAccessor();

      public:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_SMP;
         }
         virtual UINT32 getUserPageHeadSize()const;

         /// pid in "pids" must be global pid.
         INT32 allocatePages(requestContext *context,
                             UINT32 count,
                             const PAGE_ID *pids);

         INT32 dumpBitmapSlice(requestContext *context,
                               UINT32 beginUint64,
                               UINT32 uint64Count,
                               UINT32 bufferSize,
                               void *buffer)const;

         INT32 getFreeCount(UINT32 &count)const;

      private:
         INT32 validatePidsToBeAllocated(UINT32 count,
                                         const PAGE_ID *pids)const;

         INT32 _allocatePages(requestContext *context,
                              UINT32 count,
                              const PAGE_ID *pids);

         INT32 prepareAllocateLog(requestContext *context,
                                  logRecordContext *lrc,
                                  UINT32 count);

         INT32 commitAllocateLog(requestContext *context,
                                 logRecordContext *lrc,
                                 UINT32 count,
                                 const PAGE_ID *pids);
   };//class smpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_SMP_ACCESSOR_H_