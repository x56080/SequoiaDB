/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = impDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         9/7/2023     HYQ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_DEF_HPP_
#define IMP_DEF_HPP_

#include "core.hpp"

namespace import
{

   #define IMP_FILED_NAME_ID     "_id"

   // the min number of indexes
   #define IMP_HINT_MIN_NUM   1
   // the max number of indexes
   #define IMP_HINT_MAX_NUM   64

   enum INPUT_TYPE
   {
      INPUT_FILE = 0,
      INPUT_STDIN,
      INPUT_EXEC
   };

   enum INPUT_FORMAT
   {
      FORMAT_CSV = 0,
      FORMAT_JSON
   };

   enum STR_TRIM_TYPE
   {
      STR_TRIM_NO = 0,
      STR_TRIM_RIGHT,
      STR_TRIM_LEFT,
      STR_TRIM_BOTH
   };

   enum DECIMAL_TO_TYPE
   {
      DECIMALTO_DEFAULT = 0,
      DECIMALTO_DOUBLE,
      DECIMALTO_STRING
   };

   enum IMPORT_MODE
   {
      INSERT = 0,
      UPSERT
   };

}

#endif /* IMP_DEF_HPP_ */
