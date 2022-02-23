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

   Source File Name = indexDescription.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_DESCRIPTION_H_
#define VESSEL_INDEX_DESCRIPTION_H_


#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "vessel/indexKeyPattern.h"
#include "vessel/indexDef.h"

namespace engine
{
namespace vessel
{
   class indexDescription : public SDBObject
   {
      public:
         indexDescription(){}
         ~indexDescription(){}
         indexDescription(const indexDescription &) = delete;
         indexDescription &operator=(const indexDescription &o)
         {
            _name = o._name;
            _pattern = o._pattern;
            _pattern.getOwned();
            _type = o._type;
            _flags = o._flags;
            _btreeMaxPrefixFields = o._btreeMaxPrefixFields;
            _btreeMinCompressionDepth = o._btreeMinCompressionDepth;
            _columnFamily = o._columnFamily;
            return *this;
         }
         
      public:
         BOOLEAN isValid()const;
         void reset();

         strSlice getNameSlice()const;

         INT32 extractFromBson(const bson::BSONObj &obj);
         void exportToBson(bson::BSONObjBuilder &builder)const;


      public:
         OSS_INLINE const ossPoolString &getName()const
         {
            return _name;
         }
         OSS_INLINE const indexKeyPattern &getPattern()const
         {
            return _pattern;
         }
         OSS_INLINE INDEX_TYPE getType()const
         {
            return _type;
         }
         OSS_INLINE utilIdxInnerID getInnerID()const
         {
            return _innerID;
         }
         OSS_INLINE UINT32 getMaxPrefixFields()const
         {
            return _btreeMaxPrefixFields;
         }
         OSS_INLINE UINT32 getMinCompressionDepth()const
         {
            return _btreeMinCompressionDepth;
         }
         OSS_INLINE BOOLEAN isBtreeIndex()const
         {
            return INDEX_TYPE_BTREE == _type;
         }
         OSS_INLINE BOOLEAN isLsmIndex()const
         {
            return INDEX_TYPE_LSM == _type;
         }

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
         OSS_INLINE BOOLEAN isPrefixCompressionEnabled()const
         {
            return 0 != OSS_BIT_TEST(_flags, _FLAG_BTREE_COMPRESSION);
         }

      public:
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
         OSS_INLINE void setAsPrefixCompressionEnabled()
         {
            OSS_BIT_SET(_flags, _FLAG_BTREE_COMPRESSION);
         }
      
      private:
         static constexpr UINT32 _FLAG_UNIQUE = 0x00000001;
         static constexpr UINT32 _FLAG_ENFORCED = 0x00000002;
         static constexpr UINT32 _FLAG_NOT_NULL = 0x00000004;
         static constexpr UINT32 _FLAG_NOT_ARRAY = 0x00000008;
         static constexpr UINT32 _FLAG_BTREE_COMPRESSION = 0x00000010;

      private:
         ossPoolString _name;
         indexKeyPattern _pattern;
         INDEX_TYPE _type = INVALID_INDEX_TYPE;
         utilIdxInnerID _innerID = UTIL_UNIQUEID_NULL;
         UINT32 _flags = 0;

         /******* btree only begin   *******/
         /// Max column count of prefix in btree prefix compression.
         /// It should be one unless there are a lot of duplicate index keys.
         UINT32 _btreeMaxPrefixFields = 0;

         /// Min depth of btree to enable prefix compression.
         UINT32 _btreeMinCompressionDepth = 2;
         /******* btree only end   *******/

         /******* lsm only begin   *******/
         UINT32 _columnFamily = 0;
         /******* lsm only end   *******/

   };//class indexDescription
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_DESCRIPTION_H_