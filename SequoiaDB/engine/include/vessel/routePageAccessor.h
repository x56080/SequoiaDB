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

   Source File Name = routePageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ROUTE_PAGE_ACCESSOR_H_
#define VESSEL_ROUTE_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"

namespace engine
{
namespace vessel
{
   class routePageAccessor : public pageAccessor
   {
      public:
         routePageAccessor(){}
         virtual ~routePageAccessor(){}

      public:
         INT32 initPage(requestContext *context,
                        PAGE_ID lpid,
                        UINT32 logicalId);

      public:
         virtual PAGE_TYPE getPageType()const
         {
            return PAGE_TYPE_ROUTE;
         }
   };//class routePageAccessor
}//namespace vessel
}//namespace engine
#endif//VESSEL_ROUTE_PAGE_ACCESSOR_H_