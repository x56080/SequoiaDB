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

   Source File Name = collectionOptions.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/collectionOptions.h"

namespace engine
{
namespace vessel
{
   BOOLEAN createCLOptions::isValid()const
   {
      BOOLEAN r = FALSE;
      if (CL_TYPE_NORMAL != type)
      {
         goto done;
      }
      else if (50 < minFreePercent)
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }

   bson::BSONObj createCLOptions::toBson()const
   {
      bson::BSONObjBuilder builder;
      builder.append(CRT_CL_OPTIONS_FIELD_TYPE, type)
             .append(CRT_CL_OPTIONS_FIELD_MIN_FREE_PERCENT, minFreePercent)
             .append(CRT_CL_OPTIONS_FIELD_COMPRESSION, compressionType)
             .append(CRT_CL_OPTIONS_FIELD_MIN_STRIPING, stripingRange.getLow().getValue())
             .append(CRT_CL_OPTIONS_FIELD_MAX_STRIPING, stripingRange.getHigh().getValue());
      return builder.obj();
   }
}//namespace vessel
}//namespace engine