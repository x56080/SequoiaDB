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

   Source File Name = indexDefPage.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_DEF_PAGE_H_
#define VESSEL_INDEX_DEF_PAGE_H_

#include "vessel/vesselDef.h"
#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   const static INVALID_INDEX_RECORD_VERSION = 0;
   const static INDEX_RECORD_VERSION_1 = 1;
   const static MAX_INDEX_NAME_LEN = 64;

   const static INDEX_DEF_FLAG_UNIQUE = 1;
   const static INDEX_DEF_FLAG_ENFORECE = 2;
   const static INDEX_DEF_FLAG_NOT_NULL = 4;

   struct indexDefRecord
   {
      UINT16 version;
      UINT16 type;
      UINT32 indexLogicalID;
      UINT32 clLogicalID;
      UINT32 flags;
      UINT32 ordering;
      UINT32 fieldsCount;

      /// valid only when type is btree
      UINT32 btreeFlags;
      PAGE_ID btreeRoot;

      CHAR name[MAX_INDEX_NAME_LEN];
      UINT32 keyPatternLen;
   };//struct indexDefRecord
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DEF_PAGE_H_