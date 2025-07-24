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

   Source File Name = utilEndian.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== =========== ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossEndian.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

namespace engine
{

   void ossMemcpyFlipBits( void *dst, const void *src, size_t len )
   {
      const CHAR *input = static_cast<const CHAR *>( src ) ;
      CHAR *output = static_cast<CHAR *>( dst ) ;
      for ( UINT32 i = 0 ; i < len ; ++i )
      {
         *output ++ = ~( *input ++ ) ;
      }
   }

}



