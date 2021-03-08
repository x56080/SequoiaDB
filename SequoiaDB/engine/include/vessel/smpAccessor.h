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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
         INT32 initSMP(UINT32 maxSegmentCount,
                       UINT32 pageCountOfSeg,
                       UINT32 pageOccupied);

         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_SMP;
         }

         INT32 allocatePages(PAGE_TYPE type,
                             UINT32 count,
                             const PAGE_ID *lpids,
                             const PAGE_ID *pids,
                             const slice &args,
                             DPS_LSN_OFFSET *oplist);

      private:
         INT32 validatePidsToBeAllocated(UINT32 count,
                                         const PAGE_ID *pids);
         INT32 testPageFree(UINT32 bitsSlotNo, UINT32 bitNo, BOOLEAN &free);

         void setPagesFree(spaceManagementPageHead *head,
                           UINT32 count,
                           const PAGE_ID *pids);
         void setPagesNotFree(spaceManagementPageHead *head,
                              UINT32 count,
                              const PAGE_ID *pids);
         INT32 testPagesFree(UINT32 count, const PAGE_ID *pids, BOOLEAN &free);

         INT32 setPageNotFree(UINT32 bitsSlotNo, UINT32 bitNo);
         INT32 setPageFree(UINT32 bitsSlotNo, UINT32 bitNo);

         INT32 writeBits(UINT32 bitsSlotNo, UINT32 bits);
         INT32 readBits(UINT32 bitsSlotNo, UINT32 &bits);

         INT32 prepareSMPAllocateLog(logRecordContext *lrc,
                                     BOOLEAN oplist,
                                     UINT32 count,
                                     const slice &args);

         INT32 commitSMPAllocateLog(logRecordContext *lrc,
                                    PAGE_TYPE type,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    const PAGE_ID *pids,
                                    UINT32 free,
                                    const slice &args);
   };//class smpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_SMP_ACCESSOR_H_