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

   Source File Name = stpMetaData.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpMetaData.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "stpToolCommon.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   // time wait to acquire logical time
   #define STP_META_GET_TIME_WAITTIME  ( 10 )
   // timeout to acquire logical time
   #define STP_META_GET_TIME_TIEMOUT   ( OSS_ONE_SEC )

   /*
      _stpMetaSHMContent define
    */
   // _stpMetaSHMContent is the structure of shared memory for meta data

   // eye-catcher header and tailer
   #define STP_META_EYECATCHER_HEAD ( 0xED3218AC )
   #define STP_META_EYECATCHER_TAIL ( 0xAC1832ED )

   typedef struct _stpMetaSHMContent
   {
      // constructor
      _stpMetaSHMContent()
      : _head( STP_META_EYECATCHER_HEAD ),
        _data(),
        _tail( STP_META_EYECATCHER_TAIL )
      {
      }

      // eye-catcher header
      UINT32      _head ;
      // meta data
      stpMetaData _data ;
      // eye-catcher tailer
      UINT32      _tail ;
   } stpMetaSHMContent ;

   /*
      _stpMetaData implement
    */
   _stpMetaData::_stpMetaData()
   : _version( STP_VERSION ),
     _syncInterval( STP_DEF_SYNC_INTERVAL ),
     _syncHWTime(),
     _baseHWTime(),
     _baseRealTime(),
     _offset( 0LL ),
     _slewRate( STP_DEF_SLEWRATE ),
     _timeError( STP_DEF_TIME_ERROR )
   {
   }

   _stpMetaData::~_stpMetaData()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_INITIALIZE, "_stpMetaData::initialize" )
   void _stpMetaData::initialize( UINT64 timeUS, UINT32 syncInterval )
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_INITIALIZE ) ;

      // initialize version
      _version = STP_VERSION ;
      // set synchronize interval
      _syncInterval = syncInterval ;

      // reset time with given time
      reset( timeUS ) ;

      // reset synchronize time
      _syncHWTime.reset() ;

      PD_TRACE_EXIT( SDB__STPMETADATA_INITIALIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_RESET, "_stpMetaData::reset" )
   void _stpMetaData::reset( UINT64 timeUS )
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_RESET ) ;

      // set as default values
      _baseHWTime.sampleMonotonic() ;
      _baseRealTime.fromMicroSecond( timeUS ) ;
      _offset = 0LL ;
      _slewRate = STP_DEF_SLEWRATE ;
      _timeError = STP_DEF_TIME_ERROR ;

      // set synchronize time
      _syncHWTime.sampleMonotonic() ;

      PD_TRACE_EXIT( SDB__STPMETADATA_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_GETLOGICALTIMENS_QUICK, "_stpMetaData::getLogicalTimeNS" )
   INT32 _stpMetaData::getLogicalTimeNS( stpLogicalTimeNS &time ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_GETLOGICALTIMENS_QUICK ) ;

      UINT32 waitTime = 0, retryTime = 0 ;

      while ( TRUE )
      {
         // check synchronize
         rc = getLogicalTimeNS( time, FALSE, TRUE, waitTime ) ;
         if ( STP_SYNC_BUSY == rc && retryTime < STP_META_GET_TIME_TIEMOUT )
         {
            // the STP is synchronizing, wait for a while
            ossSleep( STP_META_GET_TIME_WAITTIME ) ;
            retryTime += STP_META_GET_TIME_WAITTIME ;
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, "
                      "rc: %d", rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA_GETLOGICALTIMENS_QUICK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_GETLOGICALTIMENS, "_stpMetaData::getLogicalTimeNS" )
   INT32 _stpMetaData::getLogicalTimeNS( stpLogicalTimeNS &time,
                                         BOOLEAN monotonic,
                                         BOOLEAN checkSync,
                                         UINT32 &waitTimeUS ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_GETLOGICALTIMENS ) ;

      if ( checkSync )
      {
         // if synchronize check is needed, need check if current meta data
         // is expired ( not synchronized for a synchronize interval period )
         stpHPTime syncHWTime = _syncHWTime ;
         UINT64 syncLimit = STP_SEC_TO_NANOSEC( _syncInterval +
                                                STP_MIN_SYNC_INTERVAL ) ;
         stpHPTime curHWTime( STP_SAMPLE_TIME_MONOTONIC ) ;

         // check if current hardware time is larger than synchronized hardware
         // time, if not, means the time is pushed forward because it is ahead
         // of source after synchronization, and should to be valid after
         // current hardware time passed synchronize hardware time
         // NOTE: in this case, caller could wait for synchronize time to be
         //       passed later
         if ( monotonic && curHWTime < syncHWTime )
         {
            // it is not thread-safe, even after checking, recheck synchronize
            // hardware time
            // checking busy first before checking ahead after synchronize
            PD_CHECK( syncHWTime == _syncHWTime, STP_SYNC_BUSY, error, PDERROR,
                      "Failed to get STP logical time, "
                      "it is busy for synchronizing, ignore this "
                      "logical time" ) ;

            rc = STP_TIME_AHEAD_AFTER_SYNC ;
            waitTimeUS = syncHWTime.toMicroSecond() -
                         curHWTime.toMicroSecond() ;

            PD_LOG( PDWARNING,
                    "STP logical time is not available now, time synchronized "
                    "is ahead of source, need wait for current hardware time "
                    "passes last synchronized hardware time, "
                    "last synchronized hardware time [%llu], "
                    "current hardware time [%llu]",
                    syncHWTime.toMicroSecond(),
                    curHWTime.toMicroSecond() ) ;

            goto error ;
         }
         else
         {
            waitTimeUS = 0 ;
         }

         // check if current hardware time is smaller than synchronized
         // hardware time with a synchronize limit buffer, if not, means the
         // meta data is expired ( long time without synchronize )
         PD_CHECK( curHWTime <= syncHWTime + syncLimit,
                   STP_SYNC_FAILED, error, PDERROR,
                   "Failed to get STP logical time, synchronize failed, "
                   "last synchronized hardware time [%llu], "
                   "current hardware time [%llu], synchronize limit [%llu]",
                   syncHWTime.toMicroSecond(), curHWTime.toMicroSecond(),
                   STP_NANOSEC_TO_MICROSEC( syncLimit ) ) ;

         // copy logical time to output
         time.setTime( _getLTValue( curHWTime ) ) ;
         time.setTimeError( _timeError ) ;

         // it is not thread-safe, even after checking, recheck synchronize
         // hardware time
         // NOTE: when busy synchronizing, caller could retry immediate
         PD_CHECK( syncHWTime == _syncHWTime, STP_SYNC_BUSY, error, PDERROR,
                   "Failed to get STP logical time, "
                   "it is busy for synchronizing, ignore this logical time" ) ;
      }
      else
      {
         // copy logical time to output
         time.setTime( _getLTValue() ) ;
         time.setTimeError( _timeError ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_GETLOGICALTIMEUS, "_stpMetaData::getLogicalTimeUS" )
   INT32 _stpMetaData::getLogicalTimeUS( stpLogicalTimeUS &time,
                                         BOOLEAN monotonic,
                                         BOOLEAN checkSync,
                                         UINT32 &waitTimeUS ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_GETLOGICALTIMEUS ) ;

      stpLogicalTimeNS timeNS ;

      // get logical time in nanosecond
      rc = getLogicalTimeNS( timeNS, monotonic, checkSync, waitTimeUS ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      // copy to output ( change unit to microsecond )
      time = timeNS ;

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA_GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_RESETSLEWRATE, "_stpMetaData::resetSlewRate" )
   void _stpMetaData::resetSlewRate()
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_RESETSLEWRATE ) ;

      // reset slew rate to default
      setSlewRate( STP_DEF_SLEWRATE ) ;

      PD_TRACE_EXIT( SDB__STPMETADATA_RESETSLEWRATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_TOBSON, "_stpMetaData::toBSON" )
   INT32 _stpMetaData::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_TOBSON ) ;

      try
      {
         BSONObjBuilder metaBuilder(
                           builder.subobjStart( STP_FIELD_NAME_META_DATA ) ) ;

         // append version
         metaBuilder.append( STP_FIELD_NAME_VERSION, (INT32)_version ) ;
         // append synchronize interval
         metaBuilder.append( STP_FIELD_NAME_SYNC_INTERVAL,
                             (INT32)_syncInterval ) ;

         // append synchronize hardware time
         rc = _timeToBSON( metaBuilder,
                           STP_FIELD_NAME_SYNC_HW_TIME,
                           _syncHWTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_SYNC_HW_TIME, rc ) ;

         // append based hardware time
         rc = _timeToBSON( metaBuilder,
                           STP_FIELD_NAME_BASE_HW_TIME,
                           _baseHWTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_BASE_HW_TIME, rc ) ;

         // append based real time
         rc = _timeToBSON( metaBuilder,
                           STP_FIELD_NAME_BASE_REAL_TIME,
                           _baseRealTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_BASE_REAL_TIME, rc ) ;

         // append offset
         metaBuilder.append( STP_FIELD_NAME_OFFSET, _offset ) ;
         // append slew rate
         metaBuilder.append( STP_FIELD_NAME_SLEW_RATE, (INT64)_slewRate ) ;
         // append time error
         metaBuilder.append( STP_FIELD_NAME_TIME_ERROR, (INT32)_timeError ) ;

         metaBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for meta data, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA__TIMETOBSON, "_stpMetaData::_timeToBSON" )
   INT32 _stpMetaData::_timeToBSON( BSONObjBuilder &builder,
                                    const CHAR *fieldName,
                                    const stpHPTime &hpTime ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA__TIMETOBSON ) ;

      // format high precision time to BSON format with given field name
      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( fieldName ) ) ;

         rc = hpTime.toBSON( subBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for time [%s], "
                      "rc: %d", fieldName, rc ) ;

         subBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for time [%s], "
                 "occurred unexpected error: %s", fieldName, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA__TIMETOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_FROMBSON, "_stpMetaData::fromBSON" )
   INT32 _stpMetaData::fromBSON( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_FROMBSON ) ;

      try
      {
         BSONElement subElement ;

         // parse version
         subElement = object.getField( STP_FIELD_NAME_VERSION ) ;
         PD_CHECK( NumberInt == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not integer",
                   STP_FIELD_NAME_VERSION ) ;
         _version = (UINT32)( subElement.numberInt() ) ;

         // parse synchronize interval
         subElement = object.getField( STP_FIELD_NAME_SYNC_INTERVAL ) ;
         PD_CHECK( NumberInt == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not integer",
                   STP_FIELD_NAME_SYNC_INTERVAL ) ;
         _syncInterval = (UINT32)( subElement.numberInt() ) ;

         // parse synchronize hardware time
         rc = _timeFromBSON( object, STP_FIELD_NAME_SYNC_HW_TIME,
                             _syncHWTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse [%s] from BSON, rc: %d",
                      STP_FIELD_NAME_SYNC_HW_TIME, rc ) ;

         // parse based hardware time
         rc = _timeFromBSON( object, STP_FIELD_NAME_BASE_HW_TIME,
                             _baseHWTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse [%s] from BSON, rc: %d",
                      STP_FIELD_NAME_BASE_HW_TIME, rc ) ;

         // parse based real time
         rc = _timeFromBSON( object, STP_FIELD_NAME_BASE_REAL_TIME,
                             _baseRealTime ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse [%s] from BSON, rc: %d",
                      STP_FIELD_NAME_BASE_REAL_TIME, rc ) ;

         // parse offset
         subElement = object.getField( STP_FIELD_NAME_OFFSET ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET ) ;
         _offset = subElement.numberLong() ;

         // parse slew rate
         subElement = object.getField( STP_FIELD_NAME_SLEW_RATE ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_SLEW_RATE ) ;
         _slewRate = (UINT64)subElement.numberLong() ;

         // parse time error
         subElement = object.getField( STP_FIELD_NAME_TIME_ERROR ) ;
         PD_CHECK( NumberInt == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not integer",
                   STP_FIELD_NAME_TIME_ERROR ) ;
         _timeError = (UINT32)subElement.numberInt() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for meta, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA__TIMEFROMBSON, "_stpMetaData::_timeFromBSON" )
   INT32 _stpMetaData::_timeFromBSON( const BSONObj &object,
                                      const CHAR *fieldName,
                                      stpHPTime &hpTime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPMETADATA__TIMETOBSON ) ;

      try
      {
         // get field for high precision time to parse
         BSONElement subElement = object.getField( fieldName ) ;
         // check type
         PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not an object",
                   fieldName ) ;
         // parse from BSON
         rc = hpTime.fromBSON( subElement.embeddedObject() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse time for [%s], rc: %d",
                      fieldName, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON for time [%s], "
                 "occurred unexpected error: %s", fieldName, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPMETADATA__TIMETOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   ossPoolString _stpMetaData::toString() const
   {
      StringBuilder ss ;
      ss << "baseHardwareTime [" << _baseHWTime.toMicroSecond() << "], " <<
            "basedRealTime [" << _baseRealTime.toMicroSecond() << "], " <<
            "offset [" << _offset << "], " <<
            "slewRate [" << _slewRate << "], " <<
            "timeError [" << _timeError << "]" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_ADJUSTLOGICALTIME, "_stpMetaData::adjustLogicalTime" )
   void _stpMetaData::adjustLogicalTime( INT64 syncOffset )
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_ADJUSTLOGICALTIME ) ;

      if ( syncOffset >= 0LL )
      {
         // if offset >= 0, update offset directly
         _offset += syncOffset ;
         // update synchronize hardware time with monotonic time
         // NOTE: no need to adjust by offset, if hardware time of logical time
         //       is in [ _syncHWTime, _syncHWTime + syncInterval ], this
         //       logical time is available for STP agent
         _syncHWTime.sampleMonotonic() ;
      }
      else
      {
         // if offset < 0, update offset and also push forward synchronize
         // hardware time with offset
         // which means the meta data should be valid after the current
         // hardware time should pass synchronize hardware time
         _offset += syncOffset ;
         // update synchronize hardware time with monotonic time
         _syncHWTime.sampleMonotonic() ;
         // NOTE: offset is negative now, need push synchronize hardware time
         //       forward to make that hardware time of logical time is ahead
         //       to synchronize hardware time, it means that logical time is
         //       unavailable for STP agent
         _syncHWTime.adjust( -1 * syncOffset ) ;
      }

      PD_TRACE_EXIT( SDB__STPMETADATA_ADJUSTLOGICALTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_ADJUSTLOGICALTIME_TE, "_stpMetaData::adjustLogicalTime" )
   void _stpMetaData::adjustLogicalTime( INT64 syncOffset,
                                         UINT32 timeError )
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_ADJUSTLOGICALTIME_TE ) ;

      // adjust by offset
      adjustLogicalTime( syncOffset ) ;
      // set time error
      _timeError = timeError ;

      PD_TRACE_EXIT( SDB__STPMETADATA_ADJUSTLOGICALTIME_TE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_ADJUSTSLEWRATE, "_stpMetaData::adjustSlewRate" )
   void _stpMetaData::adjustSlewRate( UINT64 sourceInterval,
                                      UINT64 localInterval )
   {
      PD_TRACE_ENTRY( SDB__STPMETADATA_ADJUSTSLEWRATE ) ;

      UINT64 oldSlewRate = _slewRate ;
      INT64 oldOffset = _offset ;

      // calculate slew rate
      // slew rate = old slew rate * source interval / local interval
      _slewRate = (UINT64)( (FLOAT64)( oldSlewRate ) *
                            (FLOAT64)( sourceInterval ) /
                            (FLOAT64)( localInterval ) ) ;

      if ( oldSlewRate != _slewRate )
      {
         // based on the formula to calculate logical time
         //
         //    logical time =
         //          ( current HW time -
         //            base HW time ) * slew rate / STP_DEF_SLEWRATE +
         //          offset + base real time
         //
         // simplify to
         //
         //    LT = HWDiff * SR + O
         //
         // if we adjust slew rate, we need to adjust offset as well, we need
         // to make the LT are the same before and after adjusting slew rate
         //
         //                  LT1 = LT2
         //    HWDiff * SR1 + O1 + BaseRT = HWDiff * SR2 + O2 + BaseRT
         //
         // so we have
         //
         //    O2 = ( HWDiff * SR1 - HWDiff * SR2 ) + O1
         //
         stpHPTime curHWTime ;
         curHWTime.sampleMonotonic() ;
         stpHPTime result = curHWTime - _baseHWTime ;
         // HPTime only handles positive values, so we need to process by
         // different ways
         if ( oldSlewRate > _slewRate )
         {
            UINT64 diff = oldSlewRate - _slewRate ;
            result.scale( diff ) ;
            diff = result.toNanoSecond() ;
            _offset = _offset + (INT64)diff ;
         }
         else
         {
            UINT64 diff = _slewRate - oldSlewRate ;
            result.scale( diff ) ;
            diff = result.toNanoSecond() ;
            _offset = _offset - (INT64)diff ;
         }

         if ( curHWTime > _syncHWTime )
         {
            _syncHWTime = curHWTime ;
         }

         PD_LOG( PDEVENT, "Adjust slew rate from [%llu] to [%llu] "
                 "offset from [%lld] to [%lld]", oldSlewRate, _slewRate,
                 oldOffset, _offset ) ;
      }

      PD_TRACE_EXIT( SDB__STPMETADATA_ADJUSTSLEWRATE ) ;
   }

   UINT32 _stpMetaData::getBufferSize()
   {
      return sizeof( stpMetaSHMContent ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_GETBUFFER, "_stpMetaData::getBuffer" )
   stpMetaData *_stpMetaData::getBuffer( CHAR *buffer )
   {
      stpMetaData *data = NULL ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_GETBUFFER ) ;

      if ( NULL != buffer )
      {
         // check eye-catcher of header and tailer in shared memory buffer
         stpMetaSHMContent *content = (stpMetaSHMContent *)( buffer ) ;
         if ( STP_META_EYECATCHER_HEAD == content->_head &&
              STP_META_EYECATCHER_TAIL == content->_tail )
         {
            data = &( content->_data ) ;
         }
      }

      PD_TRACE_EXIT( SDB__STPMETADATA_GETBUFFER ) ;

      return data ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPMETADATA_NEWBUFFER, "_stpMetaData::newBuffer" )
   stpMetaData *_stpMetaData::newBuffer( CHAR *buffer )
   {
      stpMetaData *data = NULL ;

      PD_TRACE_ENTRY( SDB__STPMETADATA_NEWBUFFER ) ;

      if ( NULL != buffer )
      {
         // construct meta data from shared memory buffer
         stpMetaSHMContent *content = (stpMetaSHMContent *)( buffer ) ;
         content->_head = STP_META_EYECATCHER_HEAD ;
         data = new( &( content->_data ) ) stpMetaData() ;
         content->_tail = STP_META_EYECATCHER_TAIL ;
      }

      PD_TRACE_EXIT( SDB__STPMETADATA_NEWBUFFER ) ;

      return data ;
   }

}
