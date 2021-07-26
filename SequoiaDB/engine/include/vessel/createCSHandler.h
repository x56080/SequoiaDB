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

   Source File Name = createCSHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CREATE_CS_HANDLER_H_
#define VESSEL_CREATE_CS_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/requestContext.h"
#include "vessel/collectionSpaceOptions.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class createCSHandler : public requestHandler
   {
      public:
         createCSHandler();
         virtual ~createCSHandler();

      public:
         INT32 doit(const CHAR *name,
                    utilCSUniqueID uniqueId,
                    const createCSOptions &options);

      private:
         INT32 validateOptions(const strSlice &name,
                               const createCSOptions &options);

   };//class createCSHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_CREATE_CS_HANDLER_H_