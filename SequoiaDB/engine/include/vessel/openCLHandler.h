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

   Source File Name = openCLHandler.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OPEN_CL_HANDLER_H_
#define VESSEL_OPEN_CL_HANDLER_H_

#include "vessel/requestHandler.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionHandler.h"

namespace engine
{
namespace vessel
{
   class openCLHandler : public requestHandler
   {
      public:
         openCLHandler(){}
         virtual ~openCLHandler(){}

      public:
         INT32 doit(const strSlice &csName,
                    const strSlice &clName,
                    const openCLOptions &options,
                    collectionHandler &clHandler);
   };//class openCLHandler
}//namespace vessel
}//namespace engine

#endif//VESSEL_OPEN_CL_HANDLER_H_