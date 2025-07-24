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

   Source File Name = dmsObjectMetaInfo.hpp

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
#ifndef DMS_INDEX_META_INFO_HPP__
#define DMS_INDEX_META_INFO_HPP__

#include "ixm.hpp"
#include "oss.hpp"
#include "ossMemPool.hpp"
#include "pmdEDU.hpp"
#include "utilUniqueID.hpp"
#include "interface/IObjectInfo.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace engine
{
   class _dmsIndexMetaInfo : public IIndexMetaInfo
   {
   public:
      static INT32 buildFromBson( const BSONObj &obj,
                                  std::shared_ptr< _dmsIndexMetaInfo > &infoPtr,
                                  const std::shared_ptr< ossPoolString > &clFullName = nullptr );

   public:
      _dmsIndexMetaInfo() = default;
      _dmsIndexMetaInfo( const std::shared_ptr< const ossPoolString > &clFullName,
                         const std::shared_ptr< const ossPoolString > &indexName,
                         utilCLUniqueID clUID,
                         utilIdxInnerID idxInnerID,
                         const OID &oid,
                         const BSONObj &keyPattern,
                         UINT64 stpEffectiveTime,
                         INT32 indexExtentID,
                         INT64 indexLogicalID,
                         UINT16 indexType,
                         INT32 indexStatus,
                         BOOLEAN isUnique,
                         BOOLEAN isEnforced,
                         BOOLEAN isNotNull,
                         BOOLEAN isNotArray,
                         BOOLEAN isDropDups ) noexcept
      : _clFullName( clFullName )
      , _indexName( indexName )
      , _oid( oid )
      , _keyPattern( keyPattern.getOwned() )
      , _stpEffectiveTime( stpEffectiveTime )
      , _indexExtentID( indexExtentID )
      , _indexLogicalID( indexLogicalID )
      , _indexType( indexType )
      , _indexStatus( indexStatus )
      {
         setUnique( isUnique );
         setEnforced( isEnforced );
         setNotNull( isNotNull );
         setNotArray( isNotArray );
         setDropDups( isDropDups );
         setIDIndex( *clFullName == IXM_ID_KEY_NAME );
      }

      virtual ~_dmsIndexMetaInfo() = default;

   public:
      virtual const CHAR *getCLFullName() const override;
      virtual const CHAR *getIndexName() const override;
      virtual utilIdxInnerID getIdxInnerID() const override;
      virtual utilCLUniqueID getCLUniqueID() const override;
      virtual const OID &getOID() const override;
      virtual const BSONObj &getKeyPattern() const override;
      virtual UINT32 getNumKeys() const override;
      virtual UINT64 getStpEffectiveTime() const override;
      virtual INT32 getExtentID() const override;
      virtual INT64 getLogicalID() const override;
      virtual UINT16 getIndexType() const override;
      virtual INT32 getIndexStatus() const override;
      virtual BOOLEAN isNormal() const override;
      virtual BOOLEAN isUnique() const override;
      virtual BOOLEAN isEnforced() const override;
      virtual BOOLEAN isNotNull() const override;
      virtual BOOLEAN isNotArray() const override;
      virtual BOOLEAN isDropDups() const override;
      virtual BOOLEAN isIDIndex() const override;

   public:
      INT32 setKeyPattern( const BSONObj &pattern );
      void setUnique( BOOLEAN isUnique );
      void setEnforced( BOOLEAN isEnforced );
      void setNotNull( BOOLEAN isNotNull );
      void setNotArray( BOOLEAN isNotArray );
      void setDropDups( BOOLEAN isDropDups );
      void setIDIndex( BOOLEAN isIDIndex );

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
      std::shared_ptr< const ossPoolString > _clFullName;
      std::shared_ptr< const ossPoolString > _indexName;
      utilCLUniqueID _clUID = UTIL_UNIQUEID_NULL;
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL;
      OID _oid;
      BSONObj _keyPattern;
      // stp time when the index takes effect after rebuild
      UINT64 _stpEffectiveTime = 0;
      // for v3/v5, quickAddressID is extent id
      INT32 _indexExtentID = DMS_INVALID_EXTENT;
      // for v3/v5, objSequenceNumber is dms logical id
      INT64 _indexLogicalID = DMS_INVALID_EXTENT;
      // the definition refer to ixmIndexCB
      UINT16 _indexType = 0;
      INT32 _indexStatus = 0;
      UINT32 _flags = 0;
   };
   using dmsIndexMetaInfo = _dmsIndexMetaInfo;
   using DMS_INDEX_META_PTR = std::shared_ptr< dmsIndexMetaInfo >;
   using CONST_DMS_INDEX_META_PTR = std::shared_ptr< const dmsIndexMetaInfo >;

   class _dmsCollectionMetaInfo : public ICollectionMetaInfo
   {
   public:
      _dmsCollectionMetaInfo( const std::shared_ptr< ossPoolString > &name,
                              utilCLUniqueID clUID,
                              UINT32 attributes,
                              UINT32 pageSizeLog2,
                              UINT32 totalDataPages,
                              UINT64 globTransAvailTime ) noexcept
      : _name( name )
      , _clUID( clUID )
      , _attributes( attributes )
      , _pageSizeLog2( pageSizeLog2 )
      , _totalDataPages( totalDataPages )
      , _globTransAvailTime( globTransAvailTime )
      {
      }

      _dmsCollectionMetaInfo(const _dmsCollectionMetaInfo &o) = default;
      _dmsCollectionMetaInfo( _dmsCollectionMetaInfo &&o ) = default;

   public:
      virtual const CHAR *getCLFullName() const override
      {
         return _name->c_str();
      }

      virtual utilCLUniqueID getCLUniqueID() const override
      {
         return _clUID;
      }

      virtual UINT32 getAttributes() const override
      {
         return _attributes;
      }

      virtual UINT32 getPageSizeLog2() const override
      {
         return _pageSizeLog2;
      }

      virtual UINT32 getTotalDataPages() const override
      {
         return _totalDataPages;
      }

      virtual UINT64 getGlobTransAvailTime() const override
      {
         return _globTransAvailTime;
      }

      virtual UINT32 getIndexNum() const override
      {
         return _vecIndexMeta.size();
      }

      virtual std::shared_ptr< const IIndexMetaInfo > at( UINT32 position ) const override;

      virtual std::shared_ptr< const IIndexMetaInfo > seek( const CHAR *indexName ) const override;

      virtual std::shared_ptr< const IIndexMetaInfo > seek( const OID &indexOID ) const override;

   public:
      INT32 pushIndexMetaInfo( const CONST_DMS_INDEX_META_PTR &indexMeta );

   private:
      std::shared_ptr< const ossPoolString > _name = nullptr;
      utilCLUniqueID _clUID = UTIL_UNIQUEID_NULL;
      UINT32 _attributes = 0;
      UINT32 _pageSizeLog2 = 0;
      UINT32 _totalDataPages = 0;
      UINT64 _globTransAvailTime = DPS_MAX_TRANS_TIME;

      ossPoolVector< CONST_DMS_INDEX_META_PTR > _vecIndexMeta;
   };
   using dmsCollectionMetaInfo = _dmsCollectionMetaInfo;
   using DMS_CL_META_PTR = shared_ptr< dmsCollectionMetaInfo >;
   using CONST_DMS_CL_META_PTR = shared_ptr< const dmsCollectionMetaInfo >;
} // namespace engine
#endif