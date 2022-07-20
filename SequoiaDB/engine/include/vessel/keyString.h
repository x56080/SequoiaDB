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
#include "vessel/keyStringMetaBlock.h"
#include "../bson/bsonobj.h"
#include "vessel/keyStringDef.h"

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
      ~typeBitsReader();
      typeBitsReader operator=(const typeBitsReader &) = delete;
      typeBitsReader(const typeBitsReader &) = delete;

   public:
      typeBitsType readNumeric();
      typeBitsType readZero();
      typeBitsType readStringLike();
      void readBitsAndAssign(CHAR *dst, UINT32 bytesSize);

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

         ///WARNING: shallow copy!
         keyString(const keyString &);
         keyString &operator=(const keyString &);

      keyString(keyString &&);
      keyString &operator=(keyString &&);

      public:
         OSS_INLINE BOOLEAN isValid() const
         {
            return _ref.isValid() && _block.isValid();
         }
         OSS_INLINE BOOLEAN isOwned()const {return nullptr != _bufferOwned;}

      OSS_INLINE const slice &getDataSlice() const
      {
         return _ref;
      }

      public:
         void reset();
         INT32 init(const slice &s);
         INT32 getOwned();
      
      private:
         void _adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize);

   public:
      slice getKeySlice() const;
      slice getSliceFromKeyTo(UINT32 bytesAfterKey) const;
      slice getSliceBeforeKey() const;
      slice getSliceAfterKey() const;
      slice getTypeBits() const;
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           BOOLEAN withFieldName = FALSE);
      bson::BSONObj toBSON(const bson::BSONObj &pattern,
                           bson::BSONObjBuilder &builder,
                           BOOLEAN withFieldName = FALSE);

   public:
      INT32 compare(const keyString &ks) const;

   private:
      template <typename T> T _read(UINT32 &offset, BOOLEAN inverted);
      void _toBsonValue(EncodedType type,
                        UINT32 &offset,
                        BOOLEAN inverted,
                        bson::BSONObjBuilder &builder,
                        typeBitsReader &typeReader,
                        const CHAR *fieldName = nullptr);
      void _toNumeric(EncodedType type,
                      UINT32 &offset,
                      BOOLEAN inverted,
                      bson::BSONObjBuilder &builder,
                      typeBitsReader &typeReader,
                      const CHAR *fieldName = nullptr);
      bson::bsonDecimal _decodeDecimal(UINT32 &offset,
                                       BOOLEAN isNegative,
                                       BOOLEAN inverted);

      protected:
         slice _ref;  

      private:
         CHAR *_bufferOwned = nullptr;
         UINT32 _bufferSize = 0;
         keyStringMetaBlock _block;
   };//class keyString
} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_H_
