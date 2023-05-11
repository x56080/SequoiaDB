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

   Source File Name = mthModifierDef.hpp

   Descriptive Name = Method Modifier Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for modify
   operation, which is changing a data record based on modification rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/11/2023  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMODIFIER_DEF_HPP_
#define MTHMODIFIER_DEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msgDef.hpp"
#include "mthDef.hpp"
#include "utilString.hpp"

namespace engine
{

   /*
      ModType define
   */
   enum ModType
   {
      INC = 0,
      SET,
      PUSH,
      PUSH_ALL,
      PULL,
      PULL_BY,
      PULL_ALL,
      PULL_ALL_BY,
      POP,
      UNSET,
      BITNOT,
      BITXOR,
      BITAND,
      BITOR,
      BIT,
      ADDTOSET,
      RENAME,
      NULLOPR,
      REPLACE,
      KEEP,
      SETARRAY,
      CURRENT_DATE,

      UNKNOWN
   } ;

   /*
      Mod operator define
   */
   #define MTH_MODIFIER_INC           "$inc"
   #define MTH_MODIFIER_SET           "$set"
   #define MTH_MODIFIER_PUSH          "$push"
   #define MTH_MODIFIER_PUSH_ALL      "$push_all"
   #define MTH_MODIFIER_PULL          "$pull"
   #define MTH_MODIFIER_PULL_BY       "$pull_by"
   #define MTH_MODIFIER_PULL_ALL      "$pull_all"
   #define MTH_MODIFIER_PULL_ALL_BY   "$pull_all_by"
   #define MTH_MODIFIER_POP           "$pop"
   #define MTH_MODIFIER_UNSET         "$unset"
   #define MTH_MODIFIER_BITNOT        "$bitnot"
   #define MTH_MODIFIER_BITXOR        "$bitxor"
   #define MTH_MODIFIER_BITAND        "$bitand"
   #define MTH_MODIFIER_BITOR         "$bitor"
   #define MTH_MODIFIER_BIT           "$bit"
   #define MTH_MODIFIER_ADDTOSET      "$addtoset"
   #define MTH_MODIFIER_RENAME        "$rename"
   #define MTH_MODIFIER_NULLOPR       "$null"
   #define MTH_MODIFIER_REPLACE       "$replace"
   #define MTH_MODIFIER_KEEP          "$keep"
   #define MTH_MODIFIER_SETARRAY      "$setarray"
   #define MTH_MODIFIER_CURRENTDATE   "$currentDate"

   /*
      Mod operator param key and value define
   */
   #define MTH_MOD_INC_VALUE           "Value"
   #define MTH_MOD_INC_DEFAULT         "Default"
   #define MTH_MOD_INC_MIN             "Min"
   #define MTH_MOD_INC_MAX             "Max"

   #define MTH_MOD_CURDATE_TYPE        "$type"

   /*
      typedef
   */
   #define MTH_SUB_FIELD_STATIC_LEN                ( 127 )
   typedef _utilString<MTH_SUB_FIELD_STATIC_LEN>   MTH_SUBFIELD_STR ;

}

#endif //MTHMODIFIER_DEF_HPP_

