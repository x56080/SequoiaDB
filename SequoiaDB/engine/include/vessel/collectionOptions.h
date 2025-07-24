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

   Source File Name = collectionOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_COLLECTION_OPTIONS_H_
#define VESSEL_COLLECTION_OPTIONS_H_

#include "vessel/vesselIdDef.h"
#include "vessel/clMetaBlockPage.h"
#include "../bson/bson.hpp"
#include "dmsEngineDef.hpp"
#include "dmsStripingId.hpp"

namespace engine
{
namespace vessel
{
   static const CHAR * const CRT_CL_OPTIONS_FIELD_TYPE = "type";
   static const CHAR * const CRT_CL_OPTIONS_FIELD_MIN_FREE_PERCENT = "min_free_percent";
   static const CHAR * const CRT_CL_OPTIONS_FIELD_COMPRESSION = "compression";
   static const CHAR * const CRT_CL_OPTIONS_FIELD_MIN_STRIPING = "min_striping";
   static const CHAR * const CRT_CL_OPTIONS_FIELD_MAX_STRIPING = "max_striping";


   class createCLOptions : public SDBObject
   {
      public:
      createCLOptions(){}
      ~createCLOptions(){}
   
      public:

      BOOLEAN isValid()const;

      bson::BSONObj toBson()const;


      public:
         UINT16 type = CL_TYPE_NORMAL;
         UINT16 minFreePercent = 10; ///valid range [0, 50]
         UTIL_COMPRESSOR_TYPE compressionType = UTIL_COMPRESSOR_INVALID;
         dmsStripingRange stripingRange;
         //UINT32 stripingBucketCount = 1;
   };// class createCLOptions
}//namespace vessel
}//namespace engine


#endif//VESSEL_COLLECTION_OPTIONS_H_