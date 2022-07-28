/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = keyStringMetaBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_META_BLOCK_H_
#define VESSEL_KEY_STRING_META_BLOCK_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{ 
   template<UINT32 N>
   class keyStringMetaBlock
   {
      ~keyStringMetaBlock() = delete;
   };

   constexpr UINT32 KEY_STRING_META_BLOCK_MIN_SIZE = 4;

#pragma pack(4)
   template<>
   class keyStringMetaBlock<0>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return 0;}
         OSS_INLINE UINT32 getAfterKeySize()const {return 0;}

      private:
         UINT8 __;
      public:
         UINT8 keySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(4 == sizeof(keyStringMetaBlock<0>), "invalid size");

   template<>
   class keyStringMetaBlock<1>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return beforeKeySize;}
         OSS_INLINE UINT32 getAfterKeySize()const {return 0;}

      public:
         UINT8 keySize;
         UINT8 beforeKeySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(4 == sizeof(keyStringMetaBlock<1>), "invalid size");

#pragma pack()

#pragma pack(1)
   template<>
   class keyStringMetaBlock<2>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return 0;}
         OSS_INLINE UINT32 getAfterKeySize()const {return 0;}

      public:
         UINT32 keySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(6 == sizeof(keyStringMetaBlock<2>), "invalid size");

   template<>
   class keyStringMetaBlock<3>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return beforeKeySize;}
         OSS_INLINE UINT32 getAfterKeySize()const {return 0;}

      public:
         UINT32 keySize;
         UINT8 beforeKeySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(7 == sizeof(keyStringMetaBlock<3>), "invalid size");
#pragma pack()


#pragma pack(4)
   template<>
   class keyStringMetaBlock<4>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return 0;}
         OSS_INLINE UINT32 getAfterKeySize()const {return afterKeySize;}

      public:
         UINT8 afterKeySize;
         UINT8 keySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(4 == sizeof(keyStringMetaBlock<4>), "invalid size");
#pragma pack()

#pragma pack(1)
   template<>
   class keyStringMetaBlock<5>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return beforeKeySize;}
         OSS_INLINE UINT32 getAfterKeySize()const {return afterKeySize;}

      public:
         UINT8 afterKeySize;
         UINT8 keySize;
         UINT8 beforeKeySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(5 == sizeof(keyStringMetaBlock<5>), "invalid size");
#pragma pack()

#pragma pack(1)
   template<>
   class keyStringMetaBlock<6>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return 0;}
         OSS_INLINE UINT32 getAfterKeySize()const {return afterKeySize;}


      public:
         UINT8 afterKeySize;
         UINT32 keySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(7 == sizeof(keyStringMetaBlock<6>), "invalid size");
#pragma pack()

#pragma pack(1)
   template<>
   class keyStringMetaBlock<7>
   {
      public:
         ~keyStringMetaBlock() = delete;

      public:
         OSS_INLINE UINT32 getKeySize()const {return keySize;}
         OSS_INLINE UINT32 getBeforeKeySize()const {return beforeKeySize;}
         OSS_INLINE UINT32 getAfterKeySize()const {return afterKeySize;}

      public:
         UINT8 afterKeySize;
         UINT32 keySize;
         UINT8 beforeKeySize;
         UINT8 metaByte;
         UINT8 version; 
   };
   static_assert(8 == sizeof(keyStringMetaBlock<7>), "invalid size");
#pragma pack()

   extern UINT32 GET_META_BLOCK_SIZE(UINT8 metabyte);

} // namespace vessel

} // namespace engine


#endif//VESSEL_KEY_STRING_META_BLOCK_H_