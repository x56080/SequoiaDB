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

   Source File Name = impAccessor.h

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

#ifndef VESSEL_IMP_ACCESSOR_H_
#define VESSEL_IMP_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   class IRedoLogger;
   class logRecordContext;
   ///id map page accessor
   class impAccessor : public pageAccessor
   {
      public:
         impAccessor();
         virtual ~impAccessor();
      
      public:
         INT32 initPage(PAGE_ID minLpid);

         INT32 getPid(PAGE_ID lpid, PAGE_ID *pid, SNAPSHOT_ID *snapID);

         INT32 remap(PAGE_ID lpid,
                     PAGE_ID pid,
                     SNAPSHOT_ID snap,
                     const DPS_LSN_OFFSET *oplist);

         INT32 getHeadContent(PAGE_ID &minLpid, UINT32 &capacity, UINT32 &free);

      private:
         INT32 getSlot(UINT32 slot, idMapSlot &value);

         INT32 writeSlot(UINT32 slot, const idMapSlot &value);
         
         INT32 prepareRemapLog(logRecordContext *lrc,
                               const DPS_LSN_OFFSET *oplist,
                               PAGE_ID lpid,
                               const idMapSlot &oldSlot,
                               const idMapSlot &newSlot);

         INT32 commitRemapLog(logRecordContext *lrc,
                               PAGE_ID lpid,
                               const idMapSlot &oldSlot,
                               const idMapSlot &newSlot);

      private:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_ID_MAP;
         }
   };//class csgpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_IMP_ACCESSOR_H_