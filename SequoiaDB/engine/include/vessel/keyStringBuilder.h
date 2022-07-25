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

   Source File Name = keyStringBuilder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/11/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef KEY_SLICE_BUILDER_H_
#define KEY_SLICE_BUILDER_H_

#include "vessel/keyStringDef.h"
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
#include "vessel/keyString.h"
#include "vessel/recordID.h"
#include "inclusiveVec.h"
#include "vessel/globalIndexID.h"
#include "dpsDef.hpp"

#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>
namespace engine
{
namespace vessel
{
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
       std::numeric_limits<FLOAT64>::max_digits10;

   EncodedType bsonTypeToSupertype(bson::BSONType type);

   INT32 countLeadingZeros64(UINT64 num);

   class typeBitsBuilder : public SDBObject
   {
   public:
      typeBitsBuilder() = default;
      ~typeBitsBuilder();
      typeBitsBuilder operator=(const typeBitsBuilder &) = delete;
      typeBitsBuilder(const typeBitsBuilder &) = delete;

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

      OSS_INLINE BOOLEAN isEmpty() const
      {
         return nullptr == _buf || 0 == _bufSize;
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
      enum class BUILDER_STATUS : UINT16
      {
         EMPTY = 0,
         BEFORE_ELEMENTS = 1,
         APPENDING_ELEMENTS = 2,
         AFTER_ELEMENTS = 3,
         DONE = 4
      };

   public:
      using stringTransformFn =
          std::function<std::string(const bson::StringData &)>;
      void reset();
      void resetTypeBits(const typeBitsBuilder &tb);
      INT32 appendBSONElement(const bson::BSONElement &elem,
                              BOOLEAN isDescending = FALSE);
      INT32 appendAllElements(const bson::BSONObj &obj,
                              const orderingWrapper &o,
                              Discriminator d = Discriminator::INCLUSIVE);
      template <
          typename T,
          class = typename std::enable_if<std::is_unsigned<T>::value>::type>
      INT32 appendUnsignedWithoutType(const T &val,
                                      BOOLEAN isDescending = FALSE)
      {
         INT32 rc = SDB_OK;
         if (BUILDER_STATUS::EMPTY == _status ||
             BUILDER_STATUS::BEFORE_ELEMENTS == _status)
         {
            _transition(BUILDER_STATUS::BEFORE_ELEMENTS);
         }
         else if (BUILDER_STATUS::APPENDING_ELEMENTS == _status)
         {
            _transition(BUILDER_STATUS::AFTER_ELEMENTS);
         }

         rc = _append(ossNativeToBigEndian(val), isDescending);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append unsigned fixed field, rc:%d", rc);
            goto error;
         }
         SDB_ASSERT(_sizeAheadElements + sizeof(val) <= 0xff,
                    "size ahead key string must be equal or less than 255");
         if (BUILDER_STATUS::BEFORE_ELEMENTS == _status)
         {
            _sizeAheadElements += sizeof(val);
         }
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
         T mask = std::numeric_limits<T>::min();
         T tmp = val;
         if (BUILDER_STATUS::DONE == _status)
         {
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            PD_LOG(PDERROR, "building process has already done");
            goto error;
         }
         else if (BUILDER_STATUS::EMPTY == _status)
         {
            _transition(BUILDER_STATUS::BEFORE_ELEMENTS);
         }
         else if (BUILDER_STATUS::APPENDING_ELEMENTS == _status)
         {
            _transition(BUILDER_STATUS::AFTER_ELEMENTS);
         }

         tmp ^= mask;
         rc = _append(ossNativeToBigEndian(tmp), isDescending);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append signed fixed field, rc:%d", rc);
            goto error;
         }
         SDB_ASSERT(_sizeAheadElements + sizeof(val) <= 0xff,
                    "size ahead key string must be equal or less than 255");
         if (BUILDER_STATUS::BEFORE_ELEMENTS == _status)
         {
            _sizeAheadElements += sizeof(val);
         }
      done:
         return rc;
      error:
         reset();
         goto done;
      }

