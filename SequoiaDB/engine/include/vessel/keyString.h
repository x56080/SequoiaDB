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

#include "vessel/slice.h"
#include "vessel/keyStringMetaBlock.h"
#include "../bson/bsonobj.h"

namespace engine
{
namespace vessel
{
   class keyString : public SDBObject
   {
      public:
         keyString() = default;
         ~keyString();
         explicit keyString(const slice &s);

         /// transfer buffer ownership to keyString
         explicit keyString(CHAR *buffer,
                            UINT32 bufferSize,
                            UINT32 ksSize);

         ///WARNING: shallow copy!
         keyString(const keyString &);
         keyString &operator=(const keyString &);

         keyString(keyString &&);
         keyString &operator=(keyString &&);

      public:
         OSS_INLINE BOOLEAN isValid() const
         {
            return _ref.isValid();
         }
         OSS_INLINE BOOLEAN isOwned()const {return nullptr != _bufferOwned;}

         OSS_INLINE const slice &getDataSlice() const
         {
            return _ref;
         }

      public:
         void reset();
         INT32 getOwned();
         void adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize);

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

      protected:
         slice _ref;  
         CHAR *_bufferOwned = nullptr;
         UINT32 _bufferSize = 0;
         keyStringMetaBlock _block;
   };//class keyString

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_H_
