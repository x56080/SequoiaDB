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

   Source File Name = vesselFileDef.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/vesselFileDef.h"
#include "ossUtil.hpp"

namespace engine
{
namespace vessel
{
   const CHAR * const FILE_TYPE_SUFFIX_ARRAY[] =
   {
      FILE_TYPE_SUFFIX_DATAM,
      FILE_TYPE_SUFFIX_DATAD,
      FILE_TYPE_SUFFIX_IDXM,
      FILE_TYPE_SUFFIX_IDXD,
      FILE_TYPE_SUFFIX_LOBM,
      FILE_TYPE_SUFFIX_LOBDM,
      FILE_TYPE_SUFFIX_LOBDD,
      FILE_TYPE_SUFFIX_FSM,
      FILE_TYPE_SUFFIX_CSNAME,
      FILE_TYPE_SUFFIX_CONTROL,
      FILE_TYPE_SUFFIX_DELTA
   };

   BOOLEAN parseFileSuffix(const CHAR *suffix, FILE_TYPE &type)
   {
      BOOLEAN r = FALSE;
      if (NULL == suffix)
      {
         goto done;
      }

      for (UINT32 i = 0; i < FILE_TYPE_SUFFIX_ARR_SIZE; ++i)
      {
         const CHAR *s = FILE_TYPE_SUFFIX_ARRAY[i];
         if (0 == ossStrcmp(suffix, s))
         {
            r = TRUE;
            type = i;
            goto done;
         }
      }
   done:
      return r;
   }
}//namespace vessel
}//namespace engine