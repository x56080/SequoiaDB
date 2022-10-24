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

   Source File Name = clsResourceStatDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/12/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RESOURCE_STAT_DEF_HPP__
#define CLS_RESOURCE_STAT_DEF_HPP__

#include "oss.hpp"
#include "ossTypes.h"
#include "dmsStatUnit.hpp"
#include "utilSharedPtrMaker.hpp"
#include "utilUniqueID.hpp"
#include <memory>

namespace engine
{
   class _clsStatBase : public SDBObject
   {
   public:
      _clsStatBase() = default;
      _clsStatBase( UINT64 createTime, UINT64 sampleRecords, UINT64 totalRecords )
      : _createTime( createTime ), _sampleRecords( sampleRecords ), _totalRecords( totalRecords )
      {
      }

   public:
      OSS_INLINE UINT64 getCreateTime() const
      {
         return _createTime;
      }
      OSS_INLINE UINT64 getSampleRecords() const
      {
         return _sampleRecords;
      }
      OSS_INLINE UINT64 getTotalRecords() const
      {
         return _totalRecords;
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
      UINT64 _sampleRecords = DMS_STAT_DEF_TOTAL_RECORDS;
      // Number of records in the collection when collecting this statistics
      UINT64 _totalRecords = DMS_STAT_DEF_TOTAL_RECORDS;
   };
   using clsStatBase = _clsStatBase;

   class _clsCLStat : public _clsStatBase
   {
   public:
      static constexpr const CHAR *CLS_CL_STAT_AVG_NUM_FIELDS = "AvgNumFields";

   public:
      static INT32 buildFromBson( const BSONObj &, std::shared_ptr< _clsCLStat > &clStatPtr );

   public:
      _clsCLStat() = default;
      _clsCLStat( UINT64 createTime,
                  UINT64 sampleRecords,
                  UINT64 totalRecords,
                  UINT32 totalDataPages,
                  UINT64 totalDataSize,
                  UINT32 avgNumFields )
      : _clsStatBase( createTime, sampleRecords, totalRecords )
      , _totalDataPages( totalDataPages )
      , _totalDataSize( totalDataSize )
      , _avgNumFields( avgNumFields )
      {
      }

   public:
      OSS_INLINE UINT32 getTotalDataPages() const
      {
         return _totalDataPages;
      }
      OSS_INLINE UINT64 getTotalDataSize() const
      {
         return _totalDataSize;
      }
      OSS_INLINE UINT32 getAvgNumFields() const
      {
         return _avgNumFields;
      }

      BSONObj toBson() const;
      void toBson( BSONObjBuilder &builder ) const;

   public:
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
      UINT32 _totalDataPages = DMS_STAT_DEF_TOTAL_PAGES;
      UINT64 _totalDataSize = DMS_STAT_DEF_DATA_SIZE * DMS_STAT_DEF_TOTAL_RECORDS;
      UINT32 _avgNumFields = DMS_STAT_DEF_AVG_NUM_FIELDS;
   };

   using clsCLStat = _clsCLStat;
   using CLS_CL_STAT_PTR = std::shared_ptr< clsCLStat >;
   using CONST_CLS_CL_STAT_PTR = std::shared_ptr< const clsCLStat >;
   static CLS_CL_STAT_PTR CLS_DEFAULT_CL_STAT = makeSharedPtrFromPool< clsCLStat >();

   class _clsIndexStat : public _clsStatBase
   {
   public:
      static INT32 buildFromBson( const BSONObj &obj,
                                  std::shared_ptr< _clsIndexStat > &indexStatPtr );

   public:
      _clsIndexStat() : _mcvSet(){};
      _clsIndexStat( UINT64 createTime,
                     UINT64 sampleRecords,
                     UINT64 totalRecords,
                     UINT32 indexPages,
                     UINT32 indexLevels,
                     UINT64 distinctValues,
                     UINT16 nullFrac,
                     UINT16 undefFrac )
      : _clsStatBase( createTime, sampleRecords, totalRecords )
      , _indexPages( indexPages )
      , _indexLevels( indexLevels )
      , _distinctValues( distinctValues )
      , _nullFrac( nullFrac )
      , _undefFrac( undefFrac )
      {
      }

   public:
      OSS_INLINE UINT32 getIndexPages() const
      {
         return _indexPages;
      }

      OSS_INLINE UINT32 getIndexLevels() const
      {
         return _indexLevels;
      }

      OSS_INLINE UINT64 getDistinctValues() const
      {
         return _distinctValues;
      }

      OSS_INLINE double getNullFrac() const
      {
         return (double)_nullFrac / (double)CLS_STAT_FRACTION_SCALE;
      }

      OSS_INLINE double getUndefFrac() const
      {
         return (double)_undefFrac / (double)CLS_STAT_FRACTION_SCALE;
      }

      INT32 evalRangeOperator( clsStatKey &startKey,
                               clsStatKey &stopKey,
                               double &predSelectivity,
                               double &scanSelectivity ) const;

      INT32 evalETOperator( clsStatKey &key,
                            double &predSelectivity,
                            double &scanSelectivity ) const;

      INT32 evalGTOperator( clsStatKey &startKey,
                            double &predSelectivity,
                            double &scanSelectivity ) const;

      INT32 evalLTOperator( clsStatKey &stopKey,
                            double &predSelectivity,
                            double &scanSelectivity ) const;

      OSS_INLINE BOOLEAN isValidForEstimate() const
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

      INT32 postInit( BOOLEAN isUnique, UINT32 numKeys, const BSONObj &keyPattern )
      {
         INT32 rc = SDB_OK;
         // Initialize the distinct value number if it's not found
         if ( 0 == _distinctValues )
         {
            if ( isUnique )
            {
               _distinctValues = _totalRecords;
            }
            else
            {
               _distinctValues = _mcvSet.getSize();
            }
         }

         rc = _mcvSet.checkValues( numKeys, keyPattern );
         PD_RC_CHECK( rc, PDWARNING, "Failed to set numKeys of MCV set, rc: %d", rc );

         _mcvSet.setTotalFrac();
      done:
         return rc;
      error:
         goto done;
      }

      INT32 initMCVSet( UINT32 allocSize )
      {
         _mcvSet.clear();
         return _mcvSet.init( 0, allocSize );
      }

      INT32 pushMCVSet( const BSONObj &keyPattern, const BSONObj &boValue, double fraction );

   protected:
      INT32 _initMCV( const BSONObj &boMCV );

   protected:
      // Number of index pages
      UINT32 _indexPages = DMS_STAT_DEF_TOTAL_PAGES;

      // Number of index levels
      UINT32 _indexLevels = DMS_STAT_DEF_IDX_LEVELS;

      // Number of distinct values in the index
      UINT64 _distinctValues = 0;

      UINT16 _nullFrac = 0;
      UINT16 _undefFrac = 0;

      clsStatMCVSet _mcvSet;
   };
   using clsIndexStat = _clsIndexStat;
   using CLS_INDEX_STAT_PTR = std::shared_ptr< clsIndexStat >;
   using CONST_CLS_INDEX_STAT_PTR = std::shared_ptr< const clsIndexStat >;
   static CLS_INDEX_STAT_PTR CLS_DEFAULT_INDEX_STAT = makeSharedPtrFromPool< clsIndexStat >();
} // namespace engine

#endif