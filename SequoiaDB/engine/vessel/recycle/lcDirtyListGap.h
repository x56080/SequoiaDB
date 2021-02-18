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

   Source File Name = lcDirtyListGap.h

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

#ifndef VESSEL_LC_DIRTY_LIST_GAP_H_
#define VESSEL_LC_DIRTY_LIST_GAP_H_

#include "vessel/latch.h"
#include <list>

namespace engine
{
namespace vessel
{
   class lcDirtyListGap
   {
      public:
         lcDirtyListGap(){}
         ~lcDirtyListGap(){}

      public:
         INT32 setup(UINT64 lsn);
         INT32 teardown();
         INT32 removeGap(UINT64 lsn, UINT32 len);
         UINT64 getMinGapLSN();
      private:
         struct lsnGap
         {
            lsnGap(UINT64 b, UINT64 e)
            :begin(b), end(e){}
            UINT64 begin;
            UINT64 end;
         };

      private:
         SPIN_MUTEX _mutex;
         std::list<lsnGap> _gaps;
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif
