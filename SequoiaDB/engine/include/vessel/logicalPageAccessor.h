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

   Source File Name = logicalPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_ACCESSOR_H_
#define VESSEL_LOGICAL_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/lpidLockHelper.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class logicalPageSpace;

   class logicalPageAccessor : public pageAccessor
   {
      public:
         logicalPageAccessor();
         ~logicalPageAccessor();
         logicalPageAccessor(const logicalPageAccessor &) = delete;
         logicalPageAccessor &operator=(const logicalPageAccessor &) = delete;

      public:
         INT32 init(requestContext *context,
                    FILE_TYPE type,
                    PAGE_ID lpid,
                    const pageAccessor::options &o,
                    logicalPageSpace *space,
                    DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET);

         void fini(requestContext *context);

      protected:
         virtual INT32 prepareToWrite(requestContext *context);

      private:
         INT32 _init(requestContext *context,
                     FILE_TYPE type,
                     UINT32 flags,
                     DPS_LSN_OFFSET oplist,
                     BOOLEAN toWrite);

      private:
         PAGE_ID _lpid = INVALID_PAGE_ID;
         SNAPSHOT_ID _snapshot = INVALID_SNAPSHOT_ID;
         lpidLockHelper _lh;
         logicalPageSpace *_space = NULL;
   };//class logicalPageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_ACCESSOR_H_