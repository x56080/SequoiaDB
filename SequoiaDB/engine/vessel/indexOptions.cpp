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

   Source File Name = indexOptions.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexOptions.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   BOOLEAN createIndexOptions::isValid()const
   {
      BOOLEAN r = FALSE;
      if (INDEX_TYPE_BTREE != type &&
          INDEX_TYPE_LSM != type)
      {
         goto done;
      }
      if (INDEX_TYPE_BTREE == type &&
          0 != btreePrefixCompressionColumns)
      {
         if (MAX_INDEX_BTREE_PREFIX_COMPRESSION_COLUMNS <
            btreePrefixCompressionColumns)
         {
            goto done;
         }
      }
      if (0 != sortBufferSize &&
          !ossIsPowerOf2(sortBufferSize))
      {
         goto done;
      }
      if (MAX_BUILDING_INDEX_SORT_BUF_SIZE < sortBufferSize)
      {
         goto done;
      }
   done:
      return r;
   }

   BOOLEAN isValidCreatingIndexArgs(const strSlice &indexName,
                                    const indexKeyPattern &pattern,
                                    const createIndexOptions &options)
   {
      BOOLEAN r = FALSE;

      if (indexName.empty() ||
          MAX_INDEX_NAME_LEN < indexName.strLen())
      {
         goto done;
      }
      else if (!pattern.isValid())
      {
         goto done;
      }
      else if (!options.isValid())
      {
         goto done;
      }
      else if (INDEX_TYPE_BTREE == options.type &&
               pattern.getKeyCount() < options.btreePrefixCompressionColumns)
      {
         goto done;
      }
      r = TRUE;
   done:
      return r;
   }

}//namespace vessel
}//namespace engine