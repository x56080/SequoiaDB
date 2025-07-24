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

   Source File Name = dmlHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_DML_HANDLER_H_
#define VESSEL_DML_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/slice.h"
#include "utilInsertResult.hpp"
#include "dpsTransID.hpp"
#include "vessel/objectIdentifier.h"
#include "interface/IRecordUpdater.h"
#include "vessel/dmlRequest.h"


namespace engine
{
namespace vessel
{
   class insertOptions;
   class dmlHandler : public requestHandler
   {
      public:
         dmlHandler(){}
         virtual ~dmlHandler(){}

      public:
         INT32 insert(const globalCollectionId &gcid,
                      const dmlInsertRequest &request,
                      utilInsertResult *res);

         INT32 insertBatch(const globalCollectionId &gcid,
                           const dmlBatchInsertRequest &request,
                           utilInsertResult *res);

         INT32 update(const globalCollectionId &gcid,
                      const dmlUpdateRequest &request,
                      IRecordUpdater *updater,
                      utilUpdateResult *res);

         INT32 remove(const globalCollectionId &gcid,
                      const dmlRemoveRequest &request,
                      utilDeleteResult *res);

   };//class dmlHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_DML_HANDLER_H_-