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

   Source File Name = listCollectionsDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LIST_COLLECTIONS_DEF_H_
#define VESSEL_LIST_COLLECTIONS_DEF_H_

#include "vessel/vesselIdDef.h"
#include "../bson/bson.hpp"


namespace engine
{
namespace vessel
{
   struct collectionProperties;
   static const CHAR * const CL_DUMP_RECORD_FIELD_CS_LOGICAL_ID = "cs_logical_id";
   static const CHAR * const CL_DUMP_RECORD_FIELD_MB_ID = "mbid";
   static const CHAR * const CL_DUMP_RECORD_FIELD_NAME = "name";
   static const CHAR * const CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID = "cl_logical_id";
   static const CHAR * const CL_DUMP_RECORD_FIELD_INNER_ID = "inner_id";
   static const CHAR * const CL_DUMP_RECORD_FIELD_COMPRESSION = "compression";
   static const CHAR * const CL_DUMP_RECORD_FIELD_MIN_STRIPING = "min_striping";
   static const CHAR * const CL_DUMP_RECORD_FIELD_MAX_STRIPING = "max_striping";

   bson::BSONObj dumpCollectionWhenList(UINT32 csLogicalID,
                                        const collectionProperties *properties);

}//namespace vessel
}//namespace engine

#endif//VESSEL_LIST_COLLECTION_SPACE_DEF_H_
