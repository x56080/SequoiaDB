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
         INT32 initSMP(requestContext *context,
                       UINT32 pageOccupied);

         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_SMP;
         }

         /// pid in "pids" must be global pid.
         /// they will be saved in redo log.
         INT32 allocatePages(requestContext *context,
                             PAGE_TYPE type,
                             UINT32 count,
                             const PAGE_ID *lpids,
                             const PAGE_ID *pids,
                             const slice &args);

         INT32 getFreeCount(requestContext *context,
                            INT32 &free);

         INT32 dumpSMP(requestContext *context,
                       UINT32 bufferSize,
                       CHAR *buffer);

      private:
         INT32 validatePidsToBeAllocated(UINT32 capacity,
                                         UINT32 bitsCount,
                                         UINT32 count,
                                         const PAGE_ID *pids);
         INT32 testPageFree(UINT32 bitsSlotNo, UINT32 bitNo, BOOLEAN &free);

         void setPagesFree(UINT32 bitsCount,
                           UINT32 count,
                           const PAGE_ID *pids);
         void setPagesNotFree(UINT32 bitsCount,
                              UINT32 capacity,
                              UINT32 count,
                              const PAGE_ID *pids);

         INT32 prepareSMPAllocateLog(requestContext *context,
                                     logRecordContext *lrc,
                                     UINT32 count,
                                     const PAGE_ID *lpids,
                                     const slice &args);

         INT32 commitSMPAllocateLog(requestContext *context,
                                    logRecordContext *lrc,
                                    const spaceManagementPageHead *oldHead,
                                    const spaceManagementPageHead *newHead,
                                    PAGE_TYPE type,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    const PAGE_ID *pids,
                                    const slice &args);
   };//class smpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_SMP_ACCESSOR_H_