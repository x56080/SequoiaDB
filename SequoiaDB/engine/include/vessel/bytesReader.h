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

   Source File Name = bytesReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BYTES_READER_H_
#define VESSEL_BYTES_READER_H_

#include "vessel/slice.h"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   class bytesReader : public SDBObject
   {
      public:
         bytesReader() = default;
         ~bytesReader() = default;
         bytesReader(const slice &bytes, BOOLEAN reverse=FALSE);
         bytesReader(const bytesReader &) = default;
         bytesReader &operator=(const bytesReader &) = default;
         bytesReader &operator++()
         {
            slide(1);
            return *this;
         }

      public:
         void init(const slice &bytes, BOOLEAN reverse=FALSE);
         void reset();

         template<class T>
         BOOLEAN isReadale()const;

         template<class T>
         const T &get()const;

         CHAR getChar()const {return get<CHAR>();}

         INT8 getINT8()const  {return get<INT8>();}

         UINT8 getUINT8()const {return get<UINT8>();}

         BOOLEAN slide(UINT32 size=1);

         BOOLEAN isOutOfBound()const {return _isOutOfBound(1);}
      private:
         OSS_INLINE BOOLEAN _isOutOfBound(UINT32 size)const
         {
            return 0 > _offset || (_offset + size) > static_cast<INT64>(_bytes.getSize());
         }
      private:
         BOOLEAN _reverse = FALSE;
         slice _bytes;
         INT64 _offset = 0;
   };//class bytesReader


   template<class T>
   BOOLEAN bytesReader::isReadale()const
   {
      return !_isOutOfBound(sizeof(T));
   }

   template<class T>
   const T &bytesReader::get()const
   {
      const T *ptr = nullptr;
      strictBuffer buffer(_bytes.getSize(), _bytes.getData());
      if (0 <= _offset)
      {
         ptr = buffer.getReadableObjPtr<T>(static_cast<UINT32>(_offset));
      }
      SDB_ASSERT(nullptr != ptr, "not readable");
      return *ptr;
   }
} // namespace vessel

} // namespace engine


#endif//VESSEL_BYTES_READER_H_