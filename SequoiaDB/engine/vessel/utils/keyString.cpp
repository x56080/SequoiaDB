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

   Source File Name = keyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/16/2022  LYC  Initial Draft
          07/20/2022  ZHY  Implement
   Last Changed =

*******************************************************************************/
#include "vessel/keyString.h"
#include "vessel/keyStringMetaByte.h"
#include "ossMemPool.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilAllocator.hpp"
#include "ossLikely.hpp"
#include "vessel/keyStringCoder.h"

#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

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
      if (SDB_OK != _parse(_ref, _desc))
      {
         reset();
      }
   }

   keyString::keyString(UINT32 size, const CHAR *data) : _ref(size, data)
   {
      if (SDB_OK != _parse(_ref, _desc))
      {
         reset();
      }
   }

   keyString::keyString(const keyString &o) : _ref(o._ref), _desc(o._desc)
   {
   }

   keyString &keyString::operator=(const keyString &o)
   {
      reset();
      _ref = o._ref;
      _desc = o._desc;
      return *this;
   }

   keyString::keyString(keyString &&o) noexcept
   {
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _desc = o._desc;
         o._bufferOwned = nullptr;
         o.reset();
      }
   }

   keyString &keyString::operator=(keyString &&o) noexcept
   {
      reset();
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _desc = o._desc;
         o._bufferOwned = nullptr;
         o.reset();
      }
      return *this;
   }

   void keyString::reset()
   {
      _ref.reset();
      _desc.reset();
      if (nullptr != _bufferOwned)
      {
         utilPoolAllocator allocator;
         allocator.free(_bufferOwned);
         _bufferOwned = nullptr;
      }
      _bufferSize = 0;
      return;
   }

   INT32 keyString::init(const slice &s)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(!s.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _ref = s;
      rc = _parse(_ref, _desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parese key stirng data:%d", rc);
         goto error;
      }

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

   INT32 keyString::_parse(const slice &s, keyStringDescriptor &desc) const
   {
      INT32 rc = SDB_OK;
      bytesReader reader;
      desc.reset();

      rc = parseMetaFromSlice(s, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse descriptor from slice, %d", rc);
         goto error;
      }

      if (s.getSize() != desc.getStringSizeExpected())
      {
         PD_LOG(PDERROR, "unexpected total string size[%d, %d]",
               s.getSize(), desc.getStringSizeExpected());
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }
   done:
      return rc;
   error:
      desc.reset();
      goto done;
   }

   BOOLEAN keyString::_loadSizeData(bytesReader &reader,
                                    BOOLEAN nonzero,
                                    UINT32 &size)
   {
      SDB_ASSERT(!reader.isOutOfBound(), "can not be invalid");
      size = 0;
      if (reader.getUINT8() < KEY_STRING_TYNI_SIZE_BOUND)
      {
         size = reader.getUINT8();
      }
      else if (!reader.slide(KEY_STRING_SWORD_SIZE))
      {
         return FALSE;
      }
      else
      {
         size = reader.get<UINT32>();
      }

      return !nonzero || 0 != size;
   }

   // void keyString::_adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize)
   // {
   //    SDB_ASSERT(nullptr != buffer && 0 < bufferSize, "can not be invalid");
   //    SDB_ASSERT(0 < ksSize && ksSize <= bufferSize, "invalid key string
   //    size"); reset(); _bufferOwned = buffer; _bufferSize = bufferSize;
   //    _ref.reset(ksSize, _bufferOwned);
   // #if defined(_DEBUG)
   //    SDB_ASSERT(_validate(_ref), "can not be invalid");
   // #endif
   //    return;
   // }

   slice keyString::getKeySlice() const
   {
      if (OSS_UNLIKELY(!isValid()))
      {
         return slice();
      }
      else
      {
         return _ref.getSlice(0, _desc.keySize);
      }
   }

   slice keyString::getKeyHeadSlice() const
   {
      return hasKeyHead() ?
             _ref.getSlice(0, _desc.keyHeadSize)
            : slice();
   }

   slice keyString::getKeySliceAfterHeader() const
   {
      return _desc.keyHeadSize < _desc.keySize ?
             _ref.getSlice(_desc.keyHeadSize, _desc.keySize - _desc.keyHeadSize) :
             slice();
   }

   slice keyString::getKeyElementsSlice() const
   {
      UINT32 size = getKeyElementsSize();
      return 0 < size ?
             _ref.getSlice(_desc.keyHeadSize, size):
             slice();   
   }

   slice keyString::getKeyTailSlice() const
   {
      return hasKeyTail() ?
             _ref.getSlice(_desc.keySize - _desc.keyTailSize, _desc.keyTailSize) :
             slice();
   }

   slice keyString::getKeySliceExceptTail() const
   {
      return _desc.keyTailSize < _desc.keySize ?
             _ref.getSlice(0, _desc.keySize - _desc.keyTailSize)
             : slice();
   }

   slice keyString::getTypeBits() const
   {
      return isValid() && hasTypeBits()
                 ? _ref.getSlice(_desc.keySize, _desc.typeBitsSize)
                 : slice();
   }

   INT32 keyString::compare(const keyString &s) const
   {
      SDB_ASSERT(isValid() && s.isValid(), "can not be invalid");
      return getKeySlice().compare(s.getKeySlice());
   }

   INT32 keyString::compareElements(const keyString &ks) const
   {
      SDB_ASSERT(isValid() && ks.isValid(), "can not be invalid");
      return getKeyElementsSlice().compare(ks.getKeyElementsSlice());
   }

   keyString::bodyReader::bodyReader(const CHAR *bodyBuf,
                                     UINT32 bufSize,
                                     const CHAR *typeBitsBuf,
                                     UINT32 typeBitsBufSize)
       : _buf(bodyBuf), _bufSize(bufSize),
         typeReader(typeBitsBuf, typeBitsBufSize)
   {
   }

   template <typename T> T keyString::bodyReader::_read(BOOLEAN inverted)
   {
      UINT32 size = sizeof(T);
      T t;
      if (inverted)
      {
         ossMemcpyFlipBits(&t, _buf + _offset, size);
      }
      else
      {
         ossMemcpy(&t, _buf + _offset, size);
      }
      SDB_ASSERT(_offset + size <= _bufSize, "out of buffer size");
      _offset += size;
      return t;
   }

   template <typename T> T keyString::bodyReader::_peek(BOOLEAN inverted) const
   {
      UINT32 size = sizeof(T);
      T t;
      if (inverted)
      {
         ossMemcpyFlipBits(&t, _buf + _offset, size);
      }
      else
      {
         ossMemcpy(&t, _buf + _offset, size);
      }
      return t;
   }

   bson::BSONObj keyString::toBSON(const bson::BSONObj &pattern,
                                   BOOLEAN withFieldName) const
   {
      BSONObjBuilder builder;
      SDB_ASSERT(isValid(), "must be valid");
      slice s = getKeyElementsSlice();
      bodyReader br(s.data(),
                    s.size(),
                    getTypeBits().data(),
                    getTypeBitsSize());
      br.toBSON(pattern, builder, withFieldName);
      return builder.obj();
   }

   bson::BSONObj keyString::toBSON(const bson::BSONObj &pattern,
                                   bson::BufBuilder &bufBuilder,
                                   BOOLEAN withFieldName) const
   {
      BSONObjBuilder builder(bufBuilder);
      SDB_ASSERT(isValid(), "must be valid");
      slice s = getKeyElementsSlice();
      bodyReader br(s.data(),
                    s.size(),
                    getTypeBits().data(),
                    getTypeBitsSize());
      return br.toBSON(pattern, builder, withFieldName);
   }

   bson::BSONObj keyString::bodyReader::toBSON(const bson::BSONObj &pattern,
                                               BSONObjBuilder &builder,
                                               BOOLEAN withFieldName)
   {
      _offset = 0;
      bson::BSONObjIterator it(pattern);
      while (_offset < _bufSize && it.more())
      {
         bson::BSONElement ele = it.next();
         BOOLEAN inverted = ele.numberInt() == -1 ? TRUE : FALSE;
         EncodedType type = static_cast<EncodedType>(_read<UINT8>(inverted));
         DiscriminatorValue dv = static_cast<DiscriminatorValue>(type);
         if (DiscriminatorValue::END == dv || DiscriminatorValue::LESS == dv ||
             DiscriminatorValue::GREATER == dv)
         {
            break;
         }
         toBsonValue(type,
                     inverted,
                     builder,
                     withFieldName ? ele.fieldName() : nullptr);
      }
      SDB_ASSERT(KEY_STRING_ONLY_END_SIZE == _bufSize - _offset ||
                     KEY_STRING_DISCRIMINATOR_AND_END_SIZE ==
                         _bufSize - _offset,
                 "Unexpected size");
      if (KEY_STRING_DISCRIMINATOR_AND_END_SIZE == _bufSize - _offset)
      {
         DiscriminatorValue dv = _read<DiscriminatorValue>(FALSE);
         SDB_ASSERT(DiscriminatorValue::LESS == dv ||
                        DiscriminatorValue::GREATER == dv,
                    "Unexpected discriminator byte");
      }
      if (KEY_STRING_ONLY_END_SIZE == _bufSize - _offset)
      {
         DiscriminatorValue dv = _read<DiscriminatorValue>(FALSE);
         SDB_ASSERT(DiscriminatorValue::END == dv,
                    "Unexpected discriminator byte");
      }
      return builder.done();
   }

   void keyString::bodyReader::_toBSON(BOOLEAN inverted,
                                       bson::BSONObjBuilder &builder,
                                       const CHAR *fieldName)
   {
      bson::BSONObjBuilder newObjBuilder;
      while (0 != static_cast<UINT8>(_read<EncodedType>(inverted)))
      {
         ossPoolString name;
         _readCString(inverted, name);
         toBsonValue(_read<EncodedType>(inverted),
                     inverted,
                     newObjBuilder,
                     name.data());
      }
      fieldName
          ? builder.appendObject(fieldName, newObjBuilder.done().objdata())
          : builder.appendObject("", newObjBuilder.done().objdata());
   }

   bson::BSONObj keyString::bodyReader::_toBSON(BOOLEAN inverted,
                                                BOOLEAN withFieldName)
   {
      bson::BSONObjBuilder newObjBuilder;
      while (0 != static_cast<UINT8>(_read<EncodedType>(inverted)))
      {
         ossPoolString name;
         _readCString(inverted, name);
         toBsonValue(_read<EncodedType>(inverted),
                     inverted,
                     newObjBuilder,
                     withFieldName ? name.data() : "");
      }
      return newObjBuilder.obj();
   }

   void keyString::bodyReader::toBsonValue(EncodedType type,
                                           BOOLEAN inverted,
                                           bson::BSONObjBuilder &builder,
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
         _toNumeric(type, inverted, builder, fieldName);
         break;
      case EncodedType::stringLike: {
         ossPoolString s;
         _decodeStringLike(inverted, s);
         typeBitsType originalType = typeReader.readStringLike();
         if (typeBitsType::STRING == originalType)
         {
            fieldName ? builder.append(fieldName, s) : builder.append("", s);
         }
         else
         {
            fieldName ? builder.appendSymbol(fieldName, s)
                      : builder.appendSymbol("", s);
         }
         break;
      }
      case EncodedType::object: {
         _toBSON(inverted, builder, fieldName);
         break;
      }
      case EncodedType::array: {
         bson::BSONObjBuilder arrayBuilder;
         EncodedType type = _read<EncodedType>(inverted);
         UINT32 elemCount = 0;
         while (0 != static_cast<UINT8>(type))
         {
            toBsonValue(type,
                        inverted,
                        arrayBuilder,
                        std::to_string(elemCount).c_str());
            elemCount++;
            type = _read<EncodedType>(inverted);
         }
         BSONArray arr(arrayBuilder.obj());
         fieldName ? builder.appendArray(fieldName, arr)
                   : builder.appendArray("", arr);
         break;
      }

      case EncodedType::binData: {
         UINT8 firstByteLen = _read<UINT8>(inverted);
         UINT32 binDataLen = 0;
         if (0xFF == firstByteLen)
         {
            binDataLen = ossBigEndianToNative(_read<UINT32>(inverted));
         }
         else
         {
            binDataLen = firstByteLen;
         }
         bson::BinDataType binDataType =
             static_cast<bson::BinDataType>(_read<UINT8>(inverted));
         CHAR data[binDataLen];
         _readBytes(inverted, data, binDataLen);
         if (binDataType == BinDataType::ByteArrayDeprecated)
         {
            fieldName ? builder.appendBinDataArrayDeprecated(
                            fieldName, data + 4, binDataLen - 4)
                      : builder.appendBinDataArrayDeprecated(
                            "", data + 4, binDataLen - 4);
         }
         else
         {
            fieldName
                ? builder.appendBinData(
                      fieldName, binDataLen, binDataType, data)
                : builder.appendBinData("", binDataLen, binDataType, data);
         }
         break;
      }
      case EncodedType::oid: {
         bson::OID oid = _read<bson::OID>(inverted);
         fieldName ? builder.appendOID(fieldName, &oid)
                   : builder.appendOID("", &oid);
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
      case EncodedType::time: {
         typeBitsType originalType = typeReader.readTimestampOrDate();
         INT64 encoded = ossBigEndianToNative(_read<INT64>(inverted));
         INT64 seconds = encoded ^ std::numeric_limits<INT64>::min();
         UINT32 microseconds = ossBigEndianToNative(_read<UINT32>(inverted));

         if (originalType == typeBitsType::DATE)
         {
            INT64 val = seconds * 1000 + microseconds / 1000;
            Date_t dt(val);
            fieldName ? builder.appendDate(fieldName, dt)
                      : builder.appendDate("", dt);
         }
         else if (originalType == typeBitsType::TIMESTAMP)
         {
            fieldName
                ? builder.appendTimestamp(
                      fieldName, seconds * 1000, microseconds)
                : builder.appendTimestamp("", seconds * 1000, microseconds);
         }
         else
         {
            SDB_ASSERT(FALSE, "Reserved");
         }
         break;
      }
      case EncodedType::regEx: {
         ossPoolString regex;
         _readCString(inverted, regex);
         ossPoolString flags;
         _readCString(inverted, flags);
         fieldName ? builder.appendRegex(fieldName, regex, flags)
                   : builder.appendRegex("", regex, flags);
         break;
      }
      case EncodedType::dbRef: {
         UINT32 nsLen = ossBigEndianToNative(_read<UINT32>(inverted));
         CHAR nsData[nsLen + 1];
         _readBytes(inverted, nsData, nsLen);
         nsData[nsLen] = '\0';
         bson::StringData s(nsData, nsLen);
         bson::OID oid = _read<bson::OID>(inverted);
         fieldName ? builder.appendDBRef(fieldName, s, oid)
                   : builder.appendDBRef("", s, oid);
         break;
      }

      case EncodedType::code: {
         EncodedType type = _read<EncodedType>(inverted);
         SDB_ASSERT(EncodedType::stringLike == type, "Unexpected encoded type");
         ossPoolString code;
         _decodeStringLike(inverted, code);
         fieldName ? builder.appendCode(fieldName, code)
                   : builder.appendCode("", code);
         break;
      }
      case EncodedType::codeWithScope: {
         EncodedType type = _read<EncodedType>(inverted);
         SDB_ASSERT(EncodedType::stringLike == type, "Unexpected encoded type");
         ossPoolString code;
         _decodeStringLike(inverted, code);
         bson::BSONObj scope = _toBSON(inverted, TRUE);
         fieldName ? builder.appendCodeWScope(fieldName, code, scope)
                   : builder.appendCodeWScope("", code, scope);
         break;
      }
      case EncodedType::maxKey:
         fieldName ? builder.appendMaxKey(fieldName) : builder.appendMaxKey("");
         break;
      default:
         SDB_ASSERT(FALSE, "Unexpected encoded type");
         break;
      }
   }

   void keyString::bodyReader::_toNumeric(EncodedType type,
                                          BOOLEAN inverted,
                                          bson::BSONObjBuilder &builder,
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
         UINT64 encoded = _read<UINT64>(inverted);
         encoded = ossBigEndianToNative(encoded);
         if (encoded == ~0ULL)
         {
            if (originalType == typeBitsType::DOUBLE)
            {
               FLOAT64 num = std::numeric_limits<FLOAT64>::infinity();
               fieldName
                   ? builder.appendNumber(fieldName, isNegative ? -num : num)
                   : builder.appendNumber("", isNegative ? -num : num);
            }
            else if (originalType == typeBitsType::DECIMAL)
            {
               bsonDecimal dec;
               isNegative ? dec.setMin() : dec.setMax();
               fieldName ? builder.append(fieldName, dec)
                         : builder.append("", dec);
            }
            else
            {
               SDB_ASSERT(FALSE, "Unexpected original type");
            }
            break;
         }
         ContinuationMarker dcm =
             static_cast<ContinuationMarker>(encoded & 1ULL);
         encoded >>= 1;
         FLOAT64 abs;
         ossMemcpy(&abs, &encoded, sizeof(abs));
         if (dcm == ContinuationMarker::hasNoContinuation)
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
               INT32 typemod = typeReader.read<INT32>();
               INT16 ndigit = typeReader.read<UINT16>();
               dec.updateTypemod(typemod);
               SDB_ASSERT(dec.getNdigit() == ndigit, "Expected to be equal");
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
            bson::bsonDecimal dec = _decodeDecimal(inverted, isNegative);
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
         UINT64 encoded = 0;
         UINT32 integralNeededBytes = neededBytesNumForInteger(type);
         for (UINT32 i = integralNeededBytes; i; i--)
         {
            encoded = (encoded << 8) | _read<UINT8>(inverted);
         }

         BOOLEAN hasFractionPart = (encoded & 1ULL);
         INT64 integerValue = encoded >> 1;
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
               INT32 typemod = typeReader.read<INT32>();
               /*INT16 ndigit = */ typeReader.read<UINT16>();
               dec.updateTypemod(typemod);
               fieldName ? builder.append(fieldName, dec)
                         : builder.append("", dec);
               break;
            }
            }
            break;
         }
         // Has fractional part
         UINT32 frcationalBytes = 8 - integralNeededBytes;
         UINT64 fractionalEncoded = integerValue;
         for (int i = frcationalBytes; i; i--)
         {
            fractionalEncoded =
                (fractionalEncoded << 8) | _read<UINT8>(inverted);
         }
         ContinuationMarker dcm =
             static_cast<ContinuationMarker>(fractionalEncoded & 1ULL);
         if (0 == frcationalBytes)
         {
            dcm = static_cast<ContinuationMarker>(encoded & 1ULL);
         }
         if (dcm == ContinuationMarker::hasNoContinuation)
         {
            FLOAT64 abs = static_cast<FLOAT64>((fractionalEncoded &= (~1ULL)) *
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
               INT32 typemod = typeReader.read<INT32>();
               INT16 ndigit = typeReader.read<UINT16>();
               decFromDouble.updateTypemod(typemod);
               SDB_ASSERT(decFromDouble.getNdigit() == ndigit,
                          "Expected to be equal");
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
            bson::bsonDecimal dec = _decodeDecimal(inverted, isNegative);
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

   void keyString::bodyReader::_decodeStringLike(BOOLEAN inverted,
                                                 ossPoolString &s)
   {
      _readCString(inverted, s);
      while (_offset != _bufSize && 0xFF == _peek<UINT8>(inverted))
      {
         SDB_ASSERT(_offset + 1 <= _bufSize, "out of buffer size");
         _offset += 1;
         s.append("\x00", 1);
         _readCString(inverted, s);
      }
   }

   bson::bsonDecimal keyString::bodyReader::_decodeDecimal(BOOLEAN inverted,
                                                           BOOLEAN isNegative)
   {
      bson::bsonDecimal dec;
      const UINT32 integerPartNdigit =
          ossBigEndianToNative(_read<UINT32>(inverted));
      INT16 weight = 0;
      INT32 typemod = typeReader.read<INT32>();
      INT16 ndigit = typeReader.read<UINT16>();
      ossPoolString decStr;
      if (isNegative)
      {
         decStr.append("-");
      }
      if (0 == integerPartNdigit)
      {
         decStr.append("0.");
         weight = -ossBigEndianToNative(_read<INT16>(!inverted));
         SDB_ASSERT(weight < 0, "Unexpected weight");
         while (weight++ < -1)
         {
            decStr.append("0000");
         }
         const UINT16 fractionPartNdigit = ndigit;
         UINT16 digit = 0;
         for (UINT16 i = fractionPartNdigit; i; i--)
         {
            digit = static_cast<UINT16>(
                ossBigEndianToNative(_read<UINT16>(inverted)) >> 1);
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
         if (static_cast<UINT16>(ndigit) < integerPartNdigit)
         {
            UINT16 i = 0;
            for (; i < ndigit; i++)
            {
               digit = ossBigEndianToNative(_read<UINT16>(inverted));
               SDB_ASSERT(digit >= 0 && digit < SDB_DECIMAL_NBASE,
                          "Unexpected digit value");
               CHAR pBuffers[SDB_DECIMAL_DEC_DIGITS + 1] = {};
               ossSnprintf(pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit);
               decStr.append(pBuffers);
            }
            for (; i < integerPartNdigit;i++)
            {
               decStr.append("0000");
            }
         }
         else
         {
            for (UINT16 i = integerPartNdigit; i; i--)
            {
               digit = ossBigEndianToNative(_read<UINT16>(inverted));
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
                   ossBigEndianToNative(_read<UINT16>(inverted)) >> 1);
               SDB_ASSERT(digit >= 0 && digit < SDB_DECIMAL_NBASE,
                          "Unexpected digit value");
               CHAR pBuffers[SDB_DECIMAL_DEC_DIGITS + 1] = {};
               ossSnprintf(pBuffers, SDB_DECIMAL_DEC_DIGITS + 1, "%04d", digit);
               decStr.append(pBuffers);
            }
         }
      }
      dec.fromString(decStr.c_str());
      dec.updateTypemod(typemod);
      return dec;
   }

   void keyString::bodyReader::_readBytes(BOOLEAN inverted,
                                          CHAR *bytes,
                                          UINT32 len)
   {
      if (inverted)
      {
         ossMemcpyFlipBits(bytes, _buf + _offset, len);
      }
      else
      {
         ossMemcpy(bytes, _buf + _offset, len);
      }
      SDB_ASSERT(_offset + len <= _bufSize, "out of buffer size");
      _offset += len;
   }

   void keyString::bodyReader::_readCString(BOOLEAN inverted, ossPoolString &s)
   {
      UINT32 strOldSize = s.size();
      const UINT8 endChar = inverted ? 0xFF : 0;
      const CHAR *start = static_cast<const CHAR *>(_buf + _offset);
      const CHAR *end =
          static_cast<const CHAR *>(memchr(start, endChar, _bufSize - _offset));
      UINT32 bytesNum = end - start;
      SDB_ASSERT(_offset + bytesNum + 1 <= _bufSize, "out of buffer size");
      _offset += (bytesNum + 1);
      s.append(start, bytesNum);
      if (inverted)
      {
         for (UINT32 i = strOldSize; i < s.size(); i++)
         {
            s[i] = ~s[i];
         }
      }
   }

   recordID keyString::getRid() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      recordID rid;
      slice data = getKeyTailSlice();
      if (data.getSize() <= keyStringCoder::RID_ENCODING_SIZE)
      {
         rid = keyStringCoder().decodeToRid(data.getData());
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid data size after key");
      }
      return rid;
   }

   INT32 keyString::compareCoding(UINT32 sizea,
                                  const CHAR *bufa,
                                  UINT32 sizeb,
                                  const CHAR *bufb)
   {
      UINT32 keySize0 = (UINT8)(bufa[sizea - KEY_STRING_MB_HEADER_SIZE - 1]);
      UINT32 keySize1 = (UINT8)(bufb[sizeb - KEY_STRING_MB_HEADER_SIZE - 1]);
      if (keySize0 == KEY_STRING_TYNI_SIZE_BOUND)
      {
         keySize0 =
             *((const UINT32 *)(bufa + sizea - KEY_STRING_MB_HEADER_SIZE -
                                KEY_STRING_SWORD_SIZE));
      }
      if (keySize1 == KEY_STRING_TYNI_SIZE_BOUND)
      {
         keySize1 =
             *((const UINT32 *)(bufb + sizeb - KEY_STRING_MB_HEADER_SIZE -
                                KEY_STRING_SWORD_SIZE));
      }

      INT32 res = ossMemcmp(bufa, bufb, OSS_MIN(keySize0, keySize1));
      if (0 == res)
      {
         if (keySize0 < keySize1)
         {
            res = -1;
         }
         else if (keySize0 > keySize1)
         {
            res = 1;
         }
      }

      return res;
   }

   INT32 keyString::parseMetaFromSlice(const slice &s, keyStringDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      const keyStringMetaBlockHeader *header = nullptr;
      keyStringMetaByte mbyte;
      bytesReader reader;
      desc.reset();

      if (OSS_UNLIKELY(!s.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (s.getSize() <= KEY_STRING_MB_HEADER_SIZE)
      {
         PD_LOG(PDERROR, "invalid slice size:%d", s.getSize());
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }

      header = reinterpret_cast<const keyStringMetaBlockHeader *>(
                           s.getData() + s.getSize() - KEY_STRING_MB_HEADER_SIZE);
      if (!header->isValid())
      {
         PD_LOG(PDERROR, "invalid key string meta block header");
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }

      mbyte.init(header->metaByte);
      reader.init(s.getSlice(0, s.getSize() - KEY_STRING_MB_HEADER_SIZE), TRUE);

      if (!_loadSizeData(reader, FALSE, desc.keySize))
      {
         PD_LOG(PDERROR, "failed to load key size");
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }

      if (mbyte.hasKeyHead())
      {
         if (!reader.slide(1))
         {
            PD_LOG(PDERROR, "failed to move to key head size begin pos");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }

         if (!_loadSizeData(reader, TRUE, desc.keyHeadSize))
         {
            PD_LOG(PDERROR, "failed to load key head size");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }
      }

      if (mbyte.hasKeyTail())
      {
         if (!reader.slide(1))
         {
            PD_LOG(PDERROR, "failed to move to key tail size begin pos");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }

         if (!_loadSizeData(reader, TRUE, desc.keyTailSize))
         {
            PD_LOG(PDERROR, "failed to load key tail size");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }
      }

      if (mbyte.hasTypeBits())
      {
         if (!reader.slide(1))
         {
            PD_LOG(PDERROR, "failed to move to type bits size begin pos");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }

         if (!_loadSizeData(reader, TRUE, desc.typeBitsSize))
         {
            PD_LOG(PDERROR, "failed to load type bits size");
            rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
            goto error;
         }
      }

      if (!desc.isValid())
      {
         PD_LOG(PDERROR, "invalid string descriptor parsed");
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }
   done:
      return rc;
   error:
      desc.reset();
      goto done;
   }

   ///////////////////////////////

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

   typeBitsType typeBitsReader::readTimestampOrDate()
   {
      UINT8 output = 0;
      output += ((_readBit() << 1) + _readBit());
      return static_cast<typeBitsType>(output);
   }
} // namespace vessel
} // namespace engine