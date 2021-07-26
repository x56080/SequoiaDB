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

namespace engine
{
namespace vessel
{
   bson::BSONObj dumpCollectionWhenList(UINT32 csLogicalID,
                                        const collectionRecord &record)
   {
      bson::BSONObjBuilder builder;
      builder.append(CL_DUMP_RECORD_FIELD_CS_LOGICAL_ID, csLogicalID)
             .append(CL_DUMP_RECORD_FIELD_MB_ID, record.mbID)
             .append(CL_DUMP_RECORD_FIELD_NAME, record.name)
             .append(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID, record.logicalCLID)
             .append(CL_DUMP_RECORD_FIELD_INNER_ID, record.innerID)
             .append(CL_DUMP_RECORD_FIELD_COMPRESSION, record.compressionType)
             .append(CL_DUMP_RECORD_FIELD_MIN_STRIPING, record.minStriping)
             .append(CL_DUMP_RECORD_FIELD_MAX_STRIPING, record.maxStriping);
      return builder.obj();
   }
}//namespace vessel
}//namespace engine
