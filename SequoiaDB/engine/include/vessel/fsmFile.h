/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = fsmFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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