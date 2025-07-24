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

   Source File Name = utilCompressorLZ4.hpp

   Descriptive Name = LZ4 Compressor for data compression and decompression.

   When/how to use: this program may be used to compress/decompress data. This
   file contains the interfaces provided by compressors.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSOR_LZ4__
#define UTIL_COMPRESSOR_LZ4__

#include "utilCompressor.hpp"
#include "lz4.h"

namespace engine
{
   class _utilCompressorLZ4 : public utilCompressor
   {
   public:
      _utilCompressorLZ4() ;
      ~_utilCompressorLZ4() ;

   public:
      INT32 compressBound( UINT32 srcLen, UINT32 &maxCompressedLen,
                           const utilDictHandle dictionary = NULL ) ;

      INT32 compress( const CHAR *source, UINT32 sourceLen,
                      CHAR *dest, UINT32 &destLen,
                      const utilDictHandle dictionary = NULL,
                      const utilCompressStrategy *strategy = NULL ) ;

      INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                UINT32 &length ) ;

      INT32 decompress( const CHAR *source, UINT32 sourceLen,
                        CHAR *dest, UINT32 &destLen,
                        const utilDictHandle dictionary = NULL ) ;
   };
   typedef _utilCompressorLZ4 utilCompressorLZ4 ;
}

#endif /* UTIL_COMPRESSOR_LZ4__ */

