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

   Source File Name = keyString.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft
          07/20/2022  ZHY  Implement
   Last Changed =

*******************************************************************************/
#ifndef VESSEL_KEY_STRING_H_
#define VESSEL_KEY_STRING_H_

#include "../bson/bsonobj.h"
#include "../bson/bsonobjbuilder.h"
#include "../bson/bsonobjiterator.h"
#include "oss.hpp"
#include "ossMemPool.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/slice.h"
#include "../bson/bsonobj.h"
#include "vessel/keyStringDef.h"
#include "vessel/recordID.h"
#include "vessel/bytesReader.h"

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
      typeBitsType readTimestampOrDate();
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
      friend class prefixedKeyString;

   public:
      keyString() = default;
      ~keyString();
      explicit keyString(const slice &s);
      explicit keyString(UINT32 size, const CHAR *data);

      /// WARNING: shallow copy!
      keyString(const keyString &);
      keyString &operator=(const keyString &);

      keyString(keyString &&) noexcept;
      keyString &operator=(keyString &&) noexcept;

   public:
      void reset();
      INT32 init(const slice &s);
      INT32 getOwned();

   public:
      OSS_INLINE BOOLEAN isValid()const {return _ref.isValid();}
      OSS_INLINE BOOLEAN isOwned() const {return nullptr != _bufferOwned;}
      OSS_INLINE const slice &getRawData() const {return _ref;}
      OSS_INLINE const slice &getDataSlice() const {return _ref;}
      OSS_INLINE const CHAR *getRawDataPtr()const {return _ref.data();}
      OSS_INLINE UINT32 getRawDataSize()const {return _ref.getSize();}
      OSS_INLINE UINT32 getComparableSize()const {return _desc.keySize;}
      OSS_INLINE UINT32 getKeySize()const {return _desc.keySize;}
      OSS_INLINE UINT32 getKeyHeadSize()const {return _desc.keyHeadSize;}
      OSS_INLINE UINT32 getKeyTailSize()const {return _desc.keyTailSize;}
      OSS_INLINE UINT32 getKeyElementsSize()const {return _desc.getKeyElementsSize();}
      OSS_INLINE UINT32 getKeySizeAfterHeader()const {return getKeySize() - getKeyHeadSize();}
      OSS_INLINE UINT32 getTypeBitsSize()const {return _desc.typeBitsSize;}
      OSS_INLINE UINT32 getKeySizeExceptTail()const {return _desc.keySize - _desc.keyTailSize;}
      OSS_INLINE BOOLEAN hasKeyHead()const {return 0 < getKeyHeadSize();}
      OSS_INLINE BOOLEAN hasKeyElements()const {return 0 < _desc.getKeyElementsSize();}
      OSS_INLINE BOOLEAN hasKeyTail()const {return 0 < getKeyTailSize();}
      OSS_INLINE BOOLEAN hasTypeBits()const {return 0 < getTypeBitsSize();}
      slice getKeySlice() const;
      slice getKeyHeadSlice() const;
      slice getKeyElementsSlice() const;
      slice getKeyTailSlice() const;
      slice getKeySliceExceptTail()const;
      slice getKeySliceAfterHeader()const;
      slice getTypeBits() const;
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           BOOLEAN withFieldName = FALSE) const;
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           bson::BufBuilder &builder,
                           BOOLEAN withFieldName = FALSE) const;

   public:
      recordID getRid() const;

   public:
      ///WARNING: same code format only!
      INT32 compare(const keyString &ks) const;

      INT32 compareElements(const keyString &ks)const;

      ///WARNING: same code format only!
      ///WARNING: will not do any validation inside!
      static INT32 compareCoding(UINT32 sizea,
                                 const CHAR *bufa,
                                 UINT32 sizeb,
                                 const CHAR *bufb);

      static INT32 parseMetaFromSlice(const slice &s, keyStringDescriptor &desc);

      class bodyReader : public SDBObject
      {
      public:
         bodyReader(const CHAR *bodyBuf,
                    UINT32 bufSize,
                    const CHAR *typeBitsBuf,
                    UINT32 typeBitsBufSize);

      public:
         bson::BSONObj toBSON(const bson::BSONObj &pattern,
                              BSONObjBuilder &bufBuilder,
                              BOOLEAN withFieldName = FALSE);
         void toBsonValue(EncodedType type,
                          BOOLEAN inverted,
                          bson::BSONObjBuilder &builder,
                          const CHAR *fieldName = nullptr);

      private:
         template <typename T> T _read(BOOLEAN inverted);
         template <typename T> T _peek(BOOLEAN inverted) const;
         void _readBytes(BOOLEAN inverted, CHAR *bytes, UINT32 len);
         void _readCString(BOOLEAN inverted, ossPoolString &s);
         void _toBSON(BOOLEAN inverted,
                      bson::BSONObjBuilder &builder,
                      const CHAR *fieldName = nullptr);
         bson::BSONObj _toBSON(BOOLEAN inverted, BOOLEAN withFieldName);

         void _toNumeric(EncodedType type,
                         BOOLEAN inverted,
                         bson::BSONObjBuilder &builder,
                         const CHAR *fieldName = nullptr);
         void _decodeStringLike(BOOLEAN inverted, ossPoolString &s);
         bson::bsonDecimal _decodeDecimal(BOOLEAN inverted, BOOLEAN isNegative);

      private:
         const CHAR *_buf = nullptr;
         UINT32 _bufSize = 0;
         UINT32 _offset = 0;
         typeBitsReader typeReader;
      };

   private:
      INT32 _parse(const slice &s, keyStringDescriptor &desc)const;
      //void _adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize);

   private:
      static BOOLEAN _loadSizeData(bytesReader &reader,
                            BOOLEAN nonzero,
                            UINT32 &size);

   protected:
      slice _ref;
      keyStringDescriptor _desc;

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
