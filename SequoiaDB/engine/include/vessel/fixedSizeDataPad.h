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

   Source File Name = fixedSizeDataPad.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FIXED_SIZE_DATA_PAD_H_
#define VESSEL_FIXED_SIZE_DATA_PAD_H_

#include "vessel/slice.h"
#include <initializer_list>

namespace engine
{
namespace vessel
{
   class fixedSizeDataPad : public SDBObject
   {
      public:
         fixedSizeDataPad() = default;
         ~fixedSizeDataPad() = default;

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 < _bufferSize;}

         ///WANRING: should be a valid pad if not reset it.
         INT32 init(UINT32 bufferSize, CHAR *buffer, BOOLEAN resetBuffer);
         void resetBuffer();
         void fini();
         INT32 push(const slice &row);
         INT32 pushRowFragments(std::initializer_list<slice> il);
         BOOLEAN isFreeToPush(UINT32 size)const;
         slice getRow(UINT32 pos)const;
         UINT32 getRowSize(UINT32 pos)const;
         INT32 overwrite(const fixedSizeDataPad &pad);
         OSS_INLINE UINT32 getRowCount()const
         {
            return isValid() ? _getCount() : 0;
         }
         OSS_INLINE UINT32 getBufferSize()const {return _bufferSize;}
         OSS_INLINE UINT32 getFreeSize()const
         {
            return isValid() ?
                   (_backOffset - _getFrontOffset()) : 0;
         }
         OSS_INLINE UINT32 getUnfreeSize()const
         {
            return _bufferSize - getFreeSize();
         }

         static UINT32 getSavingSize(UINT32 size);
         static constexpr UINT32 getMinBufferSize()
         {
            /// 4bytes counter + 4bytes tag + 1byte data
            return 9;
         }

         static UINT32 getRowCountFast(const CHAR *buf);

      private:
         struct _tag : public SDBObject
         {
            _tag() = default;
            ~_tag() = default;
            explicit _tag(UINT32 o, UINT32 s):
            offset(o),
            size(s){}

            UINT32 offset = 0;
            UINT32 size = 0;
         };//struct _tag

      public:
         static constexpr UINT32 getMinBufferSizeInit(UINT32 size)
         {
            return sizeof(UINT32) + sizeof(_tag) + size;
         }

      private:
         OSS_INLINE UINT32 _getFrontOffset()const
         {
            static_assert(sizeof(_tag) == 8, "must be 8");
            return sizeof(UINT32) + (_getCount() << 3);
         }

         OSS_INLINE UINT32 _getTagOffset(UINT32 pos)const
         {
            static_assert(sizeof(_tag) == 8, "must be 8");
            return sizeof(UINT32) + (pos << 3);
         }

         const _tag *_getTag(UINT32 pos)const;

         OSS_INLINE UINT32 _getCount()const {return *_getCounter();}

         OSS_INLINE UINT32 *_getCounter()
         {
            return reinterpret_cast<UINT32 *>(_buffer);
         }

         OSS_INLINE void _incCount() {++(*_getCounter());}
         OSS_INLINE void _resetCounter() {*_getCounter() = 0;}

         OSS_INLINE const UINT32 *_getCounter()const
         {
            return reinterpret_cast<const UINT32 *>(_buffer);
         }

      private:
         UINT32 _bufferSize = 0;
         CHAR *_buffer = nullptr;
         UINT32 _backOffset = 0;
   };//class fixedSizeDataPad
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_SIZE_DATA_PAD_H_