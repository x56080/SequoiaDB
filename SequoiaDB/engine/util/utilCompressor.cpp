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

   Source File Name = utilCompressor.cpp

   Descriptive Name = Compressor for data compression and decompression.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCompressorSnappy.hpp"
#include "utilCompressorLZW.hpp"
#include "utilCompressorLZ4.hpp"
#include "utilCompressorZlib.hpp"
#include "msgDef.hpp"

namespace engine
{
   utilCompressor* getCompressorByType( UTIL_COMPRESSOR_TYPE type )
   {
      utilCompressor *compressor = NULL ;

      static utilCompressorSnappy snappyCompressor ;
      static utilCompressorLZW lzwCompressor ;
      static utilCompressorLZ4 lz4Compressor ;
      static utilCompressorZlib zlibCompressor ;
      switch ( type )
      {
         case UTIL_COMPRESSOR_LZW:
            compressor = &lzwCompressor ;
            break ;
         case UTIL_COMPRESSOR_SNAPPY:
            compressor = &snappyCompressor ;
            break;
         case UTIL_COMPRESSOR_LZ4:
            compressor = &lz4Compressor ;
            break;
         case UTIL_COMPRESSOR_ZLIB:
            compressor = &zlibCompressor ;
            break;
         default:
            compressor = NULL ;
      }

      return compressor ;
   }

   const CHAR *utilCompressType2String( UINT8 type )
   {
      switch( type )
      {
         case UTIL_COMPRESSOR_SNAPPY :
            return VALUE_NAME_SNAPPY ;
            break ;
         case UTIL_COMPRESSOR_LZW :
            return VALUE_NAME_LZW ;
            break ;
         default :
            return "Invalid" ;
      }
   }
} /* engine */

