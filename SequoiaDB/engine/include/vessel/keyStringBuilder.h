#ifndef KEY_SLICE_BUILDER_H_
#define KEY_SLICE_BUILDER_H_

#include "../bson/bsonDecimal.h"
#include "../bson/bsonelement.h"
#include "../bson/bsontypes.h"
#include "../bson/bsonobj.h"
#include "../bson/bsonobjiterator.h"
#include "oss.hpp"
#include "ossErr.h"
#include "pd.hpp"
#include "utilAllocator.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/slice.h"
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>
namespace engine
{
namespace vessel
{
   template <typename T> T nativeToBigEndian(T in)
   {
#ifdef SDB_BIG_ENDIAN
      return in;
#else
      T out;
      UINT32 size = sizeof(in);
      SDB_ASSERT(size % 2 == 0 && size <= 8,
                 "the size of value to be converted must be 2, 4, 6 or 8");
      ossEndianConvertIf(in, out, TRUE);
      return out;
#endif
   }

   constexpr FLOAT64 pow256[] = {
       1.0,                                            // 2**0
       1.0 * 256,                                      // 2**8
       1.0 * 256 * 256,                                // 2**16
       1.0 * 256 * 256 * 256,                          // 2**24
       1.0 * 256 * 256 * 256 * 256,                    // 2**32
       1.0 * 256 * 256 * 256 * 256 * 256,              // 2**40
       1.0 * 256 * 256 * 256 * 256 * 256 * 256,        // 2**48
       1.0 * 256 * 256 * 256 * 256 * 256 * 256 * 256}; // 2**56

   // First FLOAT64 that is not an int64
   constexpr FLOAT64 minLargeFloat64 = 1ULL << 63;
   constexpr INT32 DOUBLE_PRECISION_10 =
       std::numeric_limits<double>::max_digits10;

   enum class EncodedType : UINT8
   {
      minKey = 10,
      undefined = 15,
      nullish = 20,
      numeric = 30,
      numericNaN = numeric + 0,
      numericNegativeLargeMagnitude = numeric + 1,
      numericNegative8ByteInt = numeric + 2,
      numericNegative7ByteInt = numeric + 3,
      numericNegative6ByteInt = numeric + 4,
      numericNegative5ByteInt = numeric + 5,
      numericNegative4ByteInt = numeric + 6,
      numericNegative3ByteInt = numeric + 7,
      numericNegative2ByteInt = numeric + 8,
      numericNegative1ByteInt = numeric + 9,
      numericNegativeSmallMagnitude = numeric + 10,
      numericZero = numeric + 11,
      numericPositiveSmallMagnitude = numeric + 12,
      numericPositive1ByteInt = numeric + 13,
      numericPositive2ByteInt = numeric + 14,
      numericPositive3ByteInt = numeric + 15,
      numericPositive4ByteInt = numeric + 16,
      numericPositive5ByteInt = numeric + 17,
      numericPositive6ByteInt = numeric + 18,
      numericPositive7ByteInt = numeric + 19,
      numericPositive8ByteInt = numeric + 20,
      numericPositiveLargeMagnitude = numeric + 21,
      stringLike = 60,
      object = 70,
      array = 80,
      binData = 90,
      oid = 100,
      boolean = 110,
      booleanFalse = boolean + 0,
      booleanTrue = boolean + 1,
      date = 120,
      timestamp = 130,
      regEx = 140,
      dbRef = 150,
      code = 160,
      codeWithScope = 170,
      maxKey = 240
   };
   static_assert(
       EncodedType::numericPositiveLargeMagnitude < EncodedType::stringLike,
       "kNumericPositiveLargeMagnitude must be less than kStringLike");

   EncodedType bsonTypeToSupertype(bson::BSONType type);
   INT32 countLeadingZeros64(UINT64 num);

   class typeBits : public SDBObject
   {

   private:
      enum bits : UINT8
      {
         stringBit = 0b0,
         symbolBit = 0b1,

         intBits = 0b00,
         longBits = 0b01,
         doubleBits = 0b10,
         decimalBits = 0b11,
         positiveDoubleZero = 0b0,
         negativeDoubleZero = 0b1
      };

   public:
      void reset();
      INT32 appendBit(UINT8 oneOrZero);
      INT32 appendString();
      INT32 appendSymbol();
      INT32 appendNumberDouble();
      INT32 appendNumberInt();
      INT32 appendNumberLong();
      INT32 appendNumberDecimal();
      INT32 appendPositiveZero();
      INT32 appendNegativeZero();
      INT32 appendBits(const CHAR *bytes, const UINT32 bytesSize);
      INT32 appendDecimalMeta(const bson::bsonDecimal &dec);
      const CHAR *getBuf() const
      {
         return _buf;
      }
      UINT32 getBufSize() const
      {
         return _bufSize;
      }

   private:
      INT32 _ensureBytes(UINT32 length);

   private:
      UINT32 _curBit = 0;
      CHAR *_buf = nullptr;
      UINT32 _bufSize = 0;
      UINT32 _capacity = 0;
      utilStackAllocator<32> _allocator;
   };

   constexpr UINT8 BASIC_KEY_STRING_VERSION = 1;
   class keyString : public SDBObject
   {
   public:
      keyString() = default;
      keyString(slice data) : _data(data)
      {
      }
      BOOLEAN isOwned();
      INT32 getOwned(CHAR *buf);
      const CHAR *getDataBuf();
      UINT32 getDataSize();

   private:
      slice _data;
      CHAR *_buf = 0;
      UINT32 _bufSize = 0;
   };

   template <typename Allocator = utilStackAllocator<>>
   class keyStringBuilder : public SDBObject
   {
   public:
      keyStringBuilder() = default;
      keyStringBuilder operator=(const keyStringBuilder &) = delete;
      keyStringBuilder(const keyStringBuilder &) = delete;
      virtual ~keyStringBuilder()
      {
         if (_buf)
         {
            _allocator.free(_buf);
         }
      }

   private:
      enum class builderStatus
      {
         empty,
         beforeElements,
         appendingElements,
         appendedTypeBits,
         appendedMetaBlock
      };

      enum class DecimalContinuationMarker : UINT8
      {
         hasNoContinuation = 0b0,
         hasContinuation = 0b1,
      };

   public:
      using stringTransformFn =
          std::function<std::string(const bson::StringData &)>;
      void reset();
      void resetTypeBits(const typeBits &tb);
      INT32 appendBSONElement(const bson::BSONElement &elem,
                              BOOLEAN isDescending = FALSE,
                              const stringTransformFn &f = nullptr);
      INT32 appendAllElements(const bson::BSONObj &obj,
                              orderingWrapper ord,
                              const stringTransformFn &f = nullptr);
      template <
          typename T,
          class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      INT32 appendUnsignedWithoutType(const T &val,
                                      BOOLEAN isDescending = FALSE)
      {
         INT32 rc = SDB_OK;
         _transition(builderStatus::beforeElements);
         rc = _append(nativeToBigEndian(val), isDescending);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append unsigned fixed field, rc:%d", rc);
            goto error;
         }
         SDB_ASSERT(_sizeAheadElements + sizeof(val) <= 0xff,
                    "size ahead key string must be equal or less than 255");
         _sizeAheadElements += sizeof(val);
      done:
         return rc;
      error:
         reset();
         goto done;
      }

      template <typename T,
                class = typename std::enable_if<std::is_signed<T>::value>::type>
      INT32 appendSignedWithoutType(const T &val, BOOLEAN isDescending = FALSE)
      {
         INT32 rc = SDB_OK;
         _transition(builderStatus::beforeElements);
         T mask = std::numeric_limits<T>::min();
         T tmp = val;
         tmp ^= mask;
         rc = _append(nativeToBigEndian(tmp), isDescending);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append signed fixed field, rc:%d", rc);
            goto error;
         }
         SDB_ASSERT(_sizeAheadElements + sizeof(val) <= 0xff,
                    "size ahead key string must be equal or less than 255");
         _sizeAheadElements += sizeof(val);
      done:
         return rc;
      error:
         reset();
         goto done;
      }

   public:
      INT32 done();
      UINT8 getVersion() const
      {
         return BASIC_KEY_STRING_VERSION;
      }
      keyString getKeyString();

   protected:
      INT32 _appendBool(BOOLEAN val, BOOLEAN invert);
      INT32 _appendDate(const bson::Date_t &val, BOOLEAN invert);
      INT32 _appendTimestamp(INT64 val, BOOLEAN invert);
      INT32 _appendOID(const bson::OID &val, BOOLEAN invert);
      INT32 _appendString(const bson::StringData &val,
                          BOOLEAN invert,
                          const stringTransformFn &f = nullptr);
      INT32 _appendSymbol(const bson::StringData &val, BOOLEAN invert);
      INT32 _appendCode(const bson::StringData &val, BOOLEAN invert);
      INT32 _appendCodeWScope(const bson::StringData &code,
                              const bson::BSONObj &scope,
                              BOOLEAN invert);
      INT32 _appendBinData(const CHAR *data,
                           UINT32 dataSize,
                           bson::BinDataType type,
                           BOOLEAN invert);
      INT32 _appendRegex(const bson::StringData &regex,
                         const bson::StringData &flags,
                         BOOLEAN invert);
      INT32 _appendDBRef(const bson::StringData &dbrefNS,
                         const bson::OID &dbrefOID,
                         BOOLEAN invert);
      INT32 _appendArray(const bson::BSONArray &val,
                         BOOLEAN invert,
                         const stringTransformFn &f = nullptr);
      INT32 _appendObject(const bson::BSONObj &val,
                          BOOLEAN invert,
                          const stringTransformFn &f = nullptr);
      INT32 _appendNumberDouble(const FLOAT64 num, BOOLEAN invert);
      INT32 _appendNumberInt(const INT32 num, BOOLEAN invert);
      INT32 _appendNumberLong(const INT64 num, BOOLEAN invert);
      INT32 _appendNumberDecimal(const bson::bsonDecimal &dec, BOOLEAN invert);
      INT32 _appendDecimalEncoding(const bson::bsonDecimal &dec,
                                   BOOLEAN invert);
      INT32 _appendBsonValue(const bson::BSONElement &elem,
                             const bson::StringData *name,
                             BOOLEAN invert,
                             const stringTransformFn &f = nullptr);

      INT32 _appendStringLike(const bson::StringData &str, BOOLEAN invert);
      INT32 _appendBson(const bson::BSONObj &obj,
                        BOOLEAN invert,
                        const stringTransformFn &f = nullptr);
      INT32 _appendSmallDouble(FLOAT64 value,
                               DecimalContinuationMarker dcm,
                               BOOLEAN invert);
      INT32 _appendLargeDouble(FLOAT64 value,
                               DecimalContinuationMarker dcm,
                               BOOLEAN invert);
      INT32 _appendInteger(const INT64 num, BOOLEAN invert);
      INT32 _appendPreshiftedInteger(UINT64 value,
                                     BOOLEAN isNegative,
                                     BOOLEAN invert);

      INT32 _appendDoubleWithoutTypeBits(FLOAT64 num,
                                         DecimalContinuationMarker dcm,
                                         BOOLEAN invert);
      INT32 _appendHugeDecimalWithoutTypeBits(const bson::bsonDecimal &dec,
                                              BOOLEAN invert);
      INT32 _appendTinyDecimalWithoutTypeBits(const bson::bsonDecimal &dec,
                                              FLOAT64 bin,
                                              BOOLEAN invert);
      INT32 _appendEnd();
      INT32 _appendBytes(const void *source, UINT32 len, BOOLEAN invert);

      template <typename T>
      INT32 _append(const T &t, BOOLEAN invert)
      {
         return _appendBytes(&t, sizeof(t), invert);
      }

      INT32 _appendTypeBits();
      INT32 _appendMetaBlock();

      void _verifyStatus();
      void _transition(builderStatus to);
      INT32 _ensureBytes(UINT32 length);

   private:
      builderStatus _status = builderStatus::empty;
      typeBits _typeBits;
      CHAR *_buf = nullptr;
      UINT32 _bufSize = 0;
      UINT8 _sizeAheadElements = 0;
      UINT32 _capacity = 0;
      INT8 _elemCount = 0;
      Allocator _allocator;
   };

   template <typename Allocator> void keyStringBuilder<Allocator>::reset()
   {
      _status = builderStatus::empty;
      _typeBits.reset();
      if (nullptr != _buf)
      {
         _allocator.free(_buf);
         _buf = nullptr;
      }
      _bufSize = 0;
      _capacity = 0;
      _sizeAheadElements = 0;
      _elemCount = 0;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendBytes(const void *source,
                                                   UINT32 len,
                                                   BOOLEAN invert)
   {
      INT32 rc = SDB_OK;

      if (nullptr == source)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _ensureBytes(len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "skip bytes failed, rc:%d", rc);
         goto error;
      }

      if (invert)
      {
         CHAR *offset = _buf + _bufSize;
         const CHAR *in = static_cast<const CHAR *>(source);
         for (UINT32 i = 0; i < len; ++i)
         {
            *offset++ = ~(*in++);
         }
      }
      else
      {
         ossMemcpy(_buf + _bufSize, source, len);
      }
      _bufSize += len;

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_ensureBytes(UINT32 length)
   {
      INT32 rc = SDB_OK;
      UINT32 needSize = _capacity + length;
      UINT64 allocatedSize = 1ull << (64 - countLeadingZeros64(needSize - 1));

      if (nullptr == _buf)
      {
         _buf = static_cast<CHAR *>(_allocator.malloc(allocatedSize));
         if (nullptr == _buf)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }
         _capacity = allocatedSize;
      }
      else if ((_capacity - _bufSize) < length)
      {
         CHAR *ptr =
             static_cast<CHAR *>(_allocator.realloc(_buf, allocatedSize));
         if (nullptr == ptr)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }

         _buf = ptr;
         _capacity = allocatedSize;
      }

   done:
      return rc;
   error:
      goto done;
   }

   /////////////////////////////////////////////////////////////////////////////
   // keyStringBuilder begin
   template <typename Allocator>
   void keyStringBuilder<Allocator>::_verifyStatus()
   {
      SDB_ASSERT(_status == builderStatus::empty ||
                     _status == builderStatus::beforeElements ||
                     _status == builderStatus::appendingElements,
                 "Unexpected appending state");

      if (_status == builderStatus::empty)
      {
         _sizeAheadElements = 0;
         _transition(builderStatus::appendingElements);
      }
      else if (_status == builderStatus::beforeElements)
      {
         _sizeAheadElements = _bufSize;
         _transition(builderStatus::appendingElements);
      }
   }

   template <typename Allocator>
   void keyStringBuilder<Allocator>::_transition(builderStatus to)
   {
      {
         if (to == builderStatus::empty)
         {
            _status = to;
            return;
         }
         if (_status == to)
         {
            return;
         }

         switch (_status)
         {
         case builderStatus::empty:
            SDB_ASSERT(to == builderStatus::beforeElements ||
                           to == builderStatus::appendingElements,
                       "Invalid builder status");
            break;
         case builderStatus::beforeElements:
            SDB_ASSERT(to == builderStatus::appendingElements,
                       "Invalid builder status");
            break;
         case builderStatus::appendingElements:
            SDB_ASSERT(to == builderStatus::appendedTypeBits,
                       "Invalid builder status");
            break;
         case builderStatus::appendedTypeBits:
            SDB_ASSERT(to == builderStatus::appendedMetaBlock,
                       "Invalid builder status");
            break;
         default:
            SDB_ASSERT(FALSE, "Invalid builder status");
         } // switch (_state)
         _status = to;
      }
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendAllElements(
       const bson::BSONObj &obj,
       orderingWrapper ord,
       const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjIterator it(obj);
      UINT32 elemCount = 0;
      if (obj.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();

      while (it.more())
      {
         auto elem = it.next();
         BOOLEAN invert = ord.toBsonOrdering().get(elemCount) == -1;
         rc = appendBSONElement(elem, invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bson elements failed, rc:%d", rc);
            goto error;
         }
         elemCount += 1;
      }
      if (elemCount > ord.getNkeys())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "append bson elements failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendBSONElement(
       const bson::BSONElement &elem,
       BOOLEAN invert,
       const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;

      if (0 == elem.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _appendBsonValue(elem, nullptr, invert, f);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bson element failed, rc:%d", rc);
         goto error;
      }
      _elemCount++;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendBsonValue(
       const bson::BSONElement &elem,
       const bson::StringData *name,
       BOOLEAN invert,
       const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != elem.size(), "can not be zero");

      if (name)
      {
         rc = _appendBytes(
             name->data(), name->size() + 1, invert); // + 1 for NUL
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bson name failed, rc:%d", rc);
            goto error;
         }
      }

      switch (elem.type())
      {
      case bson::MinKey:
      case bson::MaxKey:
      case bson::EOO:
      case bson::Undefined:
      case bson::jstNULL: {
         rc = _append(bsonTypeToSupertype(elem.type()), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
         }
         break;
      }

      case bson::NumberDouble: {
         rc = _appendNumberDouble(elem._numberDouble(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::String: {
         rc = _appendString(elem.String(), invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Object: {
         rc = _appendObject(elem.Obj(), invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Array: {
         rc = _appendArray(bson::BSONArray(elem.Obj()), invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::BinData: {
         INT32 len;
         const CHAR *data = elem.binData(len);
         rc = _appendBinData(data, len, elem.binDataType(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::jstOID: {
         rc = _appendOID(elem.__oid(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Bool: {
         rc = _appendBool(elem.boolean(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Date: {
         rc = _appendDate(elem.date(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::RegEx: {
         rc = _appendRegex(elem.regex(), elem.regexFlags(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::DBRef: {
         rc = _appendDBRef(elem.dbrefNS(), elem.dbrefOID(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Symbol: {
         rc = _appendSymbol(elem.String(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Code: {
         rc = _appendCode(elem.code(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::CodeWScope: {
         rc = _appendCodeWScope(
             elem.codeWScopeCode(), elem.codeWScopeObject(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::NumberInt: {
         rc = _appendNumberInt(elem._numberInt(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::Timestamp: {
         rc = _appendTimestamp(elem.timestampTime(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::NumberLong: {
         rc = _appendNumberLong(elem._numberLong(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      case bson::NumberDecimal: {
         rc = _appendNumberDecimal(elem.Decimal(), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR,
                   "failed to append bson element[%d], rc:%d",
                   elem.type(),
                   rc);
            goto error;
         }
         break;
      }

      default: {
         rc = SDB_INVALIDARG;
         goto error;
      }
      } // switch(elem.type)
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendNumberInt(const INT32 num,
                                                       BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      rc = _typeBits.appendNumberInt();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append typebits, rc:%d", rc);
         goto error;
      }
      rc = _appendInteger(num, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append number int, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendNumberLong(const INT64 num,
                                                        BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      rc = _typeBits.appendNumberLong();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append typebits, rc:%d", rc);
         goto error;
      }
      rc = _appendInteger(num, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append number long, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendInteger(const INT64 num,
                                                     BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      const BOOLEAN isNegative = num < 0;
      const UINT64 magnitude = isNegative ? -num : num;
      if (num == std::numeric_limits<INT64>::min())
      {
         FLOAT64 doubleVal = static_cast<FLOAT64>(num);
         SDB_ASSERT(doubleVal == minLargeFloat64, "must be equal");
         rc = _appendLargeDouble(
             doubleVal, DecimalContinuationMarker::hasContinuation, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append large double", rc);
            goto error;
         }
      }
      if (num == 0)
      {
         rc = _append(EncodedType::numericZero, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append encoded type", rc);
            goto error;
         }
      }
      rc = _appendPreshiftedInteger(magnitude << 1, isNegative, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append integer", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendPreshiftedInteger(
       UINT64 value, BOOLEAN isNegative, BOOLEAN invert)
   {
      SDB_ASSERT(value != 0ULL, "Unexcepted value");
      SDB_ASSERT(value != 1ULL, "Unexcepted value");

      INT32 rc = SDB_OK;
      const UINT32 bytesNeeded = (64 - countLeadingZeros64(value) + 7) / 8;

      // Append the low bytes of value in big endian order.
      value = nativeToBigEndian(value);
      const void *firstUsedByte =
          reinterpret_cast<const char *>((&value) + 1) - bytesNeeded;

      if (isNegative)
      {
         rc = _append(
             static_cast<UINT8>(
                 static_cast<UINT8>(EncodedType::numericNegative1ByteInt) -
                 (bytesNeeded - 1)),
             invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
            goto error;
         }
         rc = _appendBytes(firstUsedByte, bytesNeeded, !invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append bytes, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _append(
             static_cast<UINT8>(
                 static_cast<UINT8>(EncodedType::numericPositive1ByteInt) +
                 (bytesNeeded - 1)),
             invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
            goto error;
         }
         rc = _appendBytes(firstUsedByte, bytesNeeded, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append bytes, rc:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendNumberDouble(const FLOAT64 num,
                                                          BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      rc = _typeBits.appendNumberDouble();
      if (SDB_OK != rc)
      {
         PD_LOG(
             PDERROR, "failed to append number FLOAT64 to typebits, rc:%d", rc);
         goto error;
      }
      if (0.0 == num)
      {
         std::signbit(num) ? _typeBits.appendNegativeZero()
                           : _typeBits.appendPositiveZero();
      }
      rc = _appendDoubleWithoutTypeBits(
          num, DecimalContinuationMarker::hasNoContinuation, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append double", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendDoubleWithoutTypeBits(
       FLOAT64 num, DecimalContinuationMarker dcm, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      const BOOLEAN isNegative = num < 0.0;
      const FLOAT64 magnitude = isNegative ? -num : num;

      if (!(magnitude >= 1.0))
      {
         if (magnitude > 0.0)
         {
            rc = _appendSmallDouble(num, dcm, invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to append small double, rc:%d", rc);
               goto error;
            }
         }
         else if (num == 0.0)
         {
            rc = _append(EncodedType::numericZero, invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
               goto error;
            }
         }
         else
         {
            SDB_ASSERT(std::isnan(num), "value must be NaN");
            rc = _append(EncodedType::numericNaN, invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
               goto error;
            }
         }
         goto done;
      }
      if (magnitude < minLargeFloat64)
      {
         UINT64 integerPart = static_cast<UINT64>(magnitude);
         if (static_cast<FLOAT64>(integerPart) == magnitude &&
             dcm == DecimalContinuationMarker::hasNoContinuation)
         {
            // No fractional part
            rc = _appendPreshiftedInteger(integerPart << 1, isNegative, invert);
            if (SDB_OK != rc)
            {
               PD_LOG(
                   PDERROR, "failed to append preshifted integer, rc:%d", rc);
               goto error;
            }
            goto done;
         }

         const UINT32 fractionalBytes =
             countLeadingZeros64(integerPart << 1) / 8;
         const UINT8 type =
             isNegative
                 ? static_cast<UINT8>(EncodedType::numericNegative8ByteInt) +
                       fractionalBytes
                 : static_cast<UINT8>(EncodedType::numericPositive8ByteInt) -
                       fractionalBytes;
         rc = _append(type, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
            goto error;
         }
         UINT64 encoding =
             static_cast<UINT64>(magnitude * pow256[fractionalBytes]);
         SDB_ASSERT(encoding == magnitude * pow256[fractionalBytes],
                    "must be equal");
         encoding += (integerPart + 1) << (fractionalBytes * 8);
         SDB_ASSERT((encoding & 0x3ULL) == 0, "must be zero");
         encoding |= static_cast<UINT8>(dcm);
         encoding = nativeToBigEndian(encoding);
         rc = _append(encoding, isNegative ? !invert : invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append FLOAT64 encoding, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         _appendLargeDouble(num, dcm, invert);
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendSmallDouble(
       FLOAT64 value, DecimalContinuationMarker dcm, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      BOOLEAN isNegative = value < 0;
      UINT64 encoding = 0;
      FLOAT64 magnitude = isNegative ? -value : value;
      SDB_ASSERT(!std::isnan(value) && value != 0 && magnitude < 1,
                 "Unexcepted value");

      rc = _append(isNegative ? EncodedType::numericNegativeSmallMagnitude
                              : EncodedType::numericPositiveSmallMagnitude,
                   invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
         goto error;
      }

      memcpy(&encoding, &magnitude, sizeof(encoding));
      encoding <<= 1;
      encoding |= static_cast<UINT8>(dcm);
      rc = _append(nativeToBigEndian(encoding), isNegative ? !invert : invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded bytes, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendLargeDouble(
       FLOAT64 value, DecimalContinuationMarker dcm, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      BOOLEAN isNegative = value < 0;
      SDB_ASSERT(!std::isnan(value), "Can not be NaN");
      SDB_ASSERT(value != 0.0, "Can not be zero");
      rc = _append(isNegative ? EncodedType::numericNegativeLargeMagnitude
                              : EncodedType::numericPositiveLargeMagnitude,
                   invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
         goto error;
      }
      UINT64 encoding;
      memcpy(&encoding, &value, sizeof(encoding));
      if (std::isfinite(value))
      {
         encoding <<= 1;
         encoding |= static_cast<UINT8>(dcm);
      }
      else
      {
         encoding = ~0ULL; // infinity
      }
      encoding = nativeToBigEndian(encoding);
      rc = _append(encoding, isNegative ? !invert : invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded bytes, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendNumberDecimal(
       const bson::bsonDecimal &dec, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      std::stringstream ss;
      bson::bsonDecimal decFromDouble;
      if (dec.isZero())
      {
         _typeBits.appendNumberDecimal();
         _append(EncodedType::numericZero, invert);
         goto done;
      }
      FLOAT64 floorDouble;
      dec.toDouble(&floorDouble);
      ss << std::setprecision(DOUBLE_PRECISION_10) << floorDouble;
      decFromDouble.fromString(ss.str().c_str());

      rc = _typeBits.appendNumberDecimal();
      if (SDB_OK != rc)
      {
         PD_LOG(
             PDERROR, "failed to append number decimal to typebits, rc:%d", rc);
         goto error;
      }
      rc = _typeBits.appendDecimalMeta(dec);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR,
                "failed to append number decimal meta to typebits, rc:%d",
                rc);
         goto error;
      }

      if (decFromDouble.compare(dec) == 0)
      {
         rc = _appendDoubleWithoutTypeBits(
             floorDouble, DecimalContinuationMarker::hasNoContinuation, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append double, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         if (decFromDouble.compare(dec) > 0)
         {
            *(UINT64 *)(&floorDouble) -= 1;
            std::stringstream ss;
            ss << std::setprecision(DOUBLE_PRECISION_10) << floorDouble;
            bson::bsonDecimal decFromDouble;
            decFromDouble.fromString(ss.str().c_str());
            SDB_ASSERT(decFromDouble.compare(dec) < 0,
                       "Excepted value that rounded toward zero");
         }
         rc = _appendDoubleWithoutTypeBits(
             floorDouble, DecimalContinuationMarker::hasContinuation, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append double, rc:%d", rc);
            goto error;
         }
         rc = _appendDecimalEncoding(dec, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append decimal encoding, rc:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendDecimalEncoding(
       const bson::bsonDecimal &dec, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      INT32 weight = dec.getWeight();
      INT32 ndigit = dec.getNdigit();
      const INT16 *digits = dec.getDigits();
      UINT32 integerPartNDigit = 0;
      invert = dec.getSign() == SDB_DECIMAL_NEG ? !invert : invert;
      if (weight >= 0)
      {
         integerPartNDigit = weight + 1;
      }
      else
      {
         integerPartNDigit = 0;
      }
      rc = _append(nativeToBigEndian(integerPartNDigit), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append integer part ndigit, rc:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < integerPartNDigit; ++i)
      {
         rc = _append(nativeToBigEndian(digits[i]), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append integer part digits, rc:%d", rc);
            goto error;
         }
      }

      if (weight < 0)
      {
         UINT16 fractionPartNdigit = static_cast<UINT16>(-weight);
         rc = _append(nativeToBigEndian(fractionPartNdigit), !invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append fraction part ndigit, rc:%d", rc);
            goto error;
         }
      }

      for (UINT32 i = integerPartNDigit; i < ndigit; ++i)
      {
         UINT16 absDigit = digits[i] << 1;
         if (i == ndigit - 1)
         {
            absDigit |= 0b1;
         }
         rc = _append(nativeToBigEndian(static_cast<UINT16>(absDigit)), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append fraction part digits, rc:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   /// non-numeric types
   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendBool(BOOLEAN val, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;

      _verifyStatus();
      rc = _append(val ? EncodedType::booleanTrue : EncodedType::booleanFalse,
                   invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bool value failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendDate(const bson::Date_t &val,
                                                  BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendTimestamp(INT64 val,
                                                       BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendOID(const bson::OID &val,
                                                 BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendSymbol(const bson::StringData &val,
                                                    BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendCode(const bson::StringData &val,
                                                  BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendCodeWScope(
       const bson::StringData &code, const bson::BSONObj &scope, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendBinData(const CHAR *data,
                                                     UINT32 dataSize,
                                                     bson::BinDataType type,
                                                     BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendRegex(
       const bson::StringData &regex,
       const bson::StringData &flags,
       BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendDBRef(
       const bson::StringData &dbrefNS,
       const bson::OID &dbrefOID,
       BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendArray(const bson::BSONArray &val,
                                                   BOOLEAN invert,
                                                   const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(FALSE, "todo");

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendString(const bson::StringData &val,
                                                    BOOLEAN invert,
                                                    const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;

      if (nullptr == val.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::stringLike, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
         goto error;
      }

      if (f)
      {
         rc = _appendStringLike(f(val), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append string data failed ,rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _appendStringLike(val, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append string data failed ,rc:%d", rc);
            goto error;
         }
      }

      _typeBits.appendString();

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendObject(const bson::BSONObj &val,
                                                    BOOLEAN invert,
                                                    const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;

      if (val.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::object, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded type object, rc:%d", rc);
         goto error;
      }

      rc = _appendBson(val, invert, f);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append object data failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendStringLike(
       const bson::StringData &str, BOOLEAN invert)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(builderStatus::appendingElements == _status,
                 "unexpected state");
      const CHAR *data = nullptr;

      if (nullptr == str.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      data = str.data();
      while (TRUE)
      {
         INT32 firstNul = bson::strnlen(str.data(), str.size());
         if (-1 == firstNul)
         {
            firstNul = str.size();
         }

         rc = _appendBytes(data, static_cast<UINT32>(firstNul), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bytes failed, rc:%d", rc);
            goto error;
         }

         if (firstNul == static_cast<INT32>(str.size()) ||
             firstNul == static_cast<INT32>(std::string::npos))
         {
            rc = _append(static_cast<INT8>(0), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "append bytes failed, rc:%d", rc);
               goto error;
            }

            break;
         }

         rc = _appendBytes("\0x00\0xff", 2, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bytes failed, rc:%d", rc);
            goto error;
         }

         data = data + firstNul + 1;
      }

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendBson(const bson::BSONObj &obj,
                                                  BOOLEAN invert,
                                                  const stringTransformFn &f)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(builderStatus::appendingElements == _status,
                 "unexpected state");
      SDB_ASSERT(!obj.isEmpty(), "can not be empty");

      bson::BSONObjIterator itr = bson::BSONObjIterator(obj);
      while (itr.more())
      {
         bson::BSONElement elem = itr.next();
         switch (elem.type())
         {
         case bson::MinKey:
         case bson::MaxKey:
         case bson::EOO:
         case bson::Undefined:
         case bson::jstNULL: {
            rc = _append(bsonTypeToSupertype(elem.type()), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
            }
            break;
         }

         case bson::NumberDouble: {
            rc = _appendNumberDouble(elem._numberDouble(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::String: {
            rc = _appendString(elem.String(), invert, f);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Object: {
            rc = _appendObject(elem.Obj(), invert, f);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Array: {
            rc = _appendArray(bson::BSONArray(elem.Obj()), invert, f);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::BinData: {
            INT32 len;
            const CHAR *data = elem.binData(len);
            rc = _appendBinData(data, len, elem.binDataType(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::jstOID: {
            rc = _appendOID(elem.__oid(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Bool: {
            rc = _appendBool(elem.boolean(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Date: {
            rc = _appendDate(elem.date(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::RegEx: {
            rc = _appendRegex(elem.regex(), elem.regexFlags(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::DBRef: {
            rc = _appendDBRef(elem.dbrefNS(), elem.dbrefOID(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Symbol: {
            rc = _appendSymbol(elem.String(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Code: {
            rc = _appendCode(elem.code(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::CodeWScope: {
            rc = _appendCodeWScope(
                elem.codeWScopeCode(), elem.codeWScopeObject(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::NumberInt: {
            rc = _appendNumberInt(elem._numberInt(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::Timestamp: {
            rc = _appendTimestamp(elem.timestampTime(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::NumberLong: {
            rc = _appendNumberLong(elem._numberLong(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         case bson::NumberDecimal: {
            rc = _appendNumberDecimal(elem.Decimal(), invert);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR,
                      "failed to append bson element[%d], rc:%d",
                      elem.type(),
                      rc);
               goto error;
            }
            break;
         }

         default: {
            rc = SDB_INVALIDARG;
            goto error;
         }
         } // switch(elem.type)
      }
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendTypeBits()
   {
      INT32 rc = SDB_OK;
      const UINT32 bufSize = _typeBits.getBufSize();
      rc = _appendBytes(_typeBits.getBuf(), bufSize, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append typebits buffer, rc:%d", rc);
         goto error;
      }
      _transition(builderStatus::appendedTypeBits);
   done:
      return rc;
   error:
      goto done;
   }

   // MetaBlock
   // TypeBits size: 1 or 4 bytes
   // key string size: 1, 2 or 4 bytes
   // Size ahead key string: 0 or 1 byte
   // metaByte: 1 byte whose 1 bit to indicate size ahead key string, 1 bit to
   // indicate size of key string and 1 bit to indicate size of TypeBits.
   // version: 1 byte.
   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendMetaBlock()
   {
      INT32 rc = SDB_OK;
      UINT32 keyBytesNeeded = _bufSize > 0xff ? 4 : 1;
      UINT32 typeBitsBytesNeeded = _typeBits.getBufSize() > 0xff ? 4 : 1;

      UINT8 metaByte = 0;
      metaByte |= (_sizeAheadElements != 0 ? 1 : 0);
      metaByte |= ((_bufSize > 0xff ? 1 : 0) << 1);
      metaByte |= ((_typeBits.getBufSize() > 0xff ? 1 : 0) << 2);

      if (typeBitsBytesNeeded == 1)
      {
         rc = _append(static_cast<UINT8>(_typeBits.getBufSize()), FALSE);
      }
      else
      {
         rc = _append(static_cast<UINT32>(_typeBits.getBufSize()), FALSE);
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append typebits size, rc:%d", rc);
         goto error;
      }

      if (keyBytesNeeded == 1)
      {
         rc = _append(static_cast<UINT8>(_bufSize), FALSE);
      }
      else
      {
         rc = _append(static_cast<UINT32>(_bufSize), FALSE);
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append key string size, rc:%d", rc);
         goto error;
      }

      if (_sizeAheadElements != 0)
      {
         rc = _append(static_cast<UINT8>(_sizeAheadElements), FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append key string size, rc:%d", rc);
            goto error;
         }
      }

      rc = _append(metaByte, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append metaByte, rc:%d", rc);
         goto error;
      }

      rc = _append(getVersion(), FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append version, rc:%d", rc);
         goto error;
      }
      _transition(builderStatus::appendedMetaBlock);
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   keyString keyStringBuilder<Allocator>::getKeyString()
   {
      slice data(_bufSize, _buf);
      return keyString(data);
   }

   template <typename Allocator> INT32 keyStringBuilder<Allocator>::done()
   {
      INT32 rc = SDB_OK;
      rc = _appendTypeBits();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append typebits, rc:%d", rc);
         goto error;
      }
      rc = _appendMetaBlock();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append meta block, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine

#endif