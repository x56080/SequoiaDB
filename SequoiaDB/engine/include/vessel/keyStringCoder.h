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

   Source File Name = keyStringCoder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_KEY_STRING_CODER_H_
#define VESSEL_KEY_STRING_CODER_H_

#include "vessel/keyStringDef.h"
#include "ossUtil.hpp"
#include "vessel/recordID.h"
#include "vessel/globalIndexID.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   struct keyStringCoder : public SDBObject
   {
      template <
          typename T,
          class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      void encodeUnsignedNative(const T &val, BOOLEAN invert, void *buf)
      {
         T *ptr = reinterpret_cast<T *>(buf);
         *ptr = invert ? (~ossNativeToBigEndian(val)) : ossNativeToBigEndian(val);
      }  

      template <
          typename T,
          class = typename std::enable_if<std::is_signed<T>::value>::type>
      void encodeSignedNative(const T &val, BOOLEAN invert, void *buf)
      {
         T *ptr = reinterpret_cast<T *>(buf);
         T tmp = val ^ std::numeric_limits<T>::min();
         *ptr = invert ? (~ossNativeToBigEndian(tmp)) : ossNativeToBigEndian(tmp);
      }  

      template <typename T,
                class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      T decodeToUnsignedNative(const void *buf, BOOLEAN inverted) const
      {
         T val = *reinterpret_cast<const T *>(buf);
         if (inverted)
         {
            val = ~val;
         }
         return ossBigEndianToNative(val);
      }

      template <typename T,
                class = typename std::enable_if<std::is_signed<T>::value>::type>
      T decodeToSignedNative(const void *buf, BOOLEAN inverted) const
      {
         T val = *reinterpret_cast<const T *>(buf);
         if (inverted)
         {
            val = ~val;
         }

         return ossBigEndianToNative(val) ^ std::numeric_limits<T>::min();
      }

      static constexpr UINT32 RID_ENCODING_SIZE = 6;/// 4bytes pid + 2bytes pos
      void encodeRid(const recordID &rid, void *buf);
      recordID decodeToRid(const void *buf) const;
      
      void encodeLSN(UINT64 lsn, void *buf);
      UINT64 decodeToLSN(const void *buf)const;

      static constexpr UINT32 INDEX_ID_ENCODEING_SIZE = 12;
      void encodeGlobalIndexId(const globalIndexID &id, BOOLEAN asUpperKey, void *buf);
      globalIndexID decodeToIndexId(const void *buf)const;
   };//struct keyStringCoder
} // namespace vessel

} // namespace engine


#endif//VESSEL_KEY_STRING_CODER_H_