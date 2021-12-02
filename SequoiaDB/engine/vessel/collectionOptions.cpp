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

   Source File Name = collectionOptions.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionOptions.h"

namespace engine
{
namespace vessel
{
   BOOLEAN createCLOptions::isValid()const
   {
      BOOLEAN r = FALSE;
      if (COLLECTION_TYPE_NORMAL != type)
      {
         goto done;
      }
      else if (50 < minFreePercent)
      {
         goto done;
      }
      else if (INVALID_STRIPING_ID != minStriping ||
               INVALID_STRIPING_ID != maxStriping)
      {
         if (INVALID_STRIPING_ID == minStriping ||
             INVALID_STRIPING_ID == maxStriping)
         {
            goto done;
         }
         else if (maxStriping < minStriping)
         {
            goto done;
         }
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
             .append(CRT_CL_OPTIONS_FIELD_MIN_STRIPING, minStriping)
             .append(CRT_CL_OPTIONS_FIELD_MAX_STRIPING, maxStriping);
      return builder.obj();
   }
}//namespace vessel
}//namespace engine