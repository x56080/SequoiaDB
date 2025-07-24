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

   Source File Name = requestHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REQUEST_HANDLER_H_
#define VESSEL_REQUEST_HANDLER_H_

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "vessel/shallowPointer.hpp"
#include "vessel/requestContext.h"
#include "vessel/collection.h"

namespace engine
{
namespace vessel
{
   class requestHandler : public SDBObject
   {
      public:
         requestHandler(){}
         virtual ~requestHandler()
         {
         }

         requestHandler(const requestHandler &o) = delete;
         requestHandler &operator=(const requestHandler &o) = delete;

      protected:
         
         INT32 getCollectionObject(requestContext *context,
                                   const globalCollectionId &gcid,
                                   OSS_LATCH_MODE mode,
                                   COLLECTION_PTR &out);
   };//class requestHandler
}//namespace vessel
}//namespace engine

#endif //VESSEL_REQUEST_HANDLER_H_