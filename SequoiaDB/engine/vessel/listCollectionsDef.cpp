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

   Source File Name = listCollectionsDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/listCollectionsDef.h"
#include "vessel/collectionProperties.h"

namespace engine
{
namespace vessel
{
   bson::BSONObj dumpCollectionWhenList(UINT32 csLogicalID,
                                        const collectionProperties *properties)
   {
      SDB_ASSERT(nullptr != properties, "can not be invalid");
      bson::BSONObjBuilder builder;
      builder.append(CL_DUMP_RECORD_FIELD_CS_LOGICAL_ID, csLogicalID)
             .append(CL_DUMP_RECORD_FIELD_MB_ID, properties->clid.getMbId())
             .append(CL_DUMP_RECORD_FIELD_NAME, properties->name)
             .append(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID, properties->clid.getLid())
             .append(CL_DUMP_RECORD_FIELD_INNER_ID, properties->clid.getInnerId())
             .append(CL_DUMP_RECORD_FIELD_COMPRESSION, properties->compressor)
             .append(CL_DUMP_RECORD_FIELD_MIN_STRIPING, properties->stripingRange.getLow().getValue())
             .append(CL_DUMP_RECORD_FIELD_MAX_STRIPING, properties->stripingRange.getHigh().getValue());
      return builder.obj();
   }
}//namespace vessel
}//namespace engine
