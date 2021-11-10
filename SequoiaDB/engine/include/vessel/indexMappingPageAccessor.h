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

   Source File Name = indexMappingPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_MAPPING_PAGE_ACCESSOR_H_
#define VESSEL_INDEX_MAPPING_PAGE_ACCESSOR_H_

#include "vessel/indexMappingPage.h"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class indexMappingPageAccessor : public SDBObject
   {
      public:
         indexMappingPageAccessor(){}
         ~indexMappingPageAccessor(){}

      public:
         /// lpid may be invalid
         INT32 getIndexDefPage(requestContext *context,
                               UINT32 pos,
                               logicalPageBuffer &lpb,
                               PAGE_ID &lpid)const;

         INT32 addNewMapping(requestContext *context,
                             UINT32 pos,
                             PAGE_ID lpid,
                             logicalPageBuffer &lpb)const;
   };//class indexMappingPageAccessor
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_MAPPING_PAGE_ACCESSOR_H_