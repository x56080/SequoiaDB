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

   Source File Name = utilMath.hpp

   Descriptive Name = math utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  HAS Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILMATH_HPP__
#define UTILMATH_HPP__



#include "ossTypes.h"


namespace engine
{

   OSS_INLINE INT64 utilAbs( INT64 value )
   {
      return value >= 0 ? value : -value ;
   }

   OSS_INLINE INT32 utilAbs( INT32 value )
   {
      return value >= 0 ? value : -value ;
   }

   OSS_INLINE BOOLEAN utilCanConvertToINT32( INT64 value )
   {
      return value <= OSS_SINT32_MAX_LL && value >= OSS_SINT32_MIN_LL ;
   }

   //
   // overflow check in basic operation
   //
   BOOLEAN utilAddIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilSubIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilMulIsOverflow( INT64 l, INT64 r, INT64 result ) ;

   BOOLEAN utilDivIsOverflow( INT64 l, INT64 r ) ;

   FLOAT64 utilPercentage( INT64 x, INT64 y ) ;
}


#endif // UTILMATH_HPP__
