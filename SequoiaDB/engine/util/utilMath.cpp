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

   Source File Name = utilMath.cpp

   Descriptive Name = math utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  HAS Initial Draft
          05/04/2018  HGM Add license

   Last Changed =

*******************************************************************************/
#include "utilMath.hpp"

namespace engine
{

   BOOLEAN utilAddIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l > 0 && r > 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r < 0 && result >= 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilSubIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l >= 0 && r < 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r > 0 && result > 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilMulIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if ( l != (INT64) ((INT32) l) || r != (INT64) ((INT32) r) )
      {
         if ( r != 0 &&
              ( ( r == -1 && l < 0 && result < 0 ) ||
                result / r != l ) )
         {
            ret = TRUE ;
         }
      }
      return ret ;
   }

   BOOLEAN utilDivIsOverflow( INT64 l, INT64 r )
   {
      BOOLEAN ret = FALSE ;
      if ( OSS_SINT64_MIN == l && -1 == r )
      {
         ret = TRUE ;
      }
      return ret ;
   }

   FLOAT64 utilPercentage( INT64 x, INT64 y )
   {
      INT32 z = 0 ;
      if ( 0 == y ) {
         return z ;
      }
      z = (INT32)( x / (FLOAT64)y * 100.0 ) ;
      return z / 100.0 ;
   }
}
