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

   Source File Name = clsIndexInfo.cpp

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

#include "clsIndexInfo.hpp"
#include "msgDef.h"
#include "ossTypes.h"
#include "ossUtil.h"
#include "utilStringView.hpp"
#include "utilUniqueID.hpp"
#include "ixm.hpp"
#include <memory>

namespace engine
{
utilIdxInnerID _clsIndexInfo::getIdxInnerID() const
{
   return _idxInnerID;
}

const CHAR *_clsIndexInfo::getIndexName() const
{
   return _name.c_str();
}

const OID &_clsIndexInfo::getOID() const
{
   return _oid;
}

const BSONObj &_clsIndexInfo::getKeyPattern() const
{
   return _keyPattern;
}

BOOLEAN _clsIndexInfo::isUnique() const
{
   return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::UNIQUE ) );
}

BOOLEAN _clsIndexInfo::isEnforced() const
{
   return OSS_BIT_TEST( _flags,
                        static_cast< UINT32 >( FLAGS_FIELD::ENFORCED ) );
}

BOOLEAN _clsIndexInfo::isNotNull() const
{
   return OSS_BIT_TEST( _flags,
                        static_cast< UINT32 >( FLAGS_FIELD::NOT_NULL ) );
}

BOOLEAN _clsIndexInfo::isNotArray() const
{
   return OSS_BIT_TEST( _flags,
                        static_cast< UINT32 >( FLAGS_FIELD::NOT_ARRAY ) );
}

BOOLEAN _clsIndexInfo::isDropDups() const
{
   return OSS_BIT_TEST( _flags,
                        static_cast< UINT32 >( FLAGS_FIELD::DROP_DUPS ) );
}

BOOLEAN _clsIndexInfo::isIDIndex() const
{
   return OSS_BIT_TEST(
      _flags,
      static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
}

clsIndexInfo _clsIndexInfo::buildIndexInfoFromBson( const BSONObj &obj )
{
   clsIndexInfo info;

   info._name = obj.getStringField( IXM_NAME_FIELD );
   info._idxInnerID = obj.getIntField( IXM_INNERID_FIELD );
   obj.getField( DMS_ID_KEY_NAME ).Val( info._oid ) ;
   info._keyPattern = obj.getObjectField( IXM_KEY_FIELD );
   if ( obj.getBoolField( IXM_UNIQUE_FIELD ) )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
   }
   if ( obj.getBoolField( IXM_ENFORCED_FIELD ) )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::ENFORCED ) );
   }
   if ( obj.getBoolField( IXM_NOTNULL_FIELD ) )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_NULL ) );
   }
   if ( obj.getBoolField( IXM_NOTARRAY_FIELD ) )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_ARRAY ) );
   }
   if ( obj.getBoolField( IXM_FIELD_NAME_DROPDUPS ) )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::DROP_DUPS ) );
   }
   if ( info._name == IXM_ID_KEY_NAME )
   {
      OSS_BIT_SET(
         info._flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
   }
   return info;
}

_clsIndexInfo::_clsIndexInfo( const CHAR *indexName,
                              utilIdxInnerID idxInnerID,
                              const OID &oid,
                              BSONObj keyPattern,
                              BOOLEAN isUnique,
                              BOOLEAN isEnforced,
                              BOOLEAN isNotNull,
                              BOOLEAN isNotArray,
                              BOOLEAN isDropDups )
: _name( indexName ), _idxInnerID( idxInnerID ), _oid(oid) ,_keyPattern( keyPattern )
{
   if ( isUnique )
   {
      OSS_BIT_SET(
         _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
   }
   if ( isEnforced )
   {
      OSS_BIT_SET(
         _flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::ENFORCED ) );
   }
   if ( isNotNull )
   {
      OSS_BIT_SET(
         _flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_NULL ) );
   }
   if ( isNotArray )
   {
      OSS_BIT_SET(
         _flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_ARRAY ) );
   }
   if ( isDropDups )
   {
      OSS_BIT_SET(
         _flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::DROP_DUPS ) );
   }
   if ( _name == IXM_ID_KEY_NAME )
   {
      OSS_BIT_SET(
         _flags,
         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
   }
}

clsIndexInfoSetPtr _clsIndexInfoSet::buildIndexSetFromBsonVec(
   const ossPoolVector< BSONObj > &v )
{
   clsIndexInfoSetPtr indexSetPtr = make_shared< clsIndexInfoSet >();
   for ( ossPoolVector< BSONObj >::const_iterator it = v.begin();
         it != v.cend();
         ++it )
   {
      indexSetPtr->_vecInfo.push_back(
         _clsIndexInfo::buildIndexInfoFromBson( *it ) );
   }
   return indexSetPtr;
}

const clsIndexInfo *_clsIndexInfoSet::get( const CHAR *indexName ) const
{
   auto isNameEqual = [ &, indexName ]( const clsIndexInfo &info ) -> BOOLEAN {
      return utilStringView( info.getIndexName() ) ==
             utilStringView( indexName );
   };
   const ossPoolVector< clsIndexInfo >::const_iterator it =
      std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isNameEqual );
   if ( it == _vecInfo.cend() )
   {
      return nullptr;
   }
   else
   {
      return it.base();
   }
}

const clsIndexInfo *_clsIndexInfoSet::get( utilIdxInnerID idxInnerID ) const
{
   auto isInnerIDEqual = [ &,
                           idxInnerID ]( const clsIndexInfo &info ) -> BOOLEAN {
      return info.getIdxInnerID() == idxInnerID;
   };
   const ossPoolVector< clsIndexInfo >::const_iterator it =
      std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isInnerIDEqual );
   if ( it == _vecInfo.cend() )
   {
      return nullptr;
   }
   else
   {
      return it.base();
   }
}

const clsIndexInfo *_clsIndexInfoSet::get( const OID &oid )
{
   auto isOIDEqual = [ &, oid ]( const clsIndexInfo &info ) -> BOOLEAN {
      return info.getOID() == oid;
   };
   const ossPoolVector< clsIndexInfo >::const_iterator it =
      std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isOIDEqual );
   if ( it == _vecInfo.cend() )
   {
      return nullptr;
   }
   else
   {
      return it.base();
   }
}

const ossPoolVector< clsIndexInfo > &_clsIndexInfoSet::getAll() const
{
   return _vecInfo;
}

} // namespace engine