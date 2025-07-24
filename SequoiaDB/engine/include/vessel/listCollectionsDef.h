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

   Source File Name = listCollectionsDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
