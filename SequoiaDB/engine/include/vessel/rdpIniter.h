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

   Source File Name = rdpIniter.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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