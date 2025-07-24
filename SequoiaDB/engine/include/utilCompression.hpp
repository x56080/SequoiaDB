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

   Source File Name = utilCompression.hpp

   Descriptive Name = util compression definitions

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/13/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSION_HPP_
#define UTIL_COMPRESSION_HPP_

namespace engine
{
   enum UTIL_COMPRESSOR_TYPE
   {
      UTIL_COMPRESSOR_SNAPPY  = 0,
      UTIL_COMPRESSOR_LZW     = 1,
      UTIL_COMPRESSOR_LZ4     = 2,
      UTIL_COMPRESSOR_ZLIB    = 3,

      UTIL_COMPRESSOR_INVALID = 255
   } ;

   enum UTIL_COMPRESSION_LEVEL
   {
      UTIL_COMP_BEST_SPEED          = 1,
      UTIL_COMP_BALANCE             = 2,
      UTIL_COMP_BEST_COMPRESSION    = 3,
   } ;

   #define UTIL_COMPRESS_EMPTY_FLAG       ( 0x00 )
   #define UTIL_COMPRESS_ALTERABLE_FLAG   ( 0x01 )

}

#endif /* UTIL_COMPRESSION_HPP_ */
