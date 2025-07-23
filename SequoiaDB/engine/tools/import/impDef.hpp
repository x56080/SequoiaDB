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
