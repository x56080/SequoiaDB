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

   Source File Name = utilKeyStringMetaByte.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_KEY_STRING_META_BYTE_HPP_
#define UTIL_KEY_STRING_META_BYTE_HPP_

#include "ossUtil.hpp"
#include <limits>

namespace engine
{

namespace keystring
{
   /*
      _keyStringMetaByte define
    */

#pragma pack(1)

   class _keyStringMetaByte
   {
      public:
      _keyStringMetaByte() = default ;

      explicit _keyStringMetaByte( UINT32 keyHeadSize,
                                   UINT32 keyTailSize,
                                   UINT32 typeBitsSize )
      : _b( 0 )
      {
         if ( 0 < keyHeadSize )
         {
            setHasKeyHead() ;
         }
         if ( 0 < keyTailSize )
         {
            setHasKeyTail() ;
         }
         if ( 0 < typeBitsSize )
         {
            setHasTypeBits() ;
         }
      }

      explicit _keyStringMetaByte( UINT8 b )
      : _b(b)
      {
      }

   public:
      enum FLAG : UINT8
      {
         HAS_KEY_HEAD = 0x01,
         HAS_KEY_TAIL = 0x02,
         HAS_TYPE_BITS = 0x04,
      } ;

   public:
      OSS_INLINE void init( UINT8 b )
      {
         _b = b ;
      }

      OSS_INLINE UINT8 getValue() const
      {
         return _b ;
      }

      OSS_INLINE void setHasKeyHead()
      {
         OSS_BIT_SET( _b, HAS_KEY_HEAD ) ;
      }

      OSS_INLINE BOOLEAN hasKeyHead() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_KEY_HEAD ) ;
      }

      OSS_INLINE void setHasKeyTail()
      {
         OSS_BIT_SET( _b, HAS_KEY_TAIL ) ;
      }

      OSS_INLINE BOOLEAN hasKeyTail() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_KEY_TAIL ) ;
      }

      OSS_INLINE void setHasTypeBits()
      {
         OSS_BIT_SET( _b, HAS_TYPE_BITS ) ;
      }

      OSS_INLINE BOOLEAN hasTypeBits() const
      {
         return 0 != OSS_BIT_TEST( _b, HAS_TYPE_BITS ) ;
      }

   private:
      UINT8 _b = 0 ;
   } ;

   static_assert( 1 == sizeof( _keyStringMetaByte ), "invalid size" ) ;
   typedef class _keyStringMetaByte keyStringMetaByte ;

#pragma pack()

}
}

#endif // UTIL_KEY_STRING_META_BYTE_HPP_
