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

   Source File Name = utilCompressor.hpp

   Descriptive Name = Compressor for data compression and decompression.

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
#ifndef UTIL_COMPRESSOR__
#define UTIL_COMPRESSOR__

#include "core.hpp"
#include "oss.hpp"
#include "utilCompression.hpp"
#include "utilDictionary.hpp"

namespace engine
{

   #define UTIL_INVALID_DICT                 NULL

   typedef void * utilDictHandle ;

   #define UTIL_COMPRESSOR_DFT_MIN_RATIO     80
   #define UTIL_COMPRESSOR_DFT_LEVEL         UTIL_COMP_BEST_COMPRESSION

   struct _utilCompressStrategy
   {
      UINT8 _minRatio ;
      UTIL_COMPRESSION_LEVEL _level ;
   } ;
   typedef _utilCompressStrategy utilCompressStrategy ;

   /* This class provides compressor interfaces. */
   class _utilCompressor : public SDBObject
   {
   public:
      _utilCompressor( UTIL_COMPRESSOR_TYPE type )
      : _type( type )
      {
      }

      virtual ~_utilCompressor() {}

      virtual INT32 compressBound( UINT32 srcLen, UINT32 &maxCompressedLen,
                                   const utilDictHandle dictionary = NULL ) = 0 ;

      virtual INT32 compress( const CHAR *source, UINT32 sourceLen,
                              CHAR *dest, UINT32 &destLen,
                              const utilDictHandle dictionary = NULL,
                              const utilCompressStrategy *strategy = NULL ) = 0 ;

      virtual INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                        UINT32 &length) = 0 ;

      virtual INT32 decompress( const CHAR *source, UINT32 sourceLen,
                                CHAR *dest, UINT32 &destLen,
                                const utilDictHandle dictionary = NULL ) = 0 ;

      OSS_INLINE UTIL_COMPRESSOR_TYPE getType () const
      {
         return _type ;
      }

      virtual OSS_INLINE BOOLEAN needDictionay () const
      {
         return FALSE ;
      }

   private:
      UTIL_COMPRESSOR_TYPE _type ;
   } ;
   typedef _utilCompressor utilCompressor ;

   /*
    * Get the global compressor pointer. When that is done, you can use the
    * compressor to compress/decompress data.
    */
   utilCompressor* getCompressorByType( UTIL_COMPRESSOR_TYPE type ) ;

   /* Get the name of the compressor in string format. */
   const CHAR *utilCompressType2String( UINT8 type ) ;

   UTIL_COMPRESSOR_TYPE utilString2CompressType( const CHAR *pStr ) ;

}

#endif /* UTIL_COMPRESSOR__ */