      INT32 appendRid(const recordID &rid, BOOLEAN force = FALSE);
      INT32 appendIndexId(const globalIndexID &indexId, BOOLEAN force = FALSE);
      INT32 appendLSN(UINT64 lsn, BOOLEAN force = FALSE);

      INT32 buildPredicate(const ossPoolVector<const BSONElement *> &elements,
                           const orderingWrapper &o,
                           const inclusiveVec &iv,
                           BOOLEAN forward);

      INT32 buildPredicate(const bson::BSONObj &key,
                           const orderingWrapper &o,
                           const inclusiveVec &iv,
                           BOOLEAN forward);

      INT32 buildIndexEntryKey(const bson::BSONObj &key,
                               const orderingWrapper &o,
                               const recordID &rid,
                               const globalIndexID *indexid = nullptr,
                               const UINT64 *lsn = nullptr);

   public:
      INT32 done();
      UINT8 getVersion() const
      {
         return KEY_STRING_VERSION_1;
      }
      keyString getShallowKeyString() const;
      keyString reap();

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
      INT32 _appendBytes(const void *source, UINT32 len, BOOLEAN invert);

      template <typename T> INT32 _append(const T &t, BOOLEAN invert)
      {
         return _appendBytes(&t, sizeof(t), invert);
      }

      INT32 _appendDiscriminator(Discriminator d);
      INT32 _appendTypeBits();
      INT32 _appendMetaBlock();

      void _verifyStatus();
      void _transition(BUILDER_STATUS to);
      INT32 _ensureBytes(UINT32 length);

   private:
      BUILDER_STATUS _status = BUILDER_STATUS::EMPTY;
      typeBitsBuilder _typeBits;
      CHAR *_buf = nullptr;
      UINT32 _bufSize = 0;
      UINT32 _sizeAheadElements = 0;
      UINT32 _sizeOfElements = 0;
      UINT32 _capacity = 0;
      Allocator _allocator;
   };

   using STACK_KEY_STRING_BUILDER = keyStringBuilder<utilStackAllocator<>>;
   using KEY_STRING_BUILDER = keyStringBuilder<utilPoolAllocator>;

   template <typename Allocator> void keyStringBuilder<Allocator>::reset()
   {
      _status = BUILDER_STATUS::EMPTY;
      _typeBits.reset();
      if (nullptr != _buf)
      {
         _allocator.free(_buf);
         _buf = nullptr;
      }
      _bufSize = 0;
      _capacity = 0;
      _sizeAheadElements = 0;
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
         ossMemcpyFlipBits(_buf + _bufSize, source, len);
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

      if (nullptr == _buf)
      {
         _buf = static_cast<CHAR *>(_allocator.malloc(needSize));
         if (nullptr == _buf)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }
         _capacity = needSize;
      }
      else if ((_capacity - _bufSize) < length)
      {
         CHAR *ptr = static_cast<CHAR *>(_allocator.realloc(_buf, needSize));
         if (nullptr == ptr)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }

