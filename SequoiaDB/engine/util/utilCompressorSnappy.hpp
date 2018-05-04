/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilCompressorSnappy.hpp

   Descriptive Name = Snappy compressor wrapper.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/01/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSOR_SNAPPY__
#define UTIL_COMPRESSOR_SNAPPY__

#include "utilCompressor.hpp"
#include "snappy.h"

namespace engine
{
   class _utilCompressorSnappy : public utilCompressor
   {
   public:
      _utilCompressorSnappy() ;

      INT32 compressBound( UINT32 srcLen,
                           UINT32 &maxCompressedLen,
                           const utilDictHandle dictionary = NULL ) ;

      INT32 compress( const CHAR *source, UINT32 sourceLen,
                      CHAR *dest, UINT32 &destLen,
                      const utilDictHandle dictionary = NULL,
                      const utilCompressStrategy *strategy = NULL ) ;

      INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                UINT32 &length) ;

      INT32 decompress( const CHAR *source, UINT32 sourceLen,
                        CHAR *dest, UINT32 &destLen,
                        const utilDictHandle dictionary = NULL ) ;
   } ;
   typedef _utilCompressorSnappy utilCompressorSnappy ;

}

#endif /*¡¡UTIL_COMPRESSOR_SNAPPY__ */

