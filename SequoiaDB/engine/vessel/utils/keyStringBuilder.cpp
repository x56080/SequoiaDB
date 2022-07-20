#include "ossErr.h"
#include "ossTypes.h"
#include "ossTypes.hpp"
#include "pd.hpp"
#include "vessel/keyStringBuilder.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>

using namespace bson;
namespace engine
{
namespace vessel
{
   INT32 countLeadingZeros64(UINT64 num)
   {
      int highbit = 0;
      for (UINT32 i = 1; i <= 63; ++i)
      {
         if (num >= (1 << i))
         {
            highbit = i;
         }
         else
         {
            break;
         }
      }
      return 64 - highbit - 1;
   }

   EncodedType bsonTypeToSupertype(bson::BSONType type)
   {
      switch (type)
      {
      case MinKey:
         return EncodedType::minKey;

      case EOO:
      case jstNULL:
         return EncodedType::nullish;

      case Undefined:
         return EncodedType::undefined;

      case NumberDecimal:
      case NumberDouble:
      case NumberInt:
      case NumberLong:
         return EncodedType::numeric;

      case String:
      case Symbol:
         return EncodedType::stringLike;

      case Object:
         return EncodedType::object;
      case Array:
         return EncodedType::array;
      case BinData:
         return EncodedType::binData;
      case jstOID:
         return EncodedType::oid;
      case Bool:
         return EncodedType::boolean;
      case Date:
         return EncodedType::date;
      case Timestamp:
         return EncodedType::timestamp;
      case RegEx:
         return EncodedType::regEx;
      case DBRef:
         return EncodedType::dbRef;

      case Code:
         return EncodedType::code;
      case CodeWScope:
         return EncodedType::codeWithScope;

      case MaxKey:
         return EncodedType::maxKey;
      }
      SDB_ASSERT(FALSE, "Unexcepted bson type");
   }

   /////////////////////////////////////////////////////////////////////////////
   // typeBitsBuilder begin
   typeBitsBuilder::~typeBitsBuilder()
   {
      reset();
   }

   void typeBitsBuilder::reset()
   {
      _curBit = 0;
      if (nullptr != _buf)
      {
         _allocator.free(_buf);
         _buf = nullptr;
      }
      _bufSize = 0;
      _capacity = 0;
   }

   INT32 typeBitsBuilder::appendBit(UINT8 oneOrZero)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(oneOrZero == 0 || oneOrZero == 1,
                 "Bit to append must be 1 or 0");
      const UINT32 byte = _curBit / 8;
      const UINT8 offsetInByte = _curBit % 8;
      if (offsetInByte == 0)
      {
         rc = _ensureBytes(1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "skip byte failed, rc:%d", rc);
            goto error;
         }
         _buf[byte] = oneOrZero << 7; // 0b10000000 or 0b00000000
         _bufSize += 1;
      }
      else
      {
         _buf[byte] |= (oneOrZero << (7 - offsetInByte));
      }
      _curBit++;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 typeBitsBuilder::appendString()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(stringBit);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendSymbol()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(symbolBit);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendNumberDouble()
   {
      INT32 rc = SDB_OK;
      rc = appendBit(doubleBits >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(doubleBits & 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", NumberInt, rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendNumberInt()
   {
      INT32 rc = SDB_OK;
      rc = appendBit(intBits >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(intBits & 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendNumberLong()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(longBits >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

      rc = appendBit(longBits & 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendNumberDecimal()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(decimalBits >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

      rc = appendBit(decimalBits & 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendPositiveZero()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(positiveZero);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendNegativeZero()
   {
      INT32 rc = SDB_OK;

      rc = appendBit(negativeZero);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::_ensureBytes(UINT32 length)
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

   INT32 typeBitsBuilder::appendBits(const CHAR *bytes, const UINT32 bytesSize)
   {
      INT32 rc = SDB_OK;
      for (UINT32 i = 0; i < bytesSize * 8; i++)
      {
         UINT8 byte = static_cast<UINT8>(bytes[i / 8]);
         UINT8 oneOrZero = (byte >> (7 - (i % 8))) & 0b00000001;
         rc = appendBit(oneOrZero);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append bit to typebits, rc%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 typeBitsBuilder::appendDecimalMeta(const bson::bsonDecimal &dec)
   {
      INT32 rc = SDB_OK;
      INT32 typemod = dec.getTypemod();
      INT16 ndigit = dec.getNdigit();
      INT16 weight = dec.getWeight();
      rc =
          appendBits(reinterpret_cast<const CHAR *>(&typemod), sizeof(typemod));
      if (SDB_OK != rc)
      {
         PD_LOG(
             PDERROR, "failed to append decimal typemod to typebits, rc%d", rc);
         goto error;
      }
      rc = appendBits(reinterpret_cast<const CHAR *>(&ndigit), sizeof(ndigit));
      if (SDB_OK != rc)
      {
         PD_LOG(
             PDERROR, "failed to append decimal ndigit to typebits, rc%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      reset();
      goto done;
   }


} // namespace vessel
} // namespace engine