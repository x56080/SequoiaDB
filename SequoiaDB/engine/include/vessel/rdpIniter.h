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

   Source File Name = rdpIniter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RDP_INITER_H_
#define VESSEL_RDP_INITER_H_

#include "vessel/pageInitializer.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   class rdpIniter : public pageInitializer
   {
      public:
         virtual INT32 initPage(requestContext *context,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb);

         virtual INT32 initInTurns(requestContext *context,
                                   UINT32 i,
                                   PAGE_ID lpid,
                                   PAGE_SNAPSHOT_VERION psv,
                                   runtimePageBuffer *rpb);

         void init(UINT32 logicalId, UINT32 sequence, UINT32 batch)
         {
            _logicalId = logicalId;
            _sequence = sequence;
            _batchCount = batch;
         }

      private:
         UINT64 pack(UINT32 logicalId, UINT32 sequence)const;

      private:
         UINT32 _logicalId = DMS_INVALID_LOGICCLID;
         UINT32 _sequence = 0;
         UINT32 _batchCount = 0;
   };//class rdpIniter
}//namespace vessel
}//namespace engine

#endif//VESSEL_RDP_INITER_H_