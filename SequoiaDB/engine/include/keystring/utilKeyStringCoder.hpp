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

   Source File Name = utilKeyStringCoder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_KEY_STRING_CODER_HPP_
#define UTIL_KEY_STRING_CODER_HPP_

#include "utilKeyStringDef.hpp"
#include "ossUtil.hpp"
#include "ossEndian.hpp"
#include "dms.hpp"
#include "dmsMetadata.hpp"

namespace engine
{
namespace keystring
{

   /*
      _keyStringCoder define
    */
   class _keyStringCoder
   {
   public:
      template<typename T,
               class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      void encodeUnsignedNative( const T &val, BOOLEAN invert, void *buf )
      {
         T *ptr = reinterpret_cast<T *>( buf ) ;
         *ptr = invert ?
                      ( ~ossNativeToBigEndian( val ) ) :
                      ( ossNativeToBigEndian( val ) ) ;
      }

      template<typename T,
               class = typename std::enable_if<std::is_signed<T>::value>::type>
      void encodeSignedNative( const T &val, BOOLEAN invert, void *buf )
      {
         T *ptr = reinterpret_cast<T *>( buf ) ;
         T tmp = val ^ std::numeric_limits<T>::min() ;
         *ptr = invert ?
                      ( ~ossNativeToBigEndian( tmp ) ) :
                      ( ossNativeToBigEndian( tmp ) ) ;
      }

      template<typename T,
               class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      T decodeToUnsignedNative( const void *buf, BOOLEAN inverted ) const
      {
         T val = *reinterpret_cast<const T *>( buf ) ;
         if ( inverted )
         {
            val = ~val ;
         }
         return ossBigEndianToNative( val ) ;
      }

      template<typename T,
               class = typename std::enable_if<std::is_signed<T>::value>::type>
      T decodeToSignedNative( const void *buf, BOOLEAN inverted ) const
      {
         T val = *reinterpret_cast<const T *>( buf ) ;
         if ( inverted )
         {
            val = ~val ;
         }

         return ossBigEndianToNative( val ) ^ std::numeric_limits< T > ::min() ;
      }

      /// 4 bytes extent ID + 4 bytes offset
      static constexpr UINT32 RID_ENCODING_SIZE = 8 ;
      void encodeRID( const dmsRecordID &rid, void *buf ) ;
      dmsRecordID decodeToRID( const void *buf ) const ;

      void encodeLSN( UINT64 lsn, void *buf ) ;
      UINT64 decodeToLSN( const void *buf ) const ;

      /// 8 bytes CL UID + 4 bytes CL LID + 4 bytes index inner ID
      static constexpr UINT32 INDEX_ID_ENCODEING_SIZE = 16 ;
      void encodeIndexID( const dmsIdxMetadataKey &id,
                          BOOLEAN asUpperKey,
                          void *buf ) ;
      dmsIdxMetadataKey decodeToIndexID( const void *buf ) const ;
   } ;

   typedef class _keyStringCoder keyStringCoder ;

}
}

#endif //UTIL_KEY_STRING_CODER_HPP_
