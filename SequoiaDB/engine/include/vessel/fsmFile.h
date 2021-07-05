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

   Source File Name = fsmFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_FSM_FILE_H_
#define SDB_VESSEL_FSM_FILE_H_

#include "vessel/storageFile.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class requestContext;

   class fsmFile : public storageFile
   {
      public:
         fsmFile(){}
         virtual ~fsmFile(){}

      public:
         INT32 initToWork(ossSpinXLatch *latch, BOOLEAN sparse);
         INT32 allocateNewPage(PAGE_ID &pid, BOOLEAN sparse);
         INT32 releasePages(UINT32 count, const PAGE_ID *pids);

         OSS_INLINE BOOLEAN isReadyToWork()const
         {
            return _readyToWork;
         }

      private:
         virtual FILE_TYPE getFileType()const
         {
            return FILE_TYPE_FSM;
         }

      private:
         INT32 findFreePageFromSmp(PAGE_ID &pid);

         INT32 allocateFreePageFromSmp(PAGE_ID pid);

         INT32 releasePagesFromSmp(UINT32 count, const PAGE_ID *pids);

         INT32 ensureSpace(PAGE_ID pid, BOOLEAN sparse);

         INT32 initAfterCreation(BOOLEAN sparse);

      private:
         ossSpinXLatch *_latch = NULL;
         BOOLEAN _readyToWork = FALSE;
         INT32 _firstFree = -1;
   };//class fsmFIle
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_FILE_H_