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

   Source File Name = dmlHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DML_HANDLER_H_
#define VESSEL_DML_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/slice.h"
#include "utilInsertResult.hpp"
#include "dpsTransID.hpp"
#include "vessel/objectIdentifier.h"
#include "vessel/api/IRecordUpdater.h"


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
                    const slice &record,
                    STRIPING_ID striping,
                    const insertOptions &options,
                    utilInsertResult *res);

         INT32 insertBatch(const globalCollectionId &gcid,
                    const ossPoolVector<slice> &batch,
                    const insertOptions &options,
                    utilInsertResult *res);

         INT32 update(const globalCollectionId &gcid,
                      const recordID &rid,
                      IRecordUpdater *updater,
                      utilUpdateResult *res);

   };//class dmlHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_DML_HANDLER_H_-