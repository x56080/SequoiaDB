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

   Source File Name = rtnObjectStatInfo.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/12/2022  ZHY Initial Draft
          11/24/2022  ZHY Mode to runtime module
   Last Changed =

*******************************************************************************/
#ifndef CLS_RESOURCE_STAT_DEF_HPP__
#define CLS_RESOURCE_STAT_DEF_HPP__

#include "oss.hpp"
#include "ossTypes.h"
#include "utilSharedPtrMaker.hpp"
#include "utilUniqueID.hpp"
#include "interface/IObjectInfo.h"
#include <memory>

namespace engine
{

   class _rtnStatBase
   {
   public:
      _rtnStatBase() = default;
      _rtnStatBase( UINT64 createTime, UINT64 sampleRecords, UINT64 totalRecords )
      : _createTime( createTime ), _sampleRecords( sampleRecords ), _totalRecords( totalRecords )
      {
      }

   public:
      OSS_INLINE void setCreateTime( UINT64 createTime )
      {
         _createTime = createTime;
      }
      OSS_INLINE void setSampleRecords( UINT64 sampleRecords )
      {
         _sampleRecords = sampleRecords;
      }
      OSS_INLINE void setTotalRecords( UINT64 totalRecords )
      {
         _totalRecords = totalRecords;
      }

   protected:
      // CreateTime (timestamp) of the cache unit
      UINT64 _createTime = 0;
      // Number of records in the sample
      UINT64 _sampleRecords = RTN_STAT_DEF_TOTAL_RECORDS;
      // Number of records in the collection when collecting this statistics
      UINT64 _totalRecords = RTN_STAT_DEF_TOTAL_RECORDS;
   };
   using rtnStatBase = _rtnStatBase;

   class _rtnIndexStatInfo : public _rtnStatBase, public IIndexStatInfo
   {
   public:
      // to save space for collection full name, we could pass the a shared pointer constructed
      // elsewhere, such as rtnCollectionStatInfo. Regardless of whether it is passed or not, the
      // name in BSON will be parsed. If the same, use the pointer passed in.
      static INT32 buildFromBson(
         const BSONObj &obj,
         std::shared_ptr< _rtnIndexStatInfo > &indexStatPtr,
         const std::shared_ptr< const ossPoolString > &clFullName = nullptr );

   public:
      _rtnIndexStatInfo( const std::shared_ptr< const ossPoolString > &clFullName,
                         const std::shared_ptr< const ossPoolString > &indexName ) noexcept
      : _clFullName( clFullName ), _indexName( indexName )
      {
      }
      virtual ~_rtnIndexStatInfo() = default;

   public:
      virtual const CHAR *getIndexName() const override
      {
         return _indexName->c_str();
      }

      virtual const CHAR *getCLFullName() const override
      {
         return _clFullName->c_str();
      }

      virtual UINT64 getCreateTime() const override
      {
         return _createTime;
      }
      virtual UINT64 getSampleRecords() const override
      {
         return _sampleRecords;
      }
      virtual UINT64 getTotalRecords() const override
      {
         return _totalRecords;
      }

      virtual UINT32 getIndexPages() const override
      {
         return _indexPages;
      }

      virtual UINT32 getIndexLevels() const override
      {
         return _indexLevels;
      }

      virtual BOOLEAN isUnique() const override
      {
         return _isUnique;
      }

      virtual const BSONObj &getKeyPattern() const override
      {
         return _keyPattern;
      }

      virtual UINT32 getNumKeys() const override
      {
         return _keyPattern.nFields();
      }

      virtual UINT64 getDistinctValues() const override
      {
         return _distinctValues;
      }

      virtual FLOAT64 getNullFrac() const override
      {
         return (FLOAT64)_nullFrac / (FLOAT64)RTN_STAT_FRACTION_SCALE;
      }

      virtual FLOAT64 getUndefFrac() const override
      {
         return (FLOAT64)_undefFrac / (FLOAT64)RTN_STAT_FRACTION_SCALE;
      }

      virtual INT32 evalRangeOperator( rtnStatKey &startKey,
                                       rtnStatKey &stopKey,
                                       FLOAT64 &predSelectivity,
                                       FLOAT64 &scanSelectivity ) const override;

