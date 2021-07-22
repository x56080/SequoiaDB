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

   Source File Name = collectionSpaceOptions.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionSpaceOptions.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   static const CHAR * const CS_OPTION_DATA_PAGE_SIZE = "dataPageSize";
   static const CHAR * const CS_OPTION_DATA_SEG_SIZE = "dataSegSize";
   static const CHAR * const CS_OPTION_IDX_PAGE_SIZE = "idxPageSize";
   static const CHAR * const CS_OPTION_IDX_SEG_SIZE = "idxSegSize";
   static const CHAR * const CS_OPTION_LOB_PAGE_SIZE = "lobPageSize";
   static const CHAR * const CS_OPTION_LOB_SEG_SIZE = "lobSegSize";

   createCSOptions &createCSOptions::operator=(const createCSOptions &o)
   {
      dataPageSize = o.dataPageSize;
      dataSegSize = o.dataSegSize;
      idxPageSize = o.idxPageSize;
      idxSegSize = o.idxSegSize;
      lobPageSize = o.lobPageSize;
      lobSegSize = o.lobSegSize;
      return *this;
   }

   bson::BSONObj createCSOptions::toBson()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      bson::BSONObjBuilder builder;
      builder.append(CS_OPTION_DATA_PAGE_SIZE, dataPageSize);
      builder.append(CS_OPTION_DATA_SEG_SIZE, dataSegSize);
      builder.append(CS_OPTION_IDX_PAGE_SIZE, idxPageSize);
      builder.append(CS_OPTION_IDX_SEG_SIZE, idxSegSize);
      builder.append(CS_OPTION_LOB_PAGE_SIZE, lobPageSize);
      builder.append(CS_OPTION_LOB_SEG_SIZE, lobSegSize);
      return builder.obj();
   }

   BOOLEAN createCSOptions::loadFromBson(const bson::BSONObj &obj)
   {
      BOOLEAN r = FALSE;
      bson::BSONElement e;
      e = obj.getField(CS_OPTION_DATA_PAGE_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      dataPageSize = e.Int();

      e = obj.getField(CS_OPTION_DATA_SEG_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      dataSegSize = e.Int();

      e = obj.getField(CS_OPTION_IDX_PAGE_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      idxPageSize = e.Int();

      e = obj.getField(CS_OPTION_IDX_SEG_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      idxSegSize = e.Int();

      e = obj.getField(CS_OPTION_LOB_PAGE_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      lobPageSize = e.Int();

      e = obj.getField(CS_OPTION_LOB_SEG_SIZE);
      if (e.eoo() || bson::NumberInt != e.type())
      {
         goto error;
      }
      lobSegSize = e.Int();

      if (!isValid())
      {
         goto error;
      }

      r = TRUE;
   
   done:
      return r;
   error:
      dataPageSize = 0;
      goto done;
   }

   BOOLEAN createCSOptions::isValid()const
   {
      BOOLEAN r = FALSE;

      if (DMS_PAGE_SIZE32K != dataPageSize &&
          DMS_PAGE_SIZE64K != dataPageSize)
      {
         goto done;
      }
      if (DMS_PAGE_SIZE64K != idxPageSize &&
          DMS_PAGE_SIZE32K != idxPageSize &&
          DMS_PAGE_SIZE16K != idxPageSize &&
          DMS_PAGE_SIZE8K != idxPageSize)
      {
         goto done;
      }
      if (DMS_PAGE_SIZE4K != lobPageSize &&
          DMS_PAGE_SIZE8K != lobPageSize &&
          DMS_PAGE_SIZE32K != lobPageSize &&
          DMS_PAGE_SIZE64K != lobPageSize &&
          DMS_PAGE_SIZE128K != lobPageSize &&
          DMS_PAGE_SIZE256K != lobPageSize &&
          DMS_PAGE_SIZE512K != lobPageSize)
      {
         goto done;
      }

      if (!isValidSegmentSize(dataSegSize) ||
          !isValidSegmentSize(idxSegSize) ||
          !isValidSegmentSize(lobSegSize))
      {
         goto done;
      }
      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine
