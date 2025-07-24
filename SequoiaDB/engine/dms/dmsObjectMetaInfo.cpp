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

   Source File Name = dmsObjectMetaInfo.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/20/2022  ZHY Initial Draft
          11/24/2022  ZHY Move to dms module
   Last Changed =

*******************************************************************************/
#include "dmsObjectMetaInfo.hpp"
#include "ixm.hpp"
#include "utilSharedPtrMaker.hpp"
#include "utilStringView.hpp"
#include <exception>
#include <memory>

namespace engine
{
   INT32 _dmsIndexMetaInfo::setKeyPattern( const BSONObj &pattern )
   {
      {
         INT32 rc = SDB_OK;
         try
         {
            _keyPattern = pattern.getOwned();
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e );
            PD_LOG( PDERROR, "occur exception: %s, rc: %d", e.what(), rc );
            goto error;
         }
      done:
         return rc;
      error:
         _keyPattern = BSONObj();
         goto done;
      }
   }

   const CHAR *_dmsIndexMetaInfo::getCLFullName() const
   {
      return _clFullName->c_str();
   }

   const CHAR *_dmsIndexMetaInfo::getIndexName() const
   {
      return _indexName->c_str();
   }

   utilIdxInnerID _dmsIndexMetaInfo::getIdxInnerID() const
   {
      return _idxInnerID;
   }

   utilCLUniqueID _dmsIndexMetaInfo::getCLUniqueID() const
   {
      return _clUID;
   }

   const OID &_dmsIndexMetaInfo::getOID() const
   {
      return _oid;
   }

   const BSONObj &_dmsIndexMetaInfo::getKeyPattern() const
   {
      return _keyPattern;
   }

   UINT32 _dmsIndexMetaInfo::getNumKeys() const
   {
      return _keyPattern.nFields();
   }

   UINT64 _dmsIndexMetaInfo::getStpEffectiveTime() const
   {
      return _stpEffectiveTime;
   }

   INT32 _dmsIndexMetaInfo::getExtentID() const
   {
      return _indexExtentID;
   }

   INT64 _dmsIndexMetaInfo::getLogicalID() const
   {
      return _indexLogicalID;
   }

   UINT16 _dmsIndexMetaInfo::getIndexType() const
   {
      return _indexType;
   }

   INT32 _dmsIndexMetaInfo::getIndexStatus() const
   {
      return _indexStatus;
   }

   BOOLEAN _dmsIndexMetaInfo::isNormal() const
   {
      return _indexStatus == IXM_INDEX_FLAG_NORMAL;
   }

   BOOLEAN _dmsIndexMetaInfo::isUnique() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::UNIQUE ) );
   }

   BOOLEAN _dmsIndexMetaInfo::isEnforced() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::ENFORCED ) );
   }

   BOOLEAN _dmsIndexMetaInfo::isNotNull() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::NOT_NULL ) );
   }

   BOOLEAN _dmsIndexMetaInfo::isNotArray() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::NOT_ARRAY ) );
   }

   BOOLEAN _dmsIndexMetaInfo::isDropDups() const
   {
      return OSS_BIT_TEST( _flags, static_cast< UINT32 >( FLAGS_FIELD::DROP_DUPS ) );
   }

   BOOLEAN _dmsIndexMetaInfo::isIDIndex() const
   {
      return OSS_BIT_TEST( _flags,
                           static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::IS_ID_INDEX ) );
   }

   INT32 _dmsIndexMetaInfo::buildFromBson( const BSONObj &obj,
                                           DMS_INDEX_META_PTR &infoPtr,
                                           const std::shared_ptr< ossPoolString > &clFullName )
   {
      INT32 rc = SDB_OK;
      DMS_INDEX_META_PTR tempPtr = makeSharedPtrFromPool< dmsIndexMetaInfo >();
      if ( !tempPtr )
      {
         PD_LOG( PDERROR, "out of memory" );
         rc = SDB_OOM;
         goto error;
      }
      try
      {
         dmsIndexMetaInfo &info = *tempPtr;
         std::shared_ptr< ossPoolString > parsedClFullName =
            makeSharedPtrFromPool< ossPoolString >( obj.getField( FIELD_NAME_COLLECTION ) );
         std::shared_ptr< ossPoolString > indexName =
            makeSharedPtrFromPool< ossPoolString >( obj.getField( IXM_NAME_FIELD ).poolStr() );
         if ( clFullName && *clFullName == *parsedClFullName )
         {
            info._clFullName = clFullName;
         }
         else
         {
            info._clFullName = parsedClFullName;
         }
         info._indexName = indexName;
         info._clUID = obj.getField( FIELD_NAME_CL_UNIQUEID ).numberLong();
         info._idxInnerID = obj.getField( IXM_FIELD_NAME_INNERID ).numberLong();
         obj.getField( DMS_ID_KEY_NAME ).Val( info._oid );
         info._stpEffectiveTime = obj.getField( IXM_FIELD_NAME_REBUILDTIME ).numberLong();
         info._indexExtentID = obj.getField( IXM_FIELD_NAME_CB_EXTENT_ID ).numberInt();
         info._indexLogicalID = obj.getField( FIELD_NAME_LOGICAL_ID ).numberLong();
         info._indexType = obj.getField( IXM_FIELD_NAME_TYPE ).numberInt();
         info._indexStatus = obj.getField( IXM_FIELD_NAME_INDEX_FLAG ).numberInt();
         info._keyPattern = obj.getObjectField( IXM_KEY_FIELD ).getOwned();
         info.setUnique( obj.getBoolField( IXM_UNIQUE_FIELD ) );
         info.setEnforced( obj.getBoolField( IXM_ENFORCED_FIELD ) );
         info.setNotNull( obj.getBoolField( IXM_NOTNULL_FIELD ) );
         info.setNotArray( obj.getBoolField( IXM_NOTARRAY_FIELD ) );
         info.setDropDups( obj.getBoolField( IXM_FIELD_NAME_DROPDUPS ) );
         info.setIDIndex( *info._indexName == IXM_ID_KEY_NAME );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDWARNING, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
      infoPtr = std::move( tempPtr );
   done:
      return rc;
   error:
      goto done;
   }

   void _dmsIndexMetaInfo::setUnique( BOOLEAN isUnique )
   {
      if ( isUnique )
      {
         OSS_BIT_SET( _flags, static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::UNIQUE ) );
      }
   }
   void _dmsIndexMetaInfo::setEnforced( BOOLEAN isEnforced )
   {
      if ( isEnforced )
      {
         OSS_BIT_SET( _flags, static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::ENFORCED ) );
      }
   }
   void _dmsIndexMetaInfo::setNotNull( BOOLEAN isNotNull )
   {
      if ( isNotNull )
      {
         OSS_BIT_SET( _flags, static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::NOT_NULL ) );
      }
   }
   void _dmsIndexMetaInfo::setNotArray( BOOLEAN isNotArray )
   {
      if ( isNotArray )
      {
         OSS_BIT_SET( _flags, static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::NOT_ARRAY ) );
      }
   }
   void _dmsIndexMetaInfo::setDropDups( BOOLEAN isDropDups )
   {
      if ( isDropDups )
      {
         OSS_BIT_SET( _flags, static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::DROP_DUPS ) );
      }
   }
   void _dmsIndexMetaInfo::setIDIndex( BOOLEAN isIDIndex )
   {
      if ( isIDIndex )
      {
         OSS_BIT_SET( _flags,
                      static_cast< UINT32 >( _dmsIndexMetaInfo::FLAGS_FIELD::IS_ID_INDEX ) );
      }
   }

   INT32 _dmsCollectionMetaInfo::pushIndexMetaInfo( const CONST_DMS_INDEX_META_PTR &indexMeta )
   {
      INT32 rc = SDB_OK;
      try
      {
         _vecIndexMeta.push_back( indexMeta );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDWARNING, "occur exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   std::shared_ptr< const IIndexMetaInfo > _dmsCollectionMetaInfo::at( UINT32 position ) const
   {
      SDB_ASSERT( position < getIndexNum(), "out of bound" );
      return _vecIndexMeta[ position ];
   }
   std::shared_ptr< const IIndexMetaInfo > _dmsCollectionMetaInfo::seek(
      const CHAR *indexName ) const
   {
      decltype( _vecIndexMeta )::const_iterator found = std::find_if(
         _vecIndexMeta.begin(), _vecIndexMeta.end(),
         [ & ]( const decltype( _vecIndexMeta )::value_type &indexMeta ) {
            return utilStringView( indexMeta->getIndexName() ) == utilStringView( indexName );
         } );
      if ( found != _vecIndexMeta.end() )
      {
         return *found;
      }
      else
      {
         return nullptr;
      }
   }

   std::shared_ptr< const IIndexMetaInfo > _dmsCollectionMetaInfo::seek( const OID &indexOID ) const
   {
      decltype( _vecIndexMeta )::const_iterator found =
         std::find_if( _vecIndexMeta.begin(), _vecIndexMeta.end(),
                       [ & ]( const decltype( _vecIndexMeta )::value_type &indexMeta ) {
                          return indexMeta->getOID() == indexOID;
                       } );
      if ( found != _vecIndexMeta.end() )
      {
         return *found;
      }
      else
      {
         return nullptr;
      }
   }
} // namespace engine