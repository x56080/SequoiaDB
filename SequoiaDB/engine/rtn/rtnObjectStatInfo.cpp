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

   Source File Name = rtnObjectStatInfo.cpp

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
#include "rtnObjectStatInfo.hpp"
#include "msgDef.h"
#include "ossUtil.hpp"
#include "utilSharedPtrMaker.hpp"
#include <exception>

namespace engine
{
   INT32 _rtnCollectionStatInfo::buildFromBson(
      const BSONObj &boStat,
      std::shared_ptr< _rtnCollectionStatInfo > &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();

      try
      {
         BSONElement beItem;
         // Required fields
         beItem = boStat.getField( RTN_STAT_COLLECTION_SPACE );
         PD_CHECK( beItem.type() == bson::String, SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_COLLECTION_SPACE );
         ossPoolString clFullName = beItem.poolStr();

         beItem = boStat.getField( RTN_STAT_COLLECTION );
         PD_CHECK( beItem.type() == bson::String, SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_COLLECTION );
         clFullName.push_back( '.' );
         clFullName.append( beItem.poolStr() );

         std::shared_ptr< rtnCollectionStatInfo > ptr =
            makeSharedPtrFromPool< rtnCollectionStatInfo >(
               makeSharedPtrFromPool< ossPoolString >( std::move( clFullName ) ) );
         PD_CHECK( ptr, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for collection statistics" );

         beItem = boStat.getField( RTN_STAT_CREATE_TIME );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   RTN_STAT_CREATE_TIME );
         ptr->_createTime = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_SAMPLE_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_SAMPLE_RECORDS );
         ptr->_sampleRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_TOTAL_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_TOTAL_RECORDS );
         ptr->_totalRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_TOTAL_DATA_PAGES );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_TOTAL_DATA_PAGES );
         ptr->_totalDataPages = (UINT32)beItem.numberInt();

         beItem = boStat.getField( FIELD_NAME_TOTAL_DATA_SIZE );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_TOTAL_DATA_SIZE );
         ptr->_totalDataSize = (UINT64)beItem.numberLong();

         beItem = boStat.getField( RTN_CL_STAT_AVG_NUM_FIELDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   RTN_CL_STAT_AVG_NUM_FIELDS );
         ptr->_avgNumFields = (UINT32)beItem.numberInt();

         clStatPtr = std::move( ptr );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR,
                 "Failed to initialize statistics object, received "
                 "unexpected error: %s",
                 e.what() );
         rc = ossException2RC( &e );
         goto error;
      }
   done:
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   void _rtnCollectionStatInfo::toBson( BSONObjBuilder &builder ) const
   {
      UINT32 pos = _name->find( '.' );
      StringData clShortName( _name->data() + pos + 1, _name->size() - pos - 1 );
      builder.appendStrWithNoTerminating( RTN_STAT_COLLECTION_SPACE, _name->data(), pos );
      builder.append( RTN_STAT_COLLECTION, clShortName );
      builder.append( RTN_STAT_CREATE_TIME, (INT64)getCreateTime() );
      builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)getSampleRecords() );
      builder.append( FIELD_NAME_TOTAL_RECORDS, (INT64)getTotalRecords() );
      builder.append( FIELD_NAME_TOTAL_DATA_PAGES, (INT32)getTotalDataPages() );
      builder.append( FIELD_NAME_TOTAL_DATA_SIZE, (INT64)getTotalDataSize() );
      builder.append( RTN_CL_STAT_AVG_NUM_FIELDS, (INT32)getAvgNumFields() );
   }

   BSONObj _rtnCollectionStatInfo::toBson() const
   {
      BSONObjBuilder builder;
      toBson( builder );
      return builder.obj();
   }

   INT32 _rtnIndexStatInfo::setKeyPattern( const BSONObj &keyPattern )
   {
      INT32 rc = SDB_OK;
      _keyPattern = keyPattern.getOwned();
      PD_CHECK( _keyPattern.nFields(), SDB_INVALIDARG, error, PDWARNING, "Empty key pattern" );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnIndexStatInfo::postInit()
   {
      INT32 rc = SDB_OK;
      // Initialize the distinct value number if it's not found
      if ( 0 == _distinctValues )
      {
         if ( _isUnique )
         {
            _distinctValues = _totalRecords;
         }
         else
         {
            _distinctValues = _mcvSet.getSize();
         }
      }

      rc = _mcvSet.checkValues( getNumKeys(), _keyPattern );
      PD_RC_CHECK( rc, PDWARNING, "Failed to set numKeys of MCV set, rc: %d", rc );

      _mcvSet.setTotalFrac();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnIndexStatInfo::pushMCVSet( const BSONObj &boValue, double fraction )
   {
      INT32 rc = SDB_OK;

      BSONObjBuilder keyBuilder;
      BSONObj boFullValue;

      UINT16 scaledFraction = 0;

      BSONObjIterator iterKey( _keyPattern );
      BSONObjIterator iterCur( boValue );
      while ( iterKey.more() && iterCur.more() )
      {
         BSONElement beKey = iterKey.next();
         BSONElement beCur = iterCur.next();

         switch ( beCur.type() )
         {
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
         case String :
         case Bool :
         case Date :
         case Timestamp :
         case jstOID :
         case jstNULL :
         case Undefined :
            break;
         default :
            // Ignore non-supported types
            goto done;
         }

         keyBuilder.appendAs( beCur, beKey.fieldName() );
      }

      PD_CHECK( !iterKey.more() && !iterCur.more(), SDB_SYS, error, PDERROR,
                "Size of keys are not matched" );

      boFullValue = keyBuilder.obj();

      // Round to 0 ~ 1.0 and scaled to 10000x
      fraction = RTN_STAT_ROUND_SELECTIVITY( fraction ) * RTN_STAT_FRACTION_SCALE;
      // Round to integer
      scaledFraction = (UINT16)RTN_STAT_ROUND_INT( fraction );
      rc = _mcvSet.pushBack( boFullValue, scaledFraction );
      PD_RC_CHECK( rc, PDERROR, "Failed to insert mcv value [%s], rc: %d",
                   boFullValue.toString( FALSE, TRUE ).c_str(), rc );

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnIndexStatInfo::_initMCV( const BSONObj &boMCV )
   {
      INT32 rc = SDB_OK;

      BSONElement beItem;

      beItem = boMCV.getField( FIELD_NAME_VALUES );
      PD_CHECK( Array == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", FIELD_NAME_VALUES );
      {
         BSONObj boValues = beItem.embeddedObject();
         BSONObjIterator iterValue( boValues );
         UINT32 idx = 0;

         if ( _mcvSet.getSize() == 0 && boValues.nFields() > 0 )
         {
            UINT32 size = boValues.nFields();
            rc = _mcvSet.init( size, size );
            PD_RC_CHECK( rc, PDWARNING, "Failed to initialize MCV, rc: %d", rc );
         }
         PD_CHECK( _mcvSet.getSize() == (UINT32)boValues.nFields(), SDB_INVALIDARG, error,
                   PDWARNING, "Field [%s] 's lenght is not matched", beItem.toString().c_str() );

         while ( iterValue.more() )
         {
            BSONElement tempVal = iterValue.next();
            PD_CHECK( Object == tempVal.type(), SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] 's type is not matched", tempVal.toString().c_str() );
            _mcvSet.setValue( idx, tempVal.embeddedObject() );
            ++idx;
         }
      }

      beItem = boMCV.getField( FIELD_NAME_FRAC );
      PD_CHECK( Array == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] is not matched", FIELD_NAME_FRAC );
      {
         BSONObj boFrac = beItem.embeddedObject();
         BSONObjIterator iterFrac( boFrac );
         UINT32 idx = 0;

         PD_CHECK( _mcvSet.getSize() == (UINT32)boFrac.nFields(), SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] 's length is not matched", beItem.toString().c_str() );

         while ( iterFrac.more() )
         {
            UINT32 frac = 0;
            BSONElement tempFrac = iterFrac.next();
            PD_CHECK( tempFrac.isNumber(), SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] 's type is not matched", tempFrac.toString().c_str() );
            frac = (UINT16)tempFrac.numberInt();
            _mcvSet.setFrac( idx, frac );
            ++idx;
         }
      }

   done:
      return rc;
   error:
      _mcvSet.clear();
      goto done;
   }

   INT32 _rtnIndexStatInfo::buildFromBson(
      const BSONObj &boStat,
      RTN_INDEX_STAT_PTR &indexStatPtr,
      const std::shared_ptr< const ossPoolString > &clFullName )
   {
      INT32 rc = SDB_OK;
      indexStatPtr.reset();

      try
      {
         BSONElement beItem;

         // Required fields
         beItem = boStat.getField( RTN_STAT_COLLECTION_SPACE );
         PD_CHECK( beItem.type() == bson::String, SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_COLLECTION_SPACE );
         std::shared_ptr< ossPoolString > parsedClFullName =
            makeSharedPtrFromPool< ossPoolString >( beItem.poolStr() );
         PD_CHECK( parsedClFullName, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for collection full name" );

         beItem = boStat.getField( RTN_STAT_COLLECTION );
         PD_CHECK( beItem.type() == bson::String, SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_COLLECTION );
         parsedClFullName->push_back( '.' );
         parsedClFullName->append( beItem.poolStr() );

         std::shared_ptr< const ossPoolString > clFullNameToSave = nullptr;
         if ( clFullName && ( *clFullName ) == *parsedClFullName)
         {
            clFullNameToSave = clFullName;
         }
         else
         {
            clFullNameToSave = parsedClFullName;
         }
         
         beItem = boStat.getField( RTN_STAT_IDX_INDEX );
         PD_CHECK( beItem.type() == bson::String, SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_IDX_INDEX );
         std::shared_ptr< const ossPoolString > indexName =
            makeSharedPtrFromPool< ossPoolString >( beItem.poolStr() );

         RTN_INDEX_STAT_PTR ptr =
            makeSharedPtrFromPool< rtnIndexStatInfo >( clFullNameToSave, indexName );
         PD_CHECK( ptr, SDB_OOM, error, PDWARNING,
                   "Failed to allocate memory for collection statistics" );

         beItem = boStat.getField( RTN_STAT_CREATE_TIME );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   RTN_STAT_CREATE_TIME );
         ptr->_createTime = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_SAMPLE_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_SAMPLE_RECORDS );
         ptr->_sampleRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_TOTAL_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_TOTAL_RECORDS );
         ptr->_totalRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( RTN_STAT_IDX_INDEX_PAGES );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   RTN_STAT_IDX_INDEX_PAGES );
         ptr->setIndexPages( (UINT32)beItem.numberInt() );

         beItem = boStat.getField( RTN_STAT_IDX_LEVELS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   RTN_STAT_IDX_LEVELS );
         ptr->setIndexLevels( (UINT32)beItem.numberInt() );

         beItem = boStat.getField( RTN_STAT_IDX_KEY_PATTERN );
         PD_CHECK( Object == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_IDX_KEY_PATTERN );
         rc = ptr->setKeyPattern( beItem.embeddedObject() );
         PD_RC_CHECK( rc, PDWARNING, "Failed to init key pattern, rc: %d", rc );

         beItem = boStat.getField( RTN_STAT_IDX_IS_UNIQUE );
         PD_CHECK( Bool == beItem.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Field [%s] is not matched", RTN_STAT_IDX_IS_UNIQUE );
         ptr->setUnique( beItem.booleanSafe() );

         // Optional fields

         beItem = boStat.getField( FIELD_NAME_NULL_FRAC );
         if ( beItem.isNumber() )
         {
            ptr->setNullFrac( (UINT16)beItem.numberInt() );
         }

         beItem = boStat.getField( FIELD_NAME_UNDEF_FRAC );
         if ( beItem.isNumber() )
         {
            ptr->setUndefFrac( (UINT16)beItem.numberInt() );
         }
         constexpr const CHAR FIELD_NAME_DISTINCT_VALUES[] = "DistinctValues";
         beItem = boStat.getField( FIELD_NAME_DISTINCT_VALUES );
         if ( beItem.isNumber() )
         {
            ptr->setDistinctValues( (UINT64)beItem.numberLong() );
         }

         beItem = boStat.getField( RTN_STAT_IDX_MCV );
         if ( Object == beItem.type() )
         {
            ptr->_initMCV( beItem.embeddedObject() );
         }

         indexStatPtr = std::move( ptr );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR,
                 "Failed to initialize statistics object, received "
                 "unexpected error: %s",
                 e.what() );
         rc = ossException2RC( &e );
         goto error;
      };
   done:
      return rc;
   error:
      indexStatPtr.reset();
      goto done;
   }

   BSONObj _rtnIndexStatInfo::toBson() const
   {
      BSONObjBuilder builder;
      toBson( builder );
      return builder.obj();
   }

   void _rtnIndexStatInfo::toBson( BSONObjBuilder &builder ) const
   {
      UINT32 pos = _clFullName->find( '.' );
      StringData clShortName( _clFullName->data() + pos + 1, _clFullName->size() - pos - 1 );
      builder.appendStrWithNoTerminating( RTN_STAT_COLLECTION_SPACE, _clFullName->data(), pos );
      builder.append( RTN_STAT_COLLECTION, clShortName );
      builder.append( RTN_STAT_IDX_INDEX, _indexName->c_str() );
      builder.append( RTN_STAT_CREATE_TIME, (INT64)getCreateTime() );
      builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)getSampleRecords() );
      builder.append( FIELD_NAME_TOTAL_RECORDS, (INT64)getTotalRecords() );
      builder.append( RTN_STAT_IDX_INDEX_PAGES, (INT32)getIndexPages() );
      builder.append( RTN_STAT_IDX_LEVELS, (INT32)getIndexLevels() );
      builder.appendBool( RTN_STAT_IDX_IS_UNIQUE, isUnique() );
      builder.append( FIELD_NAME_KEY_PATTERN, getKeyPattern() );

      if ( _mcvSet.getSize() > 0 )
      {
         BSONObjBuilder mcvBuilder( builder.subobjStart( RTN_STAT_IDX_MCV ) );

         BSONArrayBuilder mcvValueBuilder( mcvBuilder.subarrayStart( FIELD_NAME_VALUES ) );
         for ( UINT32 i = 0; i < _mcvSet.getSize(); i++ )
         {
            mcvValueBuilder.append( _mcvSet.getValue( i ) );
         }
         mcvValueBuilder.done();

         BSONArrayBuilder mcvFracBuilder( mcvBuilder.subarrayStart( FIELD_NAME_FRAC ) );
         for ( UINT32 i = 0; i < _mcvSet.getSize(); i++ )
         {
            mcvFracBuilder.append( (INT32)_mcvSet.getFracInt( i ) );
         }
         mcvFracBuilder.done();

         mcvBuilder.done();
      }
   }

   INT32 _rtnIndexStatInfo::evalRangeOperator( rtnStatKey &startKey,
                                               rtnStatKey &stopKey,
                                               double &predSelectivity,
                                               double &scanSelectivity ) const
   {
      return _evalOperator( &startKey, &stopKey, predSelectivity, scanSelectivity );
   }

   INT32 _rtnIndexStatInfo::evalETOperator( rtnStatKey &key,
                                            double &predSelectivity,
                                            double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK;

      BOOLEAN hitMCV = FALSE;

      // Special case for unique index, could be one of the totalRecords
      if ( isUnique() && key.size() == getNumKeys() )
      {
         predSelectivity = 1.0 / (double)_totalRecords;
         scanSelectivity = predSelectivity;
         goto done;
      }

      PD_CHECK( _mcvSet.getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" );

      rc = _mcvSet.evalETOperator( key, hitMCV, predSelectivity, scanSelectivity );
      PD_RC_CHECK( rc, PDWARNING, "Failed to evaluate from MCV set, rc: %d", rc );

      if ( !hitMCV )
      {
         // The value is not in MCV set, evaluate in the rest of values
         if ( _distinctValues == _mcvSet.getSize() )
         {
            predSelectivity = ( 1.0 - _mcvSet.getTotalFrac() ) * RTN_STAT_PRED_EQ_DEF_SELECTIVITY;
         }
         else
         {
            predSelectivity =
               ( 1.0 - _mcvSet.getTotalFrac() ) / (double)( _distinctValues - _mcvSet.getSize() );
         }
         scanSelectivity = predSelectivity;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnIndexStatInfo::evalGTOperator( rtnStatKey &startKey,
                                            double &predSelectivity,
                                            double &scanSelectivity ) const
   {
      return _evalOperator( &startKey, NULL, predSelectivity, scanSelectivity );
   }

   INT32 _rtnIndexStatInfo::evalLTOperator( rtnStatKey &stopKey,
                                            double &predSelectivity,
                                            double &scanSelectivity ) const
   {
      return _evalOperator( NULL, &stopKey, predSelectivity, scanSelectivity );
   }

   INT32 _rtnIndexStatInfo::_evalOperator( rtnStatKey *pStartKey,
                                           rtnStatKey *pStopKey,
                                           double &predSelectivity,
                                           double &scanSelectivity ) const
   {
      INT32 rc = SDB_OK;

      BOOLEAN hitMCV = FALSE;

      PD_CHECK( _mcvSet.getSize() > 0, SDB_INVALIDARG, error, PDWARNING,
                "No MCV set is available" );

      rc = _mcvSet.evalOperator( pStartKey, pStopKey, hitMCV, predSelectivity, scanSelectivity );
      PD_RC_CHECK( rc, PDWARNING, "Failed to evaluate from MCV set, rc: %d", rc );

      if ( !hitMCV )
      {
         // The values do not include any MCV items, try to evaluate from
         // the rest of values
         predSelectivity = ( 1.0 - _mcvSet.getTotalFrac() ) * RTN_STAT_PRED_RANGE_DEF_SELECTIVITY;
         scanSelectivity = predSelectivity;
      }

   done:
      return rc;
   error:
      goto done;
   }

   std::shared_ptr< const IIndexStatInfo > _rtnCollectionStatInfo::at( UINT32 position ) const
   {
      SDB_ASSERT( position < getIndexNum(), "out of bound" );
      return _vecIndexStat.at( position );
   }

   std::shared_ptr< const IIndexStatInfo > _rtnCollectionStatInfo::seek(
      const CHAR *indexName ) const
   {
      decltype( _vecIndexStat )::const_iterator found = std::find_if(
         _vecIndexStat.begin(), _vecIndexStat.end(),
         [ & ]( const decltype( _vecIndexStat )::value_type &indexStat ) {
            return utilStringView( indexStat->getIndexName() ) == utilStringView( indexName );
         } );
      if ( found != _vecIndexStat.end() )
      {
         return *found;
      }
      else
      {
         return nullptr;
      }
   }
} // namespace engine
