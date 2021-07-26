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

   Source File Name = createCLHandler.h

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

#ifndef VESSEL_CREATE_CL_HANDLER_H_
#define VESSEL_CREATE_CL_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/requestContext.h"
#include "vessel/collectionOptions.h"
#include "vessel/strSlice.h"

namespace engine
{
namespace vessel
{
   class createCLHandler : public requestHandler
   {
      public:
         createCLHandler();
         virtual ~createCLHandler();

      public:
         INT32 doit(const strSlice &csName,
                    const strSlice &clName,
                    utilCLInnerID innerID,
                    const createCLOptions &options);

         INT32 doit(utilCLUniqueID clUniqueID,
                    const strSlice &clName,
                    const createCLOptions &options);

      private:
         INT32 validateOptions(const strSlice &csName,
                               const strSlice &clName,
                               const createCLOptions &options);
         INT32 validateOptions(utilCLUniqueID clUniqueID,
                              const strSlice &clName,
                              const createCLOptions &options);

   };//class createCLHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_CREATE_CL_HANDLER_H_