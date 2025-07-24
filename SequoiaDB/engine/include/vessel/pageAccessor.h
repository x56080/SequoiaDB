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

   Source File Name = pageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_PAGE_ACCESSOR_H_
#define VESSEL_PAGE_ACCESSOR_H_

#include "vessel/globalPageID.h"
#include "vessel/pageDef.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   class runtimePageBuffer;
   class requestContext;

   class pageAccessor : public SDBObject
   {
      public:
         pageAccessor() = default;
         virtual ~pageAccessor() = default;

      public:
         pageAccessor(const pageAccessor &) = delete;
         pageAccessor &operator=(const pageAccessor &) = delete;

      public:
         void setLSN(DPS_LSN_OFFSET lsn) {_lsn = lsn;}
         DPS_LSN_OFFSET getLSN()const {return _lsn;}
      private:
         DPS_LSN_OFFSET _lsn = DPS_INVALID_LSN_OFFSET;
   
   };//class pageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_PAGE_ACCESSOR_H_