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
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
class _clsIndexInfo : public SDBObject
{
public:
   static _clsIndexInfo buildIndexInfoFromBson( const BSONObj & );
   _clsIndexInfo() = default;
   _clsIndexInfo( const CHAR *indexName,
                  utilIdxInnerID idxInnerID,
                  const OID &oid,
                  BSONObj keyPattern,
                  BOOLEAN isUnique,
                  BOOLEAN isEnforced,
                  BOOLEAN isNotNull,
                  BOOLEAN isNotArray,
                  BOOLEAN isDropDups );

public:
   utilIdxInnerID getIdxInnerID() const;
   const CHAR *getIndexName() const;
   const OID &getOID() const;
   const BSONObj &getKeyPattern() const;
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
      ENFORCED = (1 << 1),
      NOT_NULL = (1 << 2),
      NOT_ARRAY = (1 << 3),
      DROP_DUPS = (1 << 4),
      IS_ID_INDEX = (1 << 5)
   };

private:
   std::string _name;
   utilIdxInnerID _idxInnerID = 0;
   OID _oid;
   BSONObj _keyPattern;
   UINT32 _flags = 0;
};

using clsIndexInfo = _clsIndexInfo;

class _clsIndexInfoSet : public SDBObject
{
public:
   static std::shared_ptr< _clsIndexInfoSet > buildIndexSetFromBsonVec(
       const ossPoolVector< BSONObj > & );

public:
   const clsIndexInfo *get( const CHAR *indexName ) const;
   const clsIndexInfo *get( utilIdxInnerID idxInnerID ) const;
   const clsIndexInfo *get( const OID &oid);
   const ossPoolVector< clsIndexInfo > &getAll() const;

private:
   ossPoolVector< clsIndexInfo > _vecInfo;
};
using clsIndexInfoSet = _clsIndexInfoSet;
using clsIndexInfoSetPtr = std::shared_ptr< clsIndexInfoSet >;

} // namespace engine

#endif