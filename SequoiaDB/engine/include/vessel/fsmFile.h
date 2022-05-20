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
#include "vessel/bitmapScanner.h"
#include "vessel/freeSpaceMapDef.h"
#include <mutex>

namespace engine
{
namespace vessel
{
   class requestContext;

   class fsmFile : public storageFile
   {
      public:
         fsmFile();
         virtual ~fsmFile(){}

      public:
         INT32 initToWork();
         INT32 allocateNewPage(PAGE_ID &pid);

         ///releasing will not fsync file.
         INT32 releasePages(UINT32 count, const PAGE_ID *pids);

         fsmCLEntry* getEntrySlotPtr(CL_MB_ID mbID);

         INT32 fsyncEntry(CL_MB_ID mbID);

      private:
         INT32 _findFreePageFromSme(PAGE_ID &pid);

         INT32 _allocateFreePageFromSme(PAGE_ID pid);

         INT32 _releasePagesFromSme(UINT32 count, const PAGE_ID *pids);

         INT32 _ensureSpace(PAGE_ID pid);

         ossValuePtr _getSmePtr();

         ossValuePtr _getEntryArrayPtr();

         INT32 _fsyncSme();

         virtual UINT32 _getReservedAreaSize() const override;

         virtual INT32 _open(BOOLEAN isCreating) override;

      private:
         std::mutex _latch;
         bitmapScanner _smeScanner;
   };//class fsmFIle
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_FILE_H_