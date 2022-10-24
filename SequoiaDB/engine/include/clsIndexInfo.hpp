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

   Source File Name = clsIndexInfo.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/20/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_INDEX_INFO_HPP__
#define CLS_INDEX_INFO_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "pmdEDU.hpp"
#include "utilUniqueID.hpp"
#include "clsResourceStatDef.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
   class _clsIndexInfo : public SDBObject
   {
   public:
      static INT32 buildIndexInfoFromBson( const BSONObj &obj,
                                           std::shared_ptr< _clsIndexInfo > &infoPtr );
      _clsIndexInfo() = default;

   public:
      INT32 init( const CHAR *indexName,
                  utilIdxInnerID idxInnerID,
                  const OID &oid,
                  const BSONObj &keyPattern,
                  BOOLEAN isUnique,
                  BOOLEAN isEnforced,
                  BOOLEAN isNotNull,
                  BOOLEAN isNotArray,
                  BOOLEAN isDropDups,
                  const CLS_INDEX_STAT_PTR &statPtr );
      void reset();

      OSS_INLINE void resetStat()
      {
         _statPtr = CLS_DEFAULT_INDEX_STAT;
      }

      OSS_INLINE void setStat( const CLS_INDEX_STAT_PTR &statPtr )
      {
         _statPtr = statPtr;
      }

      OSS_INLINE void setUnique( BOOLEAN isUnique )
      {
         if ( isUnique )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
         }
         else
         {
            OSS_BIT_CLEAR( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
         }
      }

      INT32 setKeyPattern( const BSONObj &pattern );

   public:
      utilIdxInnerID getIdxInnerID() const;
      const CHAR *getIndexName() const;
      const OID &getOID() const;
      const BSONObj &getKeyPattern() const;
      const CONST_CLS_INDEX_STAT_PTR &getStat() const;
      BOOLEAN isUnique() const;
      BOOLEAN isEnforced() const;
      BOOLEAN isNotNull() const;
      BOOLEAN isNotArray() const;
      BOOLEAN isDropDups() const;
      BOOLEAN isIDIndex() const;

   private:
      // From low bit to high bit, respectively indicate:
      // unique, enforced, notNull, notArray, dropDups, isIDIndex, compression
      enum class FLAGS_FIELD
      {
         UNIQUE = 1,
         ENFORCED = ( 1 << 1 ),
         NOT_NULL = ( 1 << 2 ),
         NOT_ARRAY = ( 1 << 3 ),
         DROP_DUPS = ( 1 << 4 ),
         IS_ID_INDEX = ( 1 << 5 )
      };

   private:
      ossPoolString _name;
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL;
      OID _oid;
      BSONObj _keyPattern;
      UINT32 _flags = 0;
      CONST_CLS_INDEX_STAT_PTR _statPtr = CLS_DEFAULT_INDEX_STAT;
   };

   using clsIndexInfo = _clsIndexInfo;
   using CLS_INDEX_INFO_PTR = std::shared_ptr< clsIndexInfo >;
   using CONST_CLS_INDEX_INFO_PTR = std::shared_ptr< const clsIndexInfo >;

   class _clsIndexInfoSet : public SDBObject
   {
   public:
      static INT32 buildIndexSetFromBsonVec( const ossPoolVector< BSONObj > &,
                                             std::shared_ptr< _clsIndexInfoSet > &infoSetPtr );

      _clsIndexInfoSet() = default;
      _clsIndexInfoSet( const _clsIndexInfoSet &o ) : _vecInfo( o._vecInfo ) {}
      _clsIndexInfoSet( _clsIndexInfoSet &&o ) : _vecInfo( std::move( o._vecInfo ) ) {}

   public:
      CLS_INDEX_INFO_PTR get( const CHAR *indexName ) const;
      CLS_INDEX_INFO_PTR get( utilIdxInnerID idxInnerID ) const;
      CLS_INDEX_INFO_PTR get( const OID &oid ) const;
      const ossPoolVector< CLS_INDEX_INFO_PTR > &getVec() const;
      ossPoolVector< CLS_INDEX_INFO_PTR > &getVec();

   protected:
      ossPoolVector< CLS_INDEX_INFO_PTR > _vecInfo;
   };
   using clsIndexInfoSet = _clsIndexInfoSet;
   using CLS_INDEX_INFO_SET_PTR = std::shared_ptr< clsIndexInfoSet >;
   using CONST_CLS_INDEX_INFO_SET_PTR = std::shared_ptr< const clsIndexInfoSet >;
} // namespace engine
#endif