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
#include "utilStringView.hpp"
#include <exception>
#include <memory>

namespace engine
{
   INT32 _clsIndexInfo::init( const CHAR *indexName,
                              utilIdxInnerID idxInnerID,
                              const OID &oid,
                              const BSONObj &keyPattern,
                              BOOLEAN isUnique,
                              BOOLEAN isEnforced,
                              BOOLEAN isNotNull,
                              BOOLEAN isNotArray,
                              BOOLEAN isDropDups,
                              const CLS_INDEX_STAT_PTR &statPtr )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( indexName, SDB_INVALIDARG, error, PDERROR, "can not be nullptr" );
      try
      {
         _name = ossPoolString( indexName );
         _idxInnerID = idxInnerID;
         _oid = oid;
         _keyPattern = keyPattern;
         _keyPattern.getOwned();
         _statPtr = statPtr;
         if ( isUnique )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
         }
         if ( isEnforced )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::ENFORCED ) );
         }
         if ( isNotNull )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_NULL ) );
         }
         if ( isNotArray )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_ARRAY ) );
         }
         if ( isDropDups )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::DROP_DUPS ) );
         }
         if ( _name == IXM_ID_KEY_NAME )
         {
            OSS_BIT_SET( _flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: %s", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   void _clsIndexInfo::reset()
   {
      _name.clear();
      _idxInnerID = UTIL_UNIQUEID_NULL;
      _oid.clear();
      _keyPattern = BSONObj();
      _flags = 0;
      _statPtr = CLS_DEFAULT_INDEX_STAT;
   }

   INT32 _clsIndexInfo::setKeyPattern( const BSONObj &pattern )
   {
      {
         INT32 rc = SDB_OK;
         try
         {
            _keyPattern = pattern;
            _keyPattern.getOwned();
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e );
            PD_LOG( PDERROR, "occur exception: %s", e.what() );
            goto error;
         }
      done:
         return rc;
      error:
         _keyPattern = BSONObj();
         goto done;
      }
   }

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

   const CONST_CLS_INDEX_STAT_PTR &_clsIndexInfo::getStat() const
   {
      return _statPtr;
   }

   BOOLEAN _clsIndexInfo::isUnique() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::UNIQUE ) );
   }

   BOOLEAN _clsIndexInfo::isEnforced() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::ENFORCED ) );
   }

   BOOLEAN _clsIndexInfo::isNotNull() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::NOT_NULL ) );
   }

   BOOLEAN _clsIndexInfo::isNotArray() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::NOT_ARRAY ) );
   }

   BOOLEAN _clsIndexInfo::isDropDups() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::DROP_DUPS ) );
   }

   BOOLEAN _clsIndexInfo::isIDIndex() const
   {
      return OSS_BIT_TEST( _flags,
                           static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
   }

   INT32 _clsIndexInfo::buildIndexInfoFromBson( const BSONObj &obj, CLS_INDEX_INFO_PTR &infoPtr )
   {
      INT32 rc = SDB_OK;
      CLS_INDEX_INFO_PTR tempPtr = makeSharedPtrFromPool< clsIndexInfo >();
      if ( !tempPtr )
      {
         PD_LOG( PDERROR, "out of memory" );
         rc = SDB_OOM;
         goto error;
      }
      try
      {
         clsIndexInfo &info = *tempPtr;
         info._name = obj.getField( IXM_NAME_FIELD ).poolString();
         info._idxInnerID = obj.getField( IXM_INNERID_FIELD ).Int();
         obj.getField( DMS_ID_KEY_NAME ).Val( info._oid );
         info._keyPattern = obj.getObjectField( IXM_KEY_FIELD );
         info._keyPattern.getOwned();
         if ( obj.getBoolField( IXM_UNIQUE_FIELD ) )
         {
            OSS_BIT_SET( info._flags, static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::UNIQUE ) );
         }
         if ( obj.getBoolField( IXM_ENFORCED_FIELD ) )
         {
            OSS_BIT_SET( info._flags,
                         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::ENFORCED ) );
         }
         if ( obj.getBoolField( IXM_NOTNULL_FIELD ) )
         {
            OSS_BIT_SET( info._flags,
                         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_NULL ) );
         }
         if ( obj.getBoolField( IXM_NOTARRAY_FIELD ) )
         {
            OSS_BIT_SET( info._flags,
                         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::NOT_ARRAY ) );
         }
         if ( obj.getBoolField( IXM_FIELD_NAME_DROPDUPS ) )
         {
            OSS_BIT_SET( info._flags,
                         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::DROP_DUPS ) );
         }
         if ( info._name == IXM_ID_KEY_NAME )
         {
            OSS_BIT_SET( info._flags,
                         static_cast< UINT32 >( _clsIndexInfo::FLAGS_FIELD::IS_ID_INDEX ) );
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "occur exception: %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
      infoPtr = std::move( tempPtr );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsIndexInfoSet::buildIndexSetFromBsonVec( const ossPoolVector< BSONObj > &v,
                                                     CLS_INDEX_INFO_SET_PTR &infoSetPtr )
   {
      INT32 rc = SDB_OK;
      CLS_INDEX_INFO_SET_PTR tempPtr = makeSharedPtrFromPool< clsIndexInfoSet >();
      if ( !tempPtr )
      {
         PD_LOG( PDERROR, "out of memory" );
         rc = SDB_OOM;
         goto error;
      }
      for ( ossPoolVector< BSONObj >::const_iterator it = v.begin(); it != v.cend(); ++it )
      {
         try
         {
            CLS_INDEX_INFO_PTR infoPtr;
            rc = _clsIndexInfo::buildIndexInfoFromBson( *it, infoPtr );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build index info from bson" );
               goto error;
            }
            tempPtr->_vecInfo.push_back( std::move( infoPtr ) );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "occur exception: %s", e.what() );
            rc = ossException2RC( &e );
            goto error;
         }
      }
      infoSetPtr = std::move( tempPtr );
   done:
      return rc;
   error:
      goto done;
   }

   CLS_INDEX_INFO_PTR _clsIndexInfoSet::get( const CHAR *indexName ) const
   {
      auto isNameEqual = [ &, indexName ]( const CLS_INDEX_INFO_PTR &info ) -> BOOLEAN {
         return utilStringView( info->getIndexName() ) == utilStringView( indexName );
      };
      const ossPoolVector< CLS_INDEX_INFO_PTR >::const_iterator it =
         std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isNameEqual );
      if ( it == _vecInfo.cend() )
      {
         return nullptr;
      }
      else
      {
         return *it;
      }
   }

   CLS_INDEX_INFO_PTR _clsIndexInfoSet::get( utilIdxInnerID idxInnerID ) const
   {
      auto isInnerIDEqual = [ &, idxInnerID ]( const CLS_INDEX_INFO_PTR &info ) -> BOOLEAN {
         return info->getIdxInnerID() == idxInnerID;
      };
      const ossPoolVector< CLS_INDEX_INFO_PTR >::const_iterator it =
         std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isInnerIDEqual );
      if ( it == _vecInfo.cend() )
      {
         return nullptr;
      }
      else
      {
         return *it;
      }
   }

   CLS_INDEX_INFO_PTR _clsIndexInfoSet::get( const OID &oid ) const
   {
      auto isOIDEqual = [ &, oid ]( const CLS_INDEX_INFO_PTR &info ) -> BOOLEAN {
         return info->getOID() == oid;
      };
      const ossPoolVector< CLS_INDEX_INFO_PTR >::const_iterator it =
         std::find_if( _vecInfo.cbegin(), _vecInfo.cend(), isOIDEqual );
      if ( it == _vecInfo.cend() )
      {
         return nullptr;
      }
      else
      {
         return *it;
      }
   }

   const ossPoolVector< CLS_INDEX_INFO_PTR > &_clsIndexInfoSet::getVec() const
   {
      return _vecInfo;
   }

   ossPoolVector< CLS_INDEX_INFO_PTR > &_clsIndexInfoSet::getVec()
   {
      return _vecInfo;
   }

} // namespace engine