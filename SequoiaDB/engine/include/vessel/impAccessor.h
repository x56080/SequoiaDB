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
#include "vessel/idMapPage.h"

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
         INT32 initPage(requestContext *context);

         ///WARNING: will return the current actual stored value,
         /// regardless of whether the pid is valid
         INT32 getPidByOffset(UINT32 offset, PAGE_ID *pid, SNAPSHOT_ID *snapID);

         ///WARNING: will return the current actual stored value,
         /// regardless of whether the pid is valid
         /// user should guarantee that lpid is in valid range of this page.
         /// it will get offset by mod capacity simply in func.
         INT32 getPidByLpid(PAGE_ID lpid, PAGE_ID *pid, SNAPSHOT_ID *snapID);

         INT32 map(requestContext *context,
                   UINT32 count,
                   const PAGE_ID *lpids,
                   const PAGE_ID *pids,
                   SNAPSHOT_ID snap);

         /// WARNING: size of bits should be enough.
         INT32 dumpAsBitMap(UINT64 *bits, UINT32 &free);
      private:
         INT32 validateMap(UINT32 capacity,
                           UINT32 count,
                           const PAGE_ID *lpids,
                           const PAGE_ID *pids,
                           SNAPSHOT_ID snap);

         INT32 getSlot(UINT32 slot, idMapSlot &value);

         INT32 writeSlot(UINT32 slot, const idMapSlot &value);

         void mapLpids(UINT32 capacity,
                       UINT32 count,
                       const PAGE_ID *lpids,
                       const PAGE_ID *pids,
                       SNAPSHOT_ID snap);
         
         void unmapLpids(UINT32 capacity,
                         UINT32 count,
                         const PAGE_ID *lpids);
         
         INT32 prepareMapLog(requestContext *context,
                             logRecordContext *lrc,
                             UINT32 count);

         INT32 commitMapLog(requestContext *context,
                            logRecordContext *lrc,
                            UINT32 count,
                            const PAGE_ID *lpids,
                            const PAGE_ID *pids,
                            SNAPSHOT_ID snap);

      private:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_ID_MAP;
         }
   };//class csgpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_IMP_ACCESSOR_H_