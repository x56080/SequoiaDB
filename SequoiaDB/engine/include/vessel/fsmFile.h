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

#include "vessel/extentStorageFile.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class requestContext;

   class fsmFile : public extentStorageFile
   {
      public:
         fsmFile();
         virtual ~fsmFile();

      public:
         INT32 initAfterCreation();
         INT32 initAfterOpen();
         INT32 allocateNewPage(PAGE_ID &pid);
         INT32 releasePages(UINT32 count, const PAGE_ID *pids);

      private:
         virtual SPACE_TYPE getSpaceType()const
         {
            return SPACE_TYPE_FSM;
         }
         virtual const CHAR *getMagicChars()const
         {
            return "SDBVFSMF";
         }

         virtual INT32 afterHeadOpen(){return SDB_OK;}

      private:
         INT32 findFreePageFromSmp(PAGE_ID &pid);

         INT32 allocateFreePageFromSmp(PAGE_ID pid);

         INT32 releasePagesFromSmp(UINT32 count, const PAGE_ID *pids);

         INT32 ensureSpace(PAGE_ID pid);

      private:
         ossSpinXLatch _latch;
         INT32 _firstFree;

   };//class fsmFIle
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_FSM_FILE_H_