      virtual INT32 evalETOperator( rtnStatKey &key,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const override;

      virtual INT32 evalGTOperator( rtnStatKey &startKey,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const override;

      virtual INT32 evalLTOperator( rtnStatKey &stopKey,
                                    FLOAT64 &predSelectivity,
                                    FLOAT64 &scanSelectivity ) const override;

      virtual BOOLEAN isValidForEstimate() const override
      {
         return _mcvSet.getSize() > 0;
      }

      BSONObj toBson() const;

      void toBson( BSONObjBuilder &builder ) const;

   public:
      OSS_INLINE void setIndexPages( UINT32 indexPages )
      {
         _indexPages = indexPages;
      }

      OSS_INLINE void setIndexLevels( UINT32 indexLevels )
      {
         _indexLevels = indexLevels;
      }

      OSS_INLINE void setUnique( BOOLEAN isUnique )
      {
         _isUnique = isUnique;
      }

      INT32 setKeyPattern( const BSONObj &keyPattern );

      OSS_INLINE void setDistinctValues( UINT64 distinctValues )
      {
         _distinctValues = distinctValues;
      }

      OSS_INLINE void setNullFrac( UINT16 nullFrac )
      {
         _nullFrac = nullFrac;
      }

      OSS_INLINE void setUndefFrac( UINT16 undefFrac )
      {
         _undefFrac = undefFrac;
      }

      INT32 postInit();

      INT32 initMCVSet( UINT32 allocSize )
      {
         _mcvSet.clear();
         return _mcvSet.init( 0, allocSize );
      }

      INT32 pushMCVSet( const BSONObj &boValue, FLOAT64 fraction );

   private:
      INT32 _initMCV( const BSONObj &boMCV );
      
      INT32 _evalOperator( rtnStatKey *pStartKey,
                           rtnStatKey *pStopKey,
                           FLOAT64 &predSelectivity,
                           FLOAT64 &scanSelectivity ) const;

   protected:
      std::shared_ptr< const ossPoolString > _clFullName;
      std::shared_ptr< const ossPoolString > _indexName;
      // Number of index pages
      UINT32 _indexPages = RTN_STAT_DEF_TOTAL_PAGES;
      // Number of index levels
      UINT32 _indexLevels = RTN_STAT_DEF_IDX_LEVELS;
      BOOLEAN _isUnique = FALSE;
      BSONObj _keyPattern;
      // Number of distinct values in the index
      UINT64 _distinctValues = 0;
      UINT16 _nullFrac = 0;
      UINT16 _undefFrac = 0;
      rtnStatMCVSet _mcvSet;
   };
   using rtnIndexStatInfo = _rtnIndexStatInfo;
   using RTN_INDEX_STAT_PTR = std::shared_ptr< rtnIndexStatInfo >;
   using CONST_RTN_INDEX_STAT_PTR = std::shared_ptr< const rtnIndexStatInfo >;

   class _rtnCollectionStatInfo : public _rtnStatBase, public ICollectionStatInfo
   {
   public:
      static INT32 buildFromBson( const BSONObj &,
                                  std::shared_ptr< _rtnCollectionStatInfo > &clStatPtr );

   public:
      _rtnCollectionStatInfo( const std::shared_ptr< const ossPoolString > &clFullName ) noexcept
      : _name( clFullName )
      {
      }

      _rtnCollectionStatInfo( const std::shared_ptr< const ossPoolString > &clFullName,
                              UINT64 createTime,
                              UINT64 sampleRecords,
                              UINT64 totalRecords,
                              UINT32 totalDataPages,
                              UINT64 totalDataSize,
                              UINT32 avgNumFields ) noexcept
      : _rtnStatBase( createTime, sampleRecords, totalRecords )
      , _name( clFullName )
      , _totalDataPages( totalDataPages )
      , _totalDataSize( totalDataSize )
      , _avgNumFields( avgNumFields )
      {
      }

   public:
      virtual const CHAR *getCLFullName() const override
      {
         return _name->c_str();
      }

      virtual UINT64 getCreateTime() const override
      {
         return _createTime;
      }
      virtual UINT64 getSampleRecords() const override
      {
         return _sampleRecords;
      }
      virtual UINT64 getTotalRecords() const override
      {
         return _totalRecords;
      }

      virtual UINT32 getTotalDataPages() const override
      {
         return _totalDataPages;
      }
      virtual UINT64 getTotalDataSize() const override
      {
         return _totalDataSize;
      }
      virtual UINT32 getAvgNumFields() const override
      {
         return _avgNumFields;
      }

      virtual UINT32 getIndexNum() const override
      {
         return _vecIndexStat.size();
      }

      virtual std::shared_ptr< const IIndexStatInfo > at( UINT32 position ) const override;

      virtual std::shared_ptr< const IIndexStatInfo > seek( const CHAR *indexName ) const override;

   public:
      BSONObj toBson() const;

      void toBson( BSONObjBuilder &builder ) const;
      
   public:
      OSS_INLINE const std::shared_ptr< const ossPoolString > getCLFullNameSharedPtr() const
      {
         return _name;
      }

      OSS_INLINE const ossPoolVector< CONST_RTN_INDEX_STAT_PTR > &getIndexStatVec() const
      {
         return _vecIndexStat;
      }

      OSS_INLINE ossPoolVector< CONST_RTN_INDEX_STAT_PTR > &getIndexStatVec()
      {
         return _vecIndexStat;
      }

      OSS_INLINE void setTotalDataPages( UINT32 totalDataPages )
      {
         _totalDataPages = totalDataPages;
      }

      OSS_INLINE void setTotalDataSize( UINT64 totalDataSize )
      {
         _totalDataSize = totalDataSize;
      }

      OSS_INLINE void setAvgNumFields( UINT32 avgNumFields )
      {
         _avgNumFields = avgNumFields;
      }

   protected:
      // name is collection full name like [csName].[clName]
      std::shared_ptr< const ossPoolString > _name = nullptr;
      UINT32 _totalDataPages = RTN_STAT_DEF_TOTAL_PAGES;
      UINT64 _totalDataSize = RTN_STAT_DEF_DATA_SIZE * RTN_STAT_DEF_TOTAL_RECORDS;
      UINT32 _avgNumFields = RTN_STAT_DEF_AVG_NUM_FIELDS;

      ossPoolVector< CONST_RTN_INDEX_STAT_PTR > _vecIndexStat;
   };
   using rtnCollectionStatInfo = _rtnCollectionStatInfo;
   using RTN_CL_STAT_PTR = std::shared_ptr< rtnCollectionStatInfo >;
   using CONST_RTN_CL_STAT_PTR = std::shared_ptr< const rtnCollectionStatInfo >;
} // namespace engine

#endif