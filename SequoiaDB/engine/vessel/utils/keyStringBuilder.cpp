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

   Source File Name = keyStringBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/11/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
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
         if (num >= (1ULL << i))
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
      case Timestamp:
         return EncodedType::time;
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
      default:
         SDB_ASSERT(FALSE, "Unexpected bson type");
         return EncodedType::numeric;
      }
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::STRING));
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::SYMBOL));
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

   INT32 typeBitsBuilder::appendDate()
   {
      INT32 rc = SDB_OK;
      rc = appendBit(static_cast<UINT8>(typeBitsType::DATE) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(static_cast<UINT8>(typeBitsType::DATE) & 1);
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

   INT32 typeBitsBuilder::appendTimestamp()
   {
      INT32 rc = SDB_OK;
      rc = appendBit(static_cast<UINT8>(typeBitsType::TIMESTAMP) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(static_cast<UINT8>(typeBitsType::TIMESTAMP) & 1);
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

   INT32 typeBitsBuilder::appendNumberDouble()
   {
      INT32 rc = SDB_OK;
      rc = appendBit(static_cast<UINT8>(typeBitsType::DOUBLE) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(static_cast<UINT8>(typeBitsType::DOUBLE) & 1);
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
      rc = appendBit(static_cast<UINT8>(typeBitsType::INT) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }
      rc = appendBit(static_cast<UINT8>(typeBitsType::INT) & 1);
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::LONG) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

      rc = appendBit(static_cast<UINT8>(typeBitsType::LONG) & 1);
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::DECIMAL) >> 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append bit, rc:%d", rc);
         goto error;
      }

      rc = appendBit(static_cast<UINT8>(typeBitsType::DECIMAL) & 1);
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::POSITIVE_ZERO));
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

      rc = appendBit(static_cast<UINT8>(typeBitsType::NEGATIVE_ZERO));
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

   INT32 typeBitsBuilder::appendBits(const UINT8 *bytes, const UINT32 bytesSize)
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
      rc = appendBits(reinterpret_cast<const UINT8 *>(&typemod),
                      sizeof(typemod));
      if (SDB_OK != rc)
      {
         PD_LOG(
             PDERROR, "failed to append decimal typemod to typebits, rc%d", rc);
         goto error;
      }
      rc = appendBits(reinterpret_cast<const UINT8 *>(&ndigit), sizeof(ndigit));
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