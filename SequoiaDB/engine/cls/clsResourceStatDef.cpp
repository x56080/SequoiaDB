#include "clsResourceStatDef.hpp"
#include "ossUtil.hpp"
#include "utilSharedPtrMaker.hpp"
#include <exception>

namespace engine
{
   INT32 _clsCLStat::buildFromBson( const BSONObj &boStat,
                                    std::shared_ptr< _clsCLStat > &clStatPtr )
   {
      INT32 rc = SDB_OK;
      clStatPtr.reset();
      std::shared_ptr< clsCLStat > ptr = makeSharedPtrFromPool< clsCLStat >();
      PD_CHECK( ptr, SDB_OOM, error, PDWARNING,
                "Failed to allocate memory for collection statistics" );
      try
      {
         BSONElement beItem;
         // Required fields
         beItem = boStat.getField( DMS_STAT_CREATE_TIME );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   DMS_STAT_CREATE_TIME );
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

         beItem = boStat.getField( CLS_CL_STAT_AVG_NUM_FIELDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   CLS_CL_STAT_AVG_NUM_FIELDS );
         ptr->_avgNumFields = (UINT32)beItem.numberInt();
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
      clStatPtr = std::move( ptr );
   done:
      return rc;
   error:
      clStatPtr.reset();
      goto done;
   }

   void _clsCLStat::toBson( BSONObjBuilder &builder ) const
   {
      builder.append( DMS_STAT_CREATE_TIME, (INT64)getCreateTime() );
      builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)getSampleRecords() );
      builder.append( FIELD_NAME_TOTAL_RECORDS, (INT64)getTotalRecords() );
      builder.append( FIELD_NAME_TOTAL_DATA_PAGES, (INT32)getTotalDataPages() );
      builder.append( FIELD_NAME_TOTAL_DATA_SIZE, (INT64)getTotalDataSize() );
      builder.append( CLS_CL_STAT_AVG_NUM_FIELDS, (INT32)getAvgNumFields() );
   }

   BSONObj _clsCLStat::toBson() const
   {
      BSONObjBuilder builder;
      toBson( builder );
      return builder.obj();
   }

   INT32 _clsIndexStat::pushMCVSet( const BSONObj &keyPattern,
                                    const BSONObj &boValue,
                                    double fraction )
   {
      INT32 rc = SDB_OK;

      BSONObjBuilder keyBuilder;
      BSONObj boFullValue;

      UINT16 scaledFraction = 0;

      BSONObjIterator iterKey( keyPattern );
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
      fraction = CLS_STAT_ROUND_SELECTIVITY( fraction ) * CLS_STAT_FRACTION_SCALE;
      // Round to integer
      scaledFraction = (UINT16)DMS_STAT_ROUND_INT( fraction );
      rc = _mcvSet.pushBack( boFullValue, scaledFraction );
      PD_RC_CHECK( rc, PDERROR, "Failed to insert mcv value [%s], rc: %d",
                   boFullValue.toString( FALSE, TRUE ).c_str(), rc );

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _clsIndexStat::_initMCV( const BSONObj &boMCV )
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

   INT32 _clsIndexStat::buildFromBson( const BSONObj &boStat, CLS_INDEX_STAT_PTR &indexStatPtr )
   {
      INT32 rc = SDB_OK;
      indexStatPtr.reset();
      CLS_INDEX_STAT_PTR ptr = makeSharedPtrFromPool< clsIndexStat >();
      PD_CHECK( ptr, SDB_OOM, error, PDWARNING,
                "Failed to allocate memory for collection statistics" );
      try
      {
         BSONElement beItem;

         // Required fields
         beItem = boStat.getField( DMS_STAT_CREATE_TIME );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   DMS_STAT_CREATE_TIME );
         ptr->_createTime = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_SAMPLE_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_SAMPLE_RECORDS );
         ptr->_sampleRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( FIELD_NAME_TOTAL_RECORDS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   FIELD_NAME_TOTAL_RECORDS );
         ptr->_totalRecords = (UINT64)beItem.numberLong();

         beItem = boStat.getField( DMS_STAT_IDX_INDEX_PAGES );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   DMS_STAT_IDX_INDEX_PAGES );
         ptr->setIndexPages( (UINT32)beItem.numberInt() );

         beItem = boStat.getField( DMS_STAT_IDX_LEVELS );
         PD_CHECK( beItem.isNumber(), SDB_INVALIDARG, error, PDWARNING, "Field [%s] is not matched",
                   DMS_STAT_IDX_LEVELS );
         ptr->setIndexLevels( (UINT32)beItem.numberInt() );

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

         beItem = boStat.getField( DMS_STAT_IDX_MCV );
         if ( Object == beItem.type() )
         {
            ptr->_initMCV( beItem.embeddedObject() );
         }
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
      indexStatPtr = std::move( ptr );
   done:
      return rc;
   error:
      indexStatPtr.reset();
      goto done;
   }

   BSONObj _clsIndexStat::toBson() const
   {
      BSONObjBuilder builder;
      toBson( builder );
      return builder.obj();
   }

   void _clsIndexStat::toBson( BSONObjBuilder &builder ) const
   {
      builder.append( DMS_STAT_CREATE_TIME, (INT64)getCreateTime() );
      builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)getSampleRecords() );
      builder.append( FIELD_NAME_TOTAL_RECORDS, (INT64)getTotalRecords() );
      builder.append( DMS_STAT_IDX_INDEX_PAGES, (INT32)getIndexPages() );
      builder.append( DMS_STAT_IDX_LEVELS, (INT32)getIndexLevels() );

      if ( _mcvSet.getSize() > 0 )
      {
         BSONObjBuilder mcvBuilder( builder.subobjStart( DMS_STAT_IDX_MCV ) );

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
} // namespace engine
