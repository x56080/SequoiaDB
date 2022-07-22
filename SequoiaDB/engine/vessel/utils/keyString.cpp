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

   Source File Name = keyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/keyString.h"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilAllocator.hpp"
#include "ossLikely.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace engine
{
namespace vessel
{
   constexpr FLOAT64 invPow256[] = {
       1.0,                                            // 2**0
       1.0 / 256,                                      // 2**(-8)
       1.0 / 256 / 256,                                // 2**(-16)
       1.0 / 256 / 256 / 256,                          // 2**(-24)
       1.0 / 256 / 256 / 256 / 256,                    // 2**(-32)
       1.0 / 256 / 256 / 256 / 256 / 256,              // 2**(-40)
       1.0 / 256 / 256 / 256 / 256 / 256 / 256,        // 2**(-48)
       1.0 / 256 / 256 / 256 / 256 / 256 / 256 / 256}; // 2**(-56)

   UINT32 neededBytesNumForInteger(EncodedType type)
   {
      if (type <= EncodedType::numericNegative1ByteInt)
      {
         SDB_ASSERT(type >= EncodedType::numericNegative8ByteInt,
                    "Unexpected encoded type");
         return static_cast<UINT32>(
             static_cast<UINT8>(EncodedType::numericNegative1ByteInt) -
             static_cast<UINT8>(type) + 1);
      }
      SDB_ASSERT(type >= EncodedType::numericPositive1ByteInt,
                 "Unexpected encoded type");
      SDB_ASSERT(type <= EncodedType::numericPositive8ByteInt,
                 "Unexpected encoded type");
      return static_cast<UINT32>(
          static_cast<UINT8>(type) -
          static_cast<UINT8>(EncodedType::numericPositive1ByteInt) + 1);
   }

   keyString::~keyString()
   {
      if (nullptr != _bufferOwned)
      {
         utilPoolAllocator allocator;
         allocator.free(_bufferOwned);
      }
   }

   keyString::keyString(const slice &s) : _ref(s)
   {
      SDB_ASSERT(_ref.isValid(), "can not be invalid");
      _block.init(s);
      if (!_block.isValid())
      {
         reset();
      }
      SDB_ASSERT(_block.isValid(), "can not be invalid");
   }

   keyString::keyString(const keyString &o) : _ref(o._ref), _block(o._block)
   {
   }

   keyString &keyString::operator=(const keyString &o)
   {
      reset();
      _ref = o._ref;
      _block = o._block;
      return *this;
   }

   keyString::keyString(keyString &&o)
   {
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _block = o._block;
         o._bufferOwned = nullptr;
      }
      o.reset();
   }

   keyString &keyString::operator=(keyString &&o)
   {
      reset();
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _block = o._block;
         o._bufferOwned = nullptr;
      }
      o.reset();
      return *this;
   }

   void keyString::reset()
   {
      _ref.reset();
      if (nullptr != _bufferOwned)
      {
         utilPoolAllocator allocator;
         allocator.free(_bufferOwned);
         _bufferOwned = nullptr;
      }
      _bufferSize = 0;
      _block.reset();
      return;
   }

   INT32 keyString::init(const slice &s)
   {
      INT32 rc = SDB_OK;

      if (!s.isValid())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid slice");
         goto error;
      }

      rc = _block.init(s);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "init key string meta block failed, rc:%d", rc);
         goto error;
      }

      _ref = s;

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 keyString::getOwned()
   {
      INT32 rc = SDB_OK;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isOwned())
      {
         utilPoolAllocator allocator;
         _bufferOwned = (CHAR *)allocator.malloc(_ref.getSize());
         if (OSS_UNLIKELY(nullptr == _bufferOwned))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         _bufferSize = _ref.getSize();
         ossMemcpy(_bufferOwned, _ref.data(), _ref.getSize());
         _ref.reset(_ref.getSize(), _bufferOwned);
      }
   done:
      return rc;
   error:
      goto done;
   }

   void keyString::_adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize)
   {
      SDB_ASSERT(nullptr != buffer && 0 < bufferSize, "can not be invalid");
      SDB_ASSERT(0 < ksSize && ksSize <= bufferSize, "invalid key string size");
      reset();
      _bufferOwned = buffer;
      _bufferSize = bufferSize;
      _ref.reset(ksSize, _bufferOwned);
      _block.init(_ref);
      SDB_ASSERT(_block.isValid(), "can not be invalid");
      return;
   }

   slice keyString::getKeySlice() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 != _block.keySize, "can not be zero");
      const CHAR *buf = _ref.getData() + _block.sizeBeforeKey;
      return slice(_block.keySize, buf);
   }

   slice keyString::getSliceFromKeyTo(UINT32 bytesAfterKey) const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 != _block.keySize, "can not be zero");
      SDB_ASSERT(bytesAfterKey < (_ref.getSize() - _block.sizeBeforeKey),
                 "invalid bytes");
      const CHAR *buf = _ref.getData() + _block.sizeBeforeKey;
      return slice(_block.keySize + bytesAfterKey, buf);
   }

   slice keyString::getSliceBeforeKey() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 != _block.sizeBeforeKey, "can not be zero");
      return slice(_block.sizeBeforeKey, _ref.getData());
   }

   slice keyString::getSliceAfterKey() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 != _block.keySize && 0 != _block.sizeAfterKey,
                 "can not be zero");
      const CHAR *buf = _ref.getData() + _block.sizeBeforeKey + _block.keySize;
      return slice(_block.keySize, buf);
   }

   slice keyString::getTypeBits() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(0 != _block.keySize && 0 != _block.typeBitsSize,
                 "can not be zero");
      const CHAR *buf = _ref.getData() + _block.sizeBeforeKey + _block.keySize +
                        _block.sizeAfterKey;
      return slice(_block.typeBitsSize, buf);
   }

   INT32 keyString::compare(const keyString &s) const
   {
      SDB_ASSERT(isValid() && s.isValid(), "can not be invalid");
      UINT32 cmpSize = _ref.getSize() < s.getDataSlice().getSize()
                           ? _ref.getSize()
                           : s.getDataSlice().getSize();
      return ossMemcmp(_ref.getData(), s.getDataSlice().getData(), cmpSize);
   }

   template <typename T> T keyString::_read(UINT32 &offset, BOOLEAN inverted)
   {
      UINT32 size = sizeof(T);
      T t;
      if (inverted)
      {
         ossMemcpyFlipBits(&t, getDataSlice().data() + offset, size);
      }
      else
      {
         memcpy(&t, getDataSlice().data() + offset, size);
      }
      offset += size;
      return t;
   }

   template <typename T>
   T keyString::_peek(UINT32 offset, BOOLEAN inverted) const
   {
      UINT32 size = sizeof(T);
      T t;
      if (inverted)
      {
         ossMemcpyFlipBits(&t, getDataSlice().data() + offset, size);
      }
      else
      {
         memcpy(&t, getDataSlice().data() + offset, size);
      }
      return t;
   }

   bson::BSONObj keyString::toBSON(const bson::BSONObj &pattern,
                                   BOOLEAN withFieldName)
   {
      bson::BSONObjBuilder builder;
      UINT32 keySize = getKeySlice().getSize();
      typeBitsReader typeReader(getTypeBits().data(), getTypeBits().getSize());
      UINT32 offset = _block.sizeBeforeKey;
      bson::BSONObjIterator it(pattern);
      while (offset - _block.sizeBeforeKey < keySize && it.more())
      {
         bson::BSONElement ele = it.next();
         BOOLEAN inverted = ele.numberInt() == -1 ? TRUE : FALSE;
         EncodedType type =
             static_cast<EncodedType>(_read<UINT8>(offset, inverted));
         _toBsonValue(type,
                      offset,
                      inverted,
                      builder,
                      typeReader,
                      withFieldName ? ele.fieldName() : nullptr);
      }
      SDB_ASSERT(1 ==  _block.sizeBeforeKey + keySize - offset ||
                     2 == _block.sizeBeforeKey + keySize - offset,
                 "Unexpected size");
      return builder.obj();
   }

   bson::BSONObj keyString::toBSON(const bson::BSONObj &pattern,
                                   bson::BSONObjBuilder &builder,
                                   BOOLEAN withFieldName)
   {
      UINT32 keySize = getKeySlice().getSize();
      typeBitsReader typeReader(getTypeBits().data(), getTypeBits().getSize());
      UINT32 offset = _block.sizeBeforeKey;
      bson::BSONObjIterator it(pattern);
      while (offset - _block.sizeBeforeKey < keySize && it.more())
      {
         bson::BSONElement ele = it.next();
         BOOLEAN inverted = ele.numberInt() == -1 ? TRUE : FALSE;
         EncodedType type =
             static_cast<EncodedType>(_read<UINT8>(offset, inverted));
         _toBsonValue(type,
                      offset,
                      inverted,
                      builder,
                      typeReader,
                      withFieldName ? ele.fieldName() : nullptr);
      }
      SDB_ASSERT(1 == _block.sizeBeforeKey + keySize - offset ||
                     2 == _block.sizeBeforeKey + keySize - offset,
                 "Unexpected size");
      builder.doneFast();
      return builder.done();
   }

   void keyString::_toBSON(UINT32 &offset,
                           BOOLEAN inverted,
                           bson::BSONObjBuilder &builder,
                           typeBitsReader &typeReader,
                           const CHAR *fieldName)
   {
      bson::BSONObjBuilder newObjBuilder;
      while (0 != static_cast<UINT8>(_read<EncodedType>(offset, inverted)))
      {
         bson::StringData name = _readCString(offset, inverted);
         _toBsonValue(_read<EncodedType>(offset, inverted),
                      offset,
                      inverted,
                      newObjBuilder,
                      typeReader,
                      name.data());
      }
      fieldName
          ? builder.appendObject(fieldName, newObjBuilder.done().objdata())
          : builder.appendObject("", newObjBuilder.done().objdata());
   }

   bson::BSONObj keyString::_toBSON(UINT32 &offset,
                                    BOOLEAN inverted,
                                    typeBitsReader &typeReader,
                                    BOOLEAN withFieldName)
   {
      bson::BSONObjBuilder newObjBuilder;
      while (0 != static_cast<UINT8>(_read<EncodedType>(offset, inverted)))
      {
         bson::StringData name = _readCString(offset, inverted);
         _toBsonValue(_read<EncodedType>(offset, inverted),
                      offset,
                      inverted,
                      newObjBuilder,
                      typeReader,
                      withFieldName ? name.data() : "");
      }
      return newObjBuilder.obj();
   }

   void keyString::_toBsonValue(EncodedType type,
                                UINT32 &offset,
                                BOOLEAN inverted,
                                bson::BSONObjBuilder &builder,
                                typeBitsReader &typeReader,
                                const CHAR *fieldName)
   {
      switch (type)
      {
      case EncodedType::minKey:
         fieldName ? builder.appendMinKey(fieldName) : builder.appendMinKey("");
         break;
      case EncodedType::undefined:
         fieldName ? builder.appendUndefined(fieldName)
                   : builder.appendUndefined("");
         break;
      case EncodedType::nullish:
         fieldName ? builder.appendNull(fieldName) : builder.appendNull("");
         break;
      case EncodedType::numericNaN:
      case EncodedType::numericNegativeLargeMagnitude:
      case EncodedType::numericNegative8ByteInt:
      case EncodedType::numericNegative7ByteInt:
      case EncodedType::numericNegative6ByteInt:
      case EncodedType::numericNegative5ByteInt:
      case EncodedType::numericNegative4ByteInt:
      case EncodedType::numericNegative3ByteInt:
      case EncodedType::numericNegative2ByteInt:
      case EncodedType::numericNegative1ByteInt:
      case EncodedType::numericNegativeSmallMagnitude:
      case EncodedType::numericZero:
      case EncodedType::numericPositiveSmallMagnitude:
      case EncodedType::numericPositive1ByteInt:
      case EncodedType::numericPositive2ByteInt:
      case EncodedType::numericPositive3ByteInt:
      case EncodedType::numericPositive4ByteInt:
      case EncodedType::numericPositive5ByteInt:
      case EncodedType::numericPositive6ByteInt:
      case EncodedType::numericPositive7ByteInt:
      case EncodedType::numericPositive8ByteInt:
      case EncodedType::numericPositiveLargeMagnitude:
         _toNumeric(type, offset, inverted, builder, typeReader, fieldName);
         break;
      case EncodedType::stringLike: {
         bson::StringData s = _decodeStringLike(offset, inverted);
         typeBitsType originalType = typeReader.readStringLike();
         if (typeBitsType::STRING == originalType)
         {
            fieldName
                ? builder.appendStrWithNoTerminating(
                      fieldName, s.data(), s.size())
                : builder.appendStrWithNoTerminating("", s.data(), s.size());
         }
         else
         {
            fieldName ? builder.appendSymbol(fieldName, s)
                      : builder.appendSymbol("", s);
         }
         break;
      }
      case EncodedType::object: {
         _toBSON(offset, inverted, builder, typeReader, fieldName);
         break;
      }
      case EncodedType::array: {
         bson::BSONObjBuilder arrayBuilder;
         while (0 != static_cast<UINT8>(_read<EncodedType>(offset, inverted)))
         {
            _toBsonValue(_read<EncodedType>(offset, inverted),
                         offset,
                         inverted,
                         arrayBuilder,
                         typeReader,
                         nullptr);
         }
         builder.appendArray(fieldName, arrayBuilder.done());
         break;
      }

      case EncodedType::binData: {
         UINT8 firstByteLen = _read<UINT8>(offset, inverted);
         UINT32 binDataLen = 0;
         if (0xFF == firstByteLen)
         {
            binDataLen = ossBigEndianToNative(_read<UINT32>(offset, inverted));
         }
         else
         {
            binDataLen = firstByteLen;
         }
         bson::BinDataType binDataType =
             _read<bson::BinDataType>(offset, inverted);
         CHAR data[binDataLen];
         _readBytes(offset, inverted, data, binDataLen);
         fieldName
             ? builder.appendBinData(fieldName, binDataLen, binDataType, data)
             : builder.appendBinData("", binDataLen, binDataType, data);
         break;
      }
      case EncodedType::oid: {
         bson::OID oid = _read<bson::OID>(offset, inverted);
         fieldName ? builder.appendOID(fieldName, &oid)
                   : builder.appendOID("", &oid);
         offset += sizeof(oid);
         break;
      }
      case EncodedType::booleanFalse: {
         fieldName ? builder.appendBool(fieldName, FALSE)
                   : builder.appendBool("", FALSE);
         break;
      }
      case EncodedType::booleanTrue: {
         fieldName ? builder.appendBool(fieldName, TRUE)
                   : builder.appendBool("", TRUE);
         break;
      }
      case EncodedType::date: {
         INT64 encoded = ossBigEndianToNative(_read<INT64>(offset, inverted));
         encoded ^= (1ULL << 63);
         bson::Date_t dt(encoded);
         fieldName ? builder.appendDate(fieldName, dt)
                   : builder.appendDate("", dt);
         break;
      }
      case EncodedType::timestamp: {
         INT64 encoded = ossBigEndianToNative(_read<INT64>(offset, inverted));
         fieldName ? builder.appendTimestamp(fieldName, encoded)
                   : builder.appendTimestamp("", encoded);
         break;
      }
      case EncodedType::regEx: {
         bson::StringData regex = _readCString(offset, inverted);
         bson::StringData flags = _readCString(offset, inverted);
         fieldName ? builder.appendRegex(fieldName, regex, flags)
                   : builder.appendRegex("", regex, flags);
         break;
      }
      case EncodedType::dbRef: {
         UINT32 nsLen = ossBigEndianToNative(_read<UINT32>(offset, inverted));
         CHAR nsData[nsLen];
         _readBytes(offset, inverted, nsData, nsLen);
         bson::OID oid = _read<bson::OID>(offset, inverted);
         fieldName ? builder.appendDBRef(fieldName, nsData, oid)
                   : builder.appendDBRef("", nsData, oid);
         break;
      }

      case EncodedType::code: {
         EncodedType type = _read<EncodedType>(offset, inverted);
         SDB_ASSERT(EncodedType::stringLike == type, "Unexpected encoded type");
         bson::StringData code = _decodeStringLike(offset, inverted);
         fieldName ? builder.appendCode(fieldName, code)
                   : builder.appendCode("", code);
         break;
      }
      case EncodedType::codeWithScope: {
         EncodedType type = _read<EncodedType>(offset, inverted);
         SDB_ASSERT(EncodedType::stringLike == type, "Unexpected encoded type");
         bson::StringData code = _decodeStringLike(offset, inverted);
         bson::BSONObj scope = _toBSON(offset, inverted, typeReader, TRUE);
         fieldName ? builder.appendCodeWScope(fieldName, code, scope)
                   : builder.appendCodeWScope("", code, scope);
         break;
      }
      case EncodedType::maxKey:
         fieldName ? builder.appendMaxKey(fieldName) : builder.appendMaxKey("");
         break;
      }
   }

   void keyString::_toNumeric(EncodedType type,
                              UINT32 &offset,
                              BOOLEAN inverted,
                              bson::BSONObjBuilder &builder,
                              typeBitsReader &typeReader,
                              const CHAR *fieldName)
   {

      typeBitsType originalType = typeReader.readNumeric();
      BOOLEAN isNegative = FALSE;
      switch (type)
      {
      case EncodedType::numericNaN:
         if (originalType == typeBitsType::DOUBLE)
         {
            fieldName
                ? builder.appendNumber(
                      fieldName, std::numeric_limits<FLOAT64>::quiet_NaN())
                : builder.appendNumber(
                      "", std::numeric_limits<FLOAT64>::quiet_NaN());
         }
         else
         {
            SDB_ASSERT(FALSE, "Unexpected original type");
         }
         break;

      case EncodedType::numericZero:
         if (originalType == typeBitsType::INT)
         {
            fieldName ? builder.appendNumber(fieldName, static_cast<INT32>(0))
                      : builder.appendNumber("", static_cast<INT32>(0));
         }
         else if (originalType == typeBitsType::LONG)
         {
            fieldName ? builder.appendNumber(fieldName, static_cast<INT64>(0))
                      : builder.appendNumber("", static_cast<INT64>(0));
         }
         else if (originalType == typeBitsType::DOUBLE)
         {
            typeBitsType zeroType = typeReader.readZero();
            SDB_ASSERT(zeroType == typeBitsType::NEGATIVE_ZERO ||
                           zeroType == typeBitsType::POSITIVE_ZERO,
                       "Unexpected zero type");
            FLOAT64 zero = zeroType == typeBitsType::NEGATIVE_ZERO ? -0.0 : 0.0;
            fieldName ? builder.appendNumber(fieldName, zero)
                      : builder.appendNumber("", zero);
         }
         else if (originalType == typeBitsType::DECIMAL)
         {
            typeBitsType zeroType = typeReader.readZero();
            SDB_ASSERT(zeroType == typeBitsType::NEGATIVE_ZERO ||
                           zeroType == typeBitsType::POSITIVE_ZERO,
                       "Unexpected zero type");
            const CHAR *zero =
                zeroType == typeBitsType::NEGATIVE_ZERO ? "-0.0" : "0.0";
            fieldName ? builder.appendDecimal(fieldName, zero)
                      : builder.appendDecimal("", zero);
         }
         else
         {
            SDB_ASSERT(FALSE, "Unexpected original type");
         }
         break;
      case EncodedType::numericNegativeSmallMagnitude:
      case EncodedType::numericNegativeLargeMagnitude:
         isNegative = TRUE;
         inverted = !inverted;
      case EncodedType::numericPositiveSmallMagnitude:
      case EncodedType::numericPositiveLargeMagnitude: {
         UINT64 encoded = _read<UINT64>(offset, inverted);
         encoded = ossBigEndianToNative(encoded);
         DecimalContinuationMarker dcm =
             static_cast<DecimalContinuationMarker>(encoded & 1ULL);
         encoded >>= 1;
         FLOAT64 abs;
         ossMemcpy(&abs, &encoded, sizeof(abs));
         if (dcm == DecimalContinuationMarker::hasNoContinuation)
         {
            if (originalType == typeBitsType::DOUBLE)
            {
               fieldName
                   ? builder.appendNumber(fieldName, isNegative ? -abs : abs)
                   : builder.appendNumber("", isNegative ? -abs : abs);
            }
            else if (originalType == typeBitsType::DECIMAL)
            {
               std::stringstream ss;
               ss << std::setprecision(
                         std::numeric_limits<FLOAT64>::max_digits10)
                  << (isNegative ? -abs : abs);
               bson::bsonDecimal dec;
               dec.fromString(ss.str().c_str());
               // Todo decimal meta
               fieldName ? builder.append(fieldName, dec)
                         : builder.append("", dec);
            }
            else if (originalType == typeBitsType::LONG)
            {
               INT64 val = static_cast<INT64>(isNegative ? -abs : abs);
               fieldName ? builder.appendNumber(fieldName, val)
                         : builder.appendNumber("", val);
            }
            else
            {
               SDB_ASSERT(FALSE, "Unexpected originalType");
            }
         }
         else
         {
            bson::bsonDecimal dec =
                _decodeDecimal(offset, inverted, isNegative, typeReader);
            // Todo decimal meta
            fieldName ? builder.append(fieldName, dec)
                      : builder.append("", dec);
         }
         break;
      }
      case EncodedType::numericNegative8ByteInt:
      case EncodedType::numericNegative7ByteInt:
      case EncodedType::numericNegative6ByteInt:
      case EncodedType::numericNegative5ByteInt:
      case EncodedType::numericNegative4ByteInt:
      case EncodedType::numericNegative3ByteInt:
      case EncodedType::numericNegative2ByteInt:
      case EncodedType::numericNegative1ByteInt:
         isNegative = TRUE;
         inverted = !inverted;
      case EncodedType::numericPositive1ByteInt:
      case EncodedType::numericPositive2ByteInt:
      case EncodedType::numericPositive3ByteInt:
      case EncodedType::numericPositive4ByteInt:
      case EncodedType::numericPositive5ByteInt:
      case EncodedType::numericPositive6ByteInt:
      case EncodedType::numericPositive7ByteInt:
      case EncodedType::numericPositive8ByteInt: {
         UINT64 integerPart = 0;
         UINT32 integralNeededBytes = neededBytesNumForInteger(type);
         for (UINT32 i = integralNeededBytes; i; i--)
         {
            integerPart = (integerPart << 8) | _read<UINT8>(offset, inverted);
         }

         BOOLEAN hasFractionPart = (integerPart & 1ULL);
         INT64 integerValue = integerPart >>= 1;
         if (!hasFractionPart)
         {
            if (isNegative)
            {
               integerValue = -integerValue;
            }
            switch (originalType)
            {
            case typeBitsType::INT:
               fieldName
                   ? builder.appendNumber(fieldName,
                                          static_cast<INT32>(integerValue))
                   : builder.appendNumber("", static_cast<INT32>(integerValue));
               break;
            case typeBitsType::LONG:
               fieldName
                   ? builder.appendNumber(fieldName,
                                          static_cast<INT64>(integerValue))
                   : builder.appendNumber("", static_cast<INT64>(integerValue));
               break;
            case typeBitsType::DOUBLE:
               fieldName
                   ? builder.appendNumber(fieldName,
                                          static_cast<FLOAT64>(integerValue))
                   : builder.appendNumber("",
                                          static_cast<FLOAT64>(integerValue));
               break;
            case typeBitsType::DECIMAL: {
               bson::bsonDecimal dec;
               dec.fromLong(integerValue);
               // Todo decimal meta
               fieldName ? builder.append(fieldName, dec)
                         : builder.append("", dec);
               break;
            }
            }
            break;
         }
         // Has fractional part
         UINT32 frcationalBytes = 8 - integralNeededBytes;
         UINT64 encoded = integerPart;
         for (int i = frcationalBytes; i; i--)
         {
            encoded = (encoded << 8) | _read<UINT8>(offset, inverted);
         }
         DecimalContinuationMarker dcm =
             static_cast<DecimalContinuationMarker>(encoded & 1ULL);
         if (dcm == DecimalContinuationMarker::hasNoContinuation)
         {
            FLOAT64 abs = static_cast<FLOAT64>((encoded &= (~1ULL)) *
                                               invPow256[frcationalBytes]);
            if (originalType == typeBitsType::DOUBLE)
            {
               fieldName
                   ? builder.appendNumber(fieldName, isNegative ? -abs : abs)
                   : builder.appendNumber("", isNegative ? -abs : abs);
            }
            else if (originalType == typeBitsType::DECIMAL)
            {
               std::stringstream ss;
               ss << std::setprecision(
                         std::numeric_limits<FLOAT64>::max_digits10)
                  << (isNegative ? -abs : abs);
               bson::bsonDecimal decFromDouble;
               decFromDouble.fromString(ss.str().c_str());
               // Todo decimal meta
               fieldName ? builder.append(fieldName, decFromDouble)
                         : builder.append("", decFromDouble);
            }
            else
            {
               SDB_ASSERT(FALSE, "Unexpected original type");
            }
         }
         else
         {
            bson::bsonDecimal dec =
                _decodeDecimal(offset, inverted, isNegative, typeReader);
            // Todo decimal meta
            fieldName ? builder.append(fieldName, dec)
                      : builder.append("", dec);
         }
         break;
      }
      default:
         SDB_ASSERT(FALSE, "Unexpected encoded type");
         break;
      }
   }

   bson::StringData keyString::_decodeStringLike(UINT32 &offset,
                                                 BOOLEAN inverted)
   {
      ossPoolString s;
      s.append(_readCString(offset, inverted).data());
      while (offset != _block.sizeBeforeKey + _block.keySize &&
             0xFF != _peek<UINT8>(offset, inverted))
      {
         offset += 1;
         s.append(_readCString(offset, inverted).data());
      }
      return s;
   }

   bson::bsonDecimal keyString::_decodeDecimal(UINT32 &offset,
                                               BOOLEAN inverted,
                                               BOOLEAN isNegative,
                                               typeBitsReader &typeReader)
   {
      bson::bsonDecimal dec;
      const UINT32 integerPartNdigit =
          ossBigEndianToNative(_read<UINT32>(offset, inverted));
      INT16 weight = 0;
      INT32 typemod = typeReader.read<INT32>();
      UINT16 ndigit = typeReader.read<UINT16>();
      ossPoolString decStr;
      if (isNegative)
      {
         decStr.append("-");
      }
      if (0 == integerPartNdigit)
      {
         decStr.append("0.");
         weight = ossBigEndianToNative(_read<INT16>(offset, inverted));
         SDB_ASSERT(weight < 0, "Unexpected weight");
         const UINT16 fractionPartNdigit = -weight;
         UINT16 digit = 0;
         for (UINT16 i = fractionPartNdigit; i; i--)
         {
            digit = static_cast<UINT16>(
                ossBigEndianToNative(_read<UINT16>(offset, inverted)) >> 1);
            SDB_ASSERT(digit >= 0 && digit < SDB_DECIMAL_NBASE,
                       "Unexpected digit value");
            CHAR pBuffers[SDB_DECIMAL_DEC_DIGITS + 1] = {};
            ossSnprintf(pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit);
            decStr.append(pBuffers);
         }
      }
      else
      {
         UINT16 digit = 0;
         for (UINT16 i = integerPartNdigit; i; i--)
         {
            digit = ossBigEndianToNative(_read<UINT16>(offset, inverted));
            SDB_ASSERT(digit >= 0 && digit < SDB_DECIMAL_NBASE,
                       "Unexpected digit value");
            CHAR pBuffers[SDB_DECIMAL_DEC_DIGITS + 1] = {};
            ossSnprintf(pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit);
            decStr.append(pBuffers);
         }
         decStr.append(".");
         for (UINT16 i = ndigit - integerPartNdigit; i; i--)
         {
            digit = static_cast<UINT16>(
                ossBigEndianToNative(_read<UINT16>(offset, inverted)) >> 1);
            SDB_ASSERT(digit >= 0 && digit < SDB_DECIMAL_NBASE,
                       "Unexpected digit value");
            CHAR pBuffers[SDB_DECIMAL_DEC_DIGITS + 1] = {};
            ossSnprintf(pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit);
            decStr.append(pBuffers);
         }
      }
      dec.fromString(decStr.c_str());
      dec.updateTypemod(typemod);
      return dec;
   }

   void keyString::_readBytes(UINT32 &offset,
                              BOOLEAN inverted,
                              CHAR *bytes,
                              UINT32 len)
   {
      if (inverted)
      {
         ossMemcpyFlipBits(&bytes, getDataSlice().data() + offset, len);
      }
      else
      {
         ossMemcpy(&bytes, getDataSlice().data() + offset, len);
      }
      offset += len;
   }

   bson::StringData keyString::_readCString(UINT32 &offset, BOOLEAN inverted)
   {
      const CHAR *start =
          static_cast<const CHAR *>(getDataSlice().data() + offset);
      const CHAR *end = static_cast<const CHAR *>(
          memchr(start,
                 '\0',
                 _block.sizeBeforeKey + getKeySlice().getSize() - offset));
      UINT32 bytesNum = end - start;
      offset += (bytesNum + 1);
      std::string s(start, bytesNum);
      if (inverted)
      {
         for (UINT32 i = 0; i < s.size(); i++)
         {
            s[i] = ~s[i];
         }
      }
      return s;
   }

   typeBitsReader::typeBitsReader(const CHAR *buf, UINT32 bufSize)
       : _buf(buf), _bufSize(bufSize)
   {
   }

   UINT8 typeBitsReader::_readBit()
   {
      const UINT32 byte = _curBit / 8;
      SDB_ASSERT(byte < _bufSize, "must be less than buffer size");
      const UINT32 offsetInByte = _curBit % 8;
      UINT8 oneOrZero = (_buf[byte] >> (7 - offsetInByte)) & 0b00000001;
      _curBit++;
      return oneOrZero;
   }

   typeBitsType typeBitsReader::readNumeric()
   {
      UINT8 output = 0;
      output += ((_readBit() << 1) + _readBit());
      return static_cast<typeBitsType>(output);
   }

   UINT8 typeBitsReader::readByte()
   {
      UINT8 output = 0;
      for (UINT32 i = 0; i < 8; i++)
      {
         output = (output << 1) + _readBit();
      }
      return output;
   }

   typeBitsType typeBitsReader::readZero()
   {
      return static_cast<typeBitsType>(_readBit());
   }

   typeBitsType typeBitsReader::readStringLike()
   {
      return static_cast<typeBitsType>(_readBit());
   }
} // namespace vessel
} // namespace engine