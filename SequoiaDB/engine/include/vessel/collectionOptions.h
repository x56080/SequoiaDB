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

   Source File Name = collectionOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_OPTIONS_H_
#define VESSEL_COLLECTION_OPTIONS_H_

#include "vessel/vesselIdDef.h"
#include "vessel/collectionRecordPage.h"
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
         UINT16 type = COLLECTION_TYPE_NORMAL;
         UINT16 minFreePercent = 10; ///valid range [0, 50]
         UTIL_COMPRESSOR_TYPE compressionType = UTIL_COMPRESSOR_INVALID;
         dmsStripingRange stripingRange;
         //UINT32 stripingBucketCount = 1;
   };// class createCLOptions
}//namespace vessel
}//namespace engine


#endif//VESSEL_COLLECTION_OPTIONS_H_