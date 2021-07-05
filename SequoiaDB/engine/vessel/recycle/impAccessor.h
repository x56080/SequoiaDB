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
#include "vessel/inMemBitMap.h"

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
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_ID_MAP;
         }
         virtual UINT32 getUserPageHeadSize()const
         {
            return ID_MAP_PAGE_HEAD_SIZE;
         }
      
      public:
         ///WARNING: will return the current actual stored value,
         /// regardless of whether the pid is valid
         INT32 getPidByOffset(UINT32 offset,
                              PAGE_ID *pid,
                              PAGE_SNAPSHOT_VERION *psv)const;

         /// lpids must be free.
         INT32 mapNewLpids(requestContext *context,
                           UINT32 count,
                           const PAGE_ID *lpids,
                           const PAGE_ID *pids,
                           PAGE_SNAPSHOT_VERION psv);

         INT32 dumpToBitmap(requestContext *context,
                            inMemBitMap &bitmap)const;
      private:
         INT32 validateNewMapping(requestContext *context,
                                  UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids,
                                  PAGE_SNAPSHOT_VERION psv)const;

         INT32 _mapNewLpids(requestContext *context,
                            UINT32 count,
                            const PAGE_ID *lpids,
                            const PAGE_ID *pids,
                            PAGE_SNAPSHOT_VERION psv);
         
         INT32 prepareRemapLog(requestContext *context,
                               logRecordContext *lrc,
                               UINT32 count,
                               BOOLEAN hasOldSlots);

         INT32 commitRemapLog(requestContext *context,
                              logRecordContext *lrc,
                              UINT32 count,
                              const PAGE_ID *lpids,
                              const PAGE_ID *pids,
                              PAGE_SNAPSHOT_VERION psv,
                              const idMapSlot *oldSlots);
   };//class csgpAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_IMP_ACCESSOR_H_