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

   Source File Name = keyString.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_H_
#define VESSEL_KEY_STRING_H_

#include "../bson/bsonobj.h"
#include "../bson/bsonobjbuilder.h"
#include "../bson/bsonobjiterator.h"
#include "vessel/orderingWrapper.h"
#include "vessel/slice.h"
#include "../bson/bsonobj.h"
#include "vessel/keyStringDef.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   UINT32 neededBytesNumForInteger(EncodedType type);

   class typeBitsReader : public SDBObject
   {
   public:
      typeBitsReader() = default;
      typeBitsReader(const CHAR *buf, UINT32 bufSize);
      ~typeBitsReader() = default;
      typeBitsReader operator=(const typeBitsReader &) = delete;
      typeBitsReader(const typeBitsReader &) = delete;

   public:
      typeBitsType readNumeric();
      typeBitsType readZero();
      typeBitsType readStringLike();
      void readBitsAndAssign(CHAR *dst, UINT32 bytesSize);
      UINT8 readByte();
      template <typename T> T read();

   private:
      UINT8 _readBit();

   private:
      const CHAR *_buf = nullptr;
      UINT32 _bufSize = 0;
      UINT32 _curBit = 0;
   };

   class keyString : public SDBObject
   {
      template <typename T> friend class keyStringBuilder;

   public:
      keyString() = default;
      ~keyString();
      explicit keyString(const slice &s);

      /// WARNING: shallow copy!
      keyString(const keyString &);
      keyString &operator=(const keyString &);

      keyString(keyString &&);
      keyString &operator=(keyString &&);

   public:
      OSS_INLINE BOOLEAN isValid() const
      {
         return _ref.isValid();
      }
      OSS_INLINE BOOLEAN isOwned() const
      {
         return nullptr != _bufferOwned;
      }

      OSS_INLINE const slice &getDataSlice() const
      {
         return _ref;
      }

   public:
      void reset();
      INT32 init(const slice &s);
      INT32 getOwned();

   public:
      slice getKeySlice() const;
      slice getSliceFromKeyTo(UINT32 bytesAfterKey) const;
      slice getSliceBeforeKey() const;
      slice getSliceAfterKey() const;
      slice getTypeBits() const;
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           BOOLEAN withFieldName = FALSE) const;
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           bson::BufBuilder &builder,
                           BOOLEAN withFieldName = FALSE) const;

   public:
      UINT32 getTotalSize()const {return _ref.getSize();}
      UINT32 getComparableSize() const;
      UINT32 getKeySize() const;
      UINT32 getSizeBeforeKey() const;
      UINT32 getSizeAfterKey() const;
      UINT32 getTypeBitsSize() const;

   public:
      recordID getRid()const;

   public:
      INT32 compare(const keyString &ks) const;

   private:
      BOOLEAN _validate(const slice &s) const;
      void _adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize);

   private:
      template <typename T> T _read(UINT32 &offset, BOOLEAN inverted) const;
      template <typename T> T _peek(UINT32 offset, BOOLEAN inverted) const;
      void _readBytes(UINT32 &offset,
                      BOOLEAN inverted,
                      CHAR *bytes,
                      UINT32 len) const;
      bson::StringData _readCString(UINT32 &offset, BOOLEAN inverted) const;
      void _toBSON(UINT32 &offset,
                            BOOLEAN inverted,
                            bson::BSONObjBuilder &builder,
                            typeBitsReader &typeReader,
                            const CHAR *fieldName = nullptr) const;
      bson::BSONObj _toBSON(UINT32 &offset,
                            BOOLEAN inverted,
                            typeBitsReader &typeReader,
                            BOOLEAN withFieldName) const;
      
      void _toBsonValue(EncodedType type,
                        UINT32 &offset,
                        BOOLEAN inverted,
                        bson::BSONObjBuilder &builder,
                        typeBitsReader &typeReader,
                        const CHAR *fieldName = nullptr) const;
      void _toNumeric(EncodedType type,
                      UINT32 &offset,
                      BOOLEAN inverted,
                      bson::BSONObjBuilder &builder,
                      typeBitsReader &typeReader,
                      const CHAR *fieldName = nullptr) const;
      bson::StringData _decodeStringLike(UINT32 &offset, BOOLEAN inverted)const;
      bson::bsonDecimal _decodeDecimal(UINT32 &offset,
                                       BOOLEAN inverted,
                                       BOOLEAN isNegative,
                                       typeBitsReader &typeReader)const;

   protected:
      slice _ref;

   private:
      CHAR *_bufferOwned = nullptr;
      UINT32 _bufferSize = 0;
   }; // class keyString

   template <typename T> T typeBitsReader::read()
   {
      T t;
      UINT8 *ptr = reinterpret_cast<UINT8 *>(&t);
      for (UINT32 i = 0; i < sizeof(t); i++)
      {
         UINT8 byte = readByte();
         ossMemcpy(ptr + i, &byte, 1);
      }
      return t;
   }

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_H_
