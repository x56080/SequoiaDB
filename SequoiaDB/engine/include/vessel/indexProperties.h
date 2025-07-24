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

   Source File Name = indexProperties.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/16/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_PROPERTIES_H_
#define VESSEL_INDEX_PROPERTIES_H_


#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/indexKeyPattern.h"
#include "vessel/indexDef.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class indexProperties : public SDBObject
   {
      public:
         indexProperties() = default;
         ~indexProperties() = default;
         indexProperties(const indexProperties &o):
         _name(o._name),
         _pattern(o._pattern),
         _type(o._type),
         _flags(o._flags),
         _innerID(o._innerID)
         {
         }

         indexProperties &operator=(const indexProperties &o)
         {
            _name = o._name;
            _pattern = o._pattern;
            _type = o._type;
            _flags = o._flags;
            _innerID = o._innerID;

            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INDEX_TYPE_INVALID != _type;
         }

         OSS_INLINE const std::string &getName()const {return _name;}
         OSS_INLINE const indexKeyPattern &getPattern()const {return _pattern;}
         OSS_INLINE INDEX_TYPE getType()const {return _type;}
         OSS_INLINE utilIdxInnerID getInnerID()const {return _innerID;}

         INT32 init(const bson::BSONObj &obj);

         void reset();

         strSlice getNameSlice()const {return strSlice(_name.c_str(), _name.size());}

         void dump(bson::BSONObjBuilder &builder)const;

         OSS_INLINE BOOLEAN isUnique()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_UNIQUE);
         }
         OSS_INLINE BOOLEAN isEnforced()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_ENFORCED);           
         }
         OSS_INLINE BOOLEAN isNotNull()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_NOT_NULL);
         }
         OSS_INLINE BOOLEAN isNotArray()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_NOT_ARRAY);
         }

         /// btree only
         OSS_INLINE BOOLEAN isCompressionEnabled()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_COMPRESSION);
         }

      private:
         OSS_INLINE void setAsUnique()
         {
            OSS_BIT_SET(_flags, _FLAG_UNIQUE);
         }
         OSS_INLINE void setAsEnforced()
         {
            OSS_BIT_SET(_flags, _FLAG_ENFORCED);
         }
         OSS_INLINE void setAsNotNull()
         {
            OSS_BIT_SET(_flags, _FLAG_NOT_NULL);
         }
         OSS_INLINE void setAsNotArray()
         {
            OSS_BIT_SET(_flags, _FLAG_NOT_ARRAY);
         }
         OSS_INLINE void setCompressionEnabled()
         {
            OSS_BIT_SET(_flags, _FLAG_COMPRESSION);
         }
      private:
         static constexpr UINT16 _FLAG_UNIQUE = 0x01;
         static constexpr UINT16 _FLAG_ENFORCED = 0x02;
         static constexpr UINT16 _FLAG_NOT_NULL = 0x04;
         static constexpr UINT16 _FLAG_NOT_ARRAY = 0x08;
         static constexpr UINT16 _FLAG_COMPRESSION = 0x10;

      private:
         std::string _name;
         indexKeyPattern _pattern;
         INDEX_TYPE _type = INDEX_TYPE_INVALID;
         UINT16 _flags = 0;
         utilIdxInnerID _innerID = UTIL_UNIQUEID_NULL;
   };//class indexProperties
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_PROPERTIES_H_