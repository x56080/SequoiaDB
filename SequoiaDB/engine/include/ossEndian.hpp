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

   Source File Name = ossEndian.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_ENDIAN_HPP_
#define OSS_ENDIAN_HPP_

#include "ossUtil.hpp"
#include <type_traits>

namespace engine
{

   template <
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   T ossNativeToBigEndian( T in )
   {
   #ifdef SDB_BIG_ENDIAN
      return in ;
   #else
      T out ;
      ossEndianConvertIf( in, out, TRUE ) ;
      return out ;
   #endif
   }

   template <
      typename T,
      typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   T ossBigEndianToNative( T in )
   {
   #ifdef SDB_BIG_ENDIAN
      return in ;
   #else
      T out ;
      ossEndianConvertIf( in, out, TRUE ) ;
      return out ;
   #endif
   }

   void ossMemcpyFlipBits( void *dst, const void *src, size_t len ) ;

}

#endif // OSS_ENDIAN_HPP_

