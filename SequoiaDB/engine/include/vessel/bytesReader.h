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

   Source File Name = bytesReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
            return 0 <= _offset && (_offset + size) <= static_cast<INT64>(_bytes.getSize());
         }
      private:
         BOOLEAN _reverse = FALSE;
         slice _bytes;
         INT64 _offset = 0;
   };//class bytesReader


   template<class T>
   BOOLEAN bytesReader::isReadale()const
   {
      return _isOutOfBound(sizeof(T));
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