         _buf = ptr;
         _capacity = needSize;
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
      SDB_ASSERT(_status == BUILDER_STATUS::EMPTY ||
                     _status == BUILDER_STATUS::BEFORE_ELEMENTS ||
                     _status == BUILDER_STATUS::APPENDING_ELEMENTS,
                 "Unexpected appending state");
      if (_status == BUILDER_STATUS::EMPTY)
      {
         _sizeAheadElements = 0;
         _transition(BUILDER_STATUS::APPENDING_ELEMENTS);
      }
      else if (_status == BUILDER_STATUS::BEFORE_ELEMENTS)
      {
         _sizeAheadElements = _bufSize;
         _transition(BUILDER_STATUS::APPENDING_ELEMENTS);
      }
   }

   template <typename Allocator>
   void keyStringBuilder<Allocator>::_transition(BUILDER_STATUS to)
   {
      {
         if (to == BUILDER_STATUS::EMPTY)
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
         case BUILDER_STATUS::EMPTY:
            SDB_ASSERT(to == BUILDER_STATUS::BEFORE_ELEMENTS ||
                           to == BUILDER_STATUS::APPENDING_ELEMENTS ||
                           to == BUILDER_STATUS::DONE,
                       "Invalid builder status");
            break;
         case BUILDER_STATUS::BEFORE_ELEMENTS:
            SDB_ASSERT(to == BUILDER_STATUS::APPENDING_ELEMENTS ||
                           to == BUILDER_STATUS::DONE,
                       "Invalid builder status");
            break;
         case BUILDER_STATUS::APPENDING_ELEMENTS:
            SDB_ASSERT(to == BUILDER_STATUS::AFTER_ELEMENTS ||
                           to == BUILDER_STATUS::DONE,
                       "Invalid builder status");
            break;
         case BUILDER_STATUS::AFTER_ELEMENTS:
            SDB_ASSERT(to == BUILDER_STATUS::DONE, "Invalid builder status");
            break;
         default:
            SDB_ASSERT(FALSE, "Invalid builder status");
         } // switch (_status)
         _status = to;
      }
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendAllElements(
       const bson::BSONObj &obj, const orderingWrapper &o, Discriminator d)
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
         BOOLEAN invert = o.toBsonOrdering().get(elemCount) == -1;
         rc = appendBSONElement(elem, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bson elements failed, rc:%d", rc);
            goto error;
         }
         elemCount += 1;
      }
      if (elemCount > o.getNkeys())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "append bson elements failed, rc:%d", rc);
         goto error;
      }
      rc = _appendDiscriminator(d);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append discriminator, rc:%d", rc);
         goto error;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements;

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendBSONElement(
       const bson::BSONElement &elem, BOOLEAN isDescending)
   {
      INT32 rc = SDB_OK;

      if (0 == elem.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _appendBsonValue(elem, nullptr, isDescending);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bson element failed, rc:%d", rc);
         goto error;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements;
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
         rc = _appendTimestamp(elem._opTime().asDate(), invert);
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
         SDB_ASSERT(-doubleVal == minLargeFloat64, "must be equal");
         rc = _appendLargeDouble(
             doubleVal, DecimalContinuationMarker::hasNoContinuation, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append large double", rc);
            goto error;
         }
         goto done;
      }
      else if (num == 0)
      {
         rc = _append(EncodedType::numericZero, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append encoded type", rc);
            goto error;
         }
         goto done;
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
      SDB_ASSERT(value != 0ULL, "Unexpected value");
      SDB_ASSERT(value != 1ULL, "Unexpected value");

      INT32 rc = SDB_OK;
      const UINT32 bytesNeeded = (64 - countLeadingZeros64(value) + 7) / 8;

      // Append the low bytes of value in big endian order.
      value = ossNativeToBigEndian(value);
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
         encoding = ossNativeToBigEndian(encoding);
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
                 "Unexpected value");

      rc = _append(isNegative ? EncodedType::numericNegativeSmallMagnitude
                              : EncodedType::numericPositiveSmallMagnitude,
                   invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append encoded type, rc:%d", rc);
         goto error;
      }

      ossMemcpy(&encoding, &magnitude, sizeof(encoding));
      encoding <<= 1;
      encoding |= static_cast<UINT8>(dcm);
      rc = _append(ossNativeToBigEndian(encoding),
                   isNegative ? !invert : invert);
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
      ossMemcpy(&encoding, &value, sizeof(encoding));
      if (std::isfinite(value))
      {
         encoding <<= 1;
         encoding |= static_cast<UINT8>(dcm);
      }
      else
      {
         encoding = ~0ULL; // infinity
      }
      encoding = ossNativeToBigEndian(encoding);
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
      UINT32 ndigit = static_cast<UINT32>(dec.getNdigit());
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
      rc = _append(ossNativeToBigEndian(integerPartNDigit), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append integer part ndigit, rc:%d", rc);
         goto error;
      }

      for (UINT32 i = 0; i < integerPartNDigit; ++i)
      {
         rc = _append(ossNativeToBigEndian(digits[i]), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append integer part digits, rc:%d", rc);
            goto error;
         }
      }

      if (weight < 0)
      {
         UINT16 fractionPartNdigit = static_cast<UINT16>(-weight);
         rc = _append(ossNativeToBigEndian(fractionPartNdigit), !invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append fraction part ndigit, rc:%d", rc);
            goto error;
         }
      }

      for (UINT32 i = integerPartNDigit; i < ndigit; ++i)
      {
         UINT16 absDigit = digits[i] << 1;
         if (i != ndigit - 1)
         {
            absDigit |= 0b1;
         }
         rc = _append(ossNativeToBigEndian(static_cast<UINT16>(absDigit)),
                      invert);
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
      INT64 encoded = 0;
      _verifyStatus();
      rc = _append(EncodedType::date, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      encoded = val.millis ^ (1ull << 63);
      rc = _append(ossNativeToBigEndian(encoded), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append date value failed, rc:%d", rc);
         goto error;
      }

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
      _verifyStatus();
      rc = _append(EncodedType::timestamp, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _append(ossNativeToBigEndian(val), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append timestamp value failed, rc:%d", rc);
         goto error;
      }

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
      if (!val.isSet())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::oid, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _append(val, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append OID value failed, rc:%d", rc);
         goto error;
      }

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
      if (nullptr == val.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _typeBits.appendSymbol();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append type bits failed, rc:%d", rc);
         goto error;
      }

      rc = _append(EncodedType::stringLike, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _appendStringLike(val, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append symbol data failed ,rc:%d", rc);
         goto error;
      }

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
      if (nullptr == val.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::code, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _append(EncodedType::stringLike, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _appendStringLike(val, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append code data failed ,rc:%d", rc);
         goto error;
      }

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
      if (nullptr == code.data() || scope.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::codeWithScope, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _append(EncodedType::code, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      rc = _appendStringLike(code, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append code value failed, rc:%d");
         goto error;
      }

      rc = _appendBson(scope, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append code scope failed, rc:%d", rc);
         goto error;
      }

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
      if (nullptr == data || 0 == dataSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::binData, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
         goto error;
      }

      if (0xff > dataSize)
      {
         rc = _append(static_cast<UINT8>(dataSize), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append data size failed, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = _append(static_cast<INT8>(0xff), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bit failed, rc:%d", rc);
            goto error;
         }

         rc = _append(ossNativeToBigEndian(static_cast<UINT32>(dataSize)),
                      invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append data size failed, rc:%d", rc);
            goto error;
         }
      }

      rc = _append(type, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bin data type failed, rc:%d");
         goto error;
      }

      rc = _append(data, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bin data value failed, rc:%d");
         goto error;
      }

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
      if (nullptr == regex.data() || nullptr == flags.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::regEx, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d");
         goto error;
      }

      rc = _append(regex, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append regex value failed, rc:%d");
         goto error;
      }

      rc = _append(static_cast<INT8>(0), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bit failed, rc:%d");
         goto error;
      }

      rc = _append(flags, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append flags value failed, rc:%d");
         goto error;
      }

      rc = _append(static_cast<INT8>(0), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append bit failed, rc:%d");
         goto error;
      }

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
      if (nullptr == dbrefNS.data() || !dbrefOID.isSet())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::dbRef, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d");
         goto error;
      }

      rc = _append(ossNativeToBigEndian(static_cast<UINT32>(dbrefNS.size())),
                   invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append dbref namespace size failed, rc:%d");
         goto error;
      }

      rc = _append(dbrefNS.data(), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append dbref namespace value failed, rc:%d");
         goto error;
      }

      rc = _append(dbrefOID, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append dbref oid value failed, rc:%d");
         goto error;
      }

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
      bson::BSONObjIterator it;
      if (val.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _verifyStatus();
      rc = _append(EncodedType::array, invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append encoded type failed, rc:%d");
         goto error;
      }

      it = bson::BSONObjIterator(val);
      while (it.more())
      {
         bson::BSONElement e = it.next();
         rc = _appendBsonValue(e, nullptr, invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append bson element failed, rc:%d", rc);
            goto error;
         }
      }
      rc = _append(static_cast<UINT8>(0), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append Null, rc:%d", rc);
         goto error;
      }

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
      rc = _typeBits.appendString();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "append type bits failed, rc:%d", rc);
         goto error;
      }

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
      SDB_ASSERT(BUILDER_STATUS::APPENDING_ELEMENTS == _status,
                 "unexpected status");
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
      bson::BSONObjIterator it;
      _verifyStatus();
      SDB_ASSERT(!obj.isEmpty(), "can not be empty");

      if (obj.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      it = bson::BSONObjIterator(obj);
      while (it.more())
      {
         bson::BSONElement e = it.next();
         rc = _append(bsonTypeToSupertype(e.type()), invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append encoded type failed, rc:%d", rc);
            goto error;
         }

         bson::StringData name(e.fieldName());
         rc = _appendBsonValue(e, &name, invert, f);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "append element value failed, rc:%d", rc);
            goto error;
         }
      }
      rc = _append(static_cast<UINT8>(0), invert);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append Null, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::_appendDiscriminator(Discriminator d)
   {
      INT32 rc = SDB_OK;
      if (Discriminator::EXCLUSIVE_BEFORE == d)
      {
         rc = _append(DiscriminatorValue::LESS, FALSE);
      }
      else if (Discriminator::EXCLUSIVE_AFTER == d)
      {
         rc = _append(DiscriminatorValue::GREATER, FALSE);
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append Discriminator, rc:%d", rc);
         goto error;
      }
      // else Discriminator::Inclusive, No discriminator byte

      // append the end byte in all cases
      rc = _append(DiscriminatorValue::END, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append End, rc:%d", rc);
         goto error;
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
   done:
      return rc;
   error:
      goto done;
   }

   // MetaBlock
   // TypeBits size: 1 or 4 bytes
   // key string size: 1 or 4 bytes
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
         rc = _append(static_cast<UINT8>(_sizeOfElements), FALSE);
      }
      else
      {
         rc = _append(static_cast<UINT32>(_sizeOfElements), FALSE);
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
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   keyString keyStringBuilder<Allocator>::getShallowKeyString() const
   {
      SDB_ASSERT(BUILDER_STATUS::DONE == _status, "can not be invalid");
      return keyString(slice(_bufSize, _buf));
   }

   template <typename Allocator> keyString keyStringBuilder<Allocator>::reap()
   {
      SDB_ASSERT(BUILDER_STATUS::DONE == _status, "can not be invalid");
      SDB_ASSERT(_allocator.isMovable(), "must be movable");
      keyString ks;
      ks._adopt(_buf, _capacity, _bufSize);
      _buf = nullptr;
      reset();
      return std::move(ks);
   }

   template <typename Allocator> INT32 keyStringBuilder<Allocator>::done()
   {
      INT32 rc = SDB_OK;
      if (!_typeBits.isEmpty())
      {
         rc = _appendTypeBits();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append typebits, rc:%d", rc);
            goto error;
         }
      }

      rc = _appendMetaBlock();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append meta block, rc:%d", rc);
         goto error;
      }
      _transition(BUILDER_STATUS::DONE);
   done:
      return rc;
   error:
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendRid(const recordID &rid,
                                                BOOLEAN force)
   {
      INT32 rc = SDB_OK;
      UINT32 pid = INVALID_PAGE_ID;
      INT16 pos = -1;

      if (!force && !rid.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pid = rid.getPid();
      pos = rid.getPos();
      rc = appendUnsignedWithoutType(pid, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append pid:%d", rc);
         goto error;
      }

      rc = appendSignedWithoutType(pos, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append rid pos:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendIndexId(
       const globalIndexID &indexId, BOOLEAN force)
   {
      INT32 rc = SDB_OK;
      UINT32 val = 0;

      if (!force && !indexId.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      val = indexId.getLogicalCSID();
      rc = appendUnsignedWithoutType(val, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append logical cs id:%d", rc);
         goto error;
      }

      val = indexId.getLogicalCLID();
      rc = appendUnsignedWithoutType(val, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append logical cl id:%d", rc);
         goto error;
      }

      val = indexId.getLogicalIndexID();
      rc = appendUnsignedWithoutType(val, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append logical index id:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::appendLSN(UINT64 lsn, BOOLEAN force)
   {
      INT32 rc = SDB_OK;
      if (!force && DPS_INVALID_LSN_OFFSET == lsn)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = appendUnsignedWithoutType(lsn, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append lsn:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::buildPredicate(
       const ossPoolVector<const BSONElement *> &elements,
       const orderingWrapper &o,
       const inclusiveVec &iv,
       BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      if (elements.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      if (o.getNkeys() < elements.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      _verifyStatus();
      for (UINT32 i = 0;i < elements.size(); i++)
      {
         BOOLEAN invert = o.toBsonOrdering().get(i) == -1;
         rc = appendBSONElement(*elements[i], invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append bson elements , rc:%d", rc);
            goto error;
         }

         if (iv.isInclusive(i) && !forward)
         {
            rc = _append(DiscriminatorValue::GREATER, FALSE);
            break;
         }
         else if (iv.isExclusive(i) && forward)
         {
            rc = _append(DiscriminatorValue::GREATER, FALSE);
            break;
         }
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append discriminator, rc:%d", rc);
         goto error;
      }
      rc = _append(DiscriminatorValue::END, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append End, rc:%d", rc);
         goto error;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::buildPredicate(const bson::BSONObj &key,
                                                     const orderingWrapper &o,
                                                     const inclusiveVec &iv,
                                                     BOOLEAN forward)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjIterator it(key);
      UINT32 elemCount = 0;
      if (key.isEmpty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      _verifyStatus();
      while (it.more())
      {
         auto elem = it.next();
         BOOLEAN invert = o.toBsonOrdering().get(elemCount) == -1;
         rc = appendBSONElement(elem, invert);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append bson elements, rc:%d", rc);
            goto error;
         }

         elemCount += 1;
         if (iv.isInclusive(elemCount - 1) && !forward)
         {
            rc = _append(DiscriminatorValue::GREATER, FALSE);
            break;
         }
         else if (iv.isExclusive(elemCount - 1) && forward)
         {
            rc = _append(DiscriminatorValue::GREATER, FALSE);
            break;
         }
      }
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append discriminator, rc:%d", rc);
         goto error;
      }
      rc = _append(DiscriminatorValue::END, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append End, rc:%d", rc);
         goto error;
      }
      _sizeOfElements = _bufSize - _sizeAheadElements;
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   template <typename Allocator>
   INT32 keyStringBuilder<Allocator>::buildIndexEntryKey(
       const bson::BSONObj &key,
       const orderingWrapper &o,
       const recordID &rid,
       const globalIndexID *indexid,
       const UINT64 *lsn)
   {
      INT32 rc = SDB_OK;
      if (indexid)
      {
         rc = appendIndexId(*indexid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append index id, rc:%d", rc);
            goto error;
         }
      }

      rc = appendAllElements(key, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bson elements, rc:%d", rc);
         goto error;
      }

      rc = appendRid(rid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append record id, rc:%d", rc);
         goto error;
      }

      if (lsn)
      {
         rc = appendLSN(*lsn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append lsn, rc:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

} // namespace vessel
} // namespace engine

#endif