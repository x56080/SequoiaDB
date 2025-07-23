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

   Source File Name = stpSyncStats.cpp

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
#include "stpSyncStats.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpSyncRecord implement
    */
   _stpSyncRecord::_stpSyncRecord()
   : _requestID( 0LL ),
     _syncTick( 0LL ),
     _status( STP_SYNC_NOSOURCE ),
     _reqSendTime(),
     _reqReceiveTime(),
     _rspSendTime(),
     _rspReceiveTime(),
     _reqTimeError( STP_DEF_TIME_ERROR ),
     _rspTimeError( STP_DEF_TIME_ERROR ),
     _sendDelay( 0LL ),
     _receiveDelay( 0LL ),
     _offset( 0LL ),
     _delay( 0LL ),
     _cost( 0LL )
   {
   }

   _stpSyncRecord::_stpSyncRecord( const stpSyncRecord &record )
   : _requestID( record._requestID ),
     _syncTick( record._syncTick ),
     _status( record._status ),
     _reqSendTime( record._reqSendTime ),
     _reqReceiveTime( record._reqReceiveTime ),
     _rspSendTime( record._rspSendTime ),
     _rspReceiveTime( record._rspReceiveTime ),
     _reqTimeError( record._reqTimeError ),
     _rspTimeError( record._rspTimeError ),
     _sendDelay( record._sendDelay ),
     _receiveDelay( record._receiveDelay ),
     _offset( record._offset ),
     _delay( record._delay ),
     _cost( record._cost )
   {
   }

   _stpSyncRecord::_stpSyncRecord( const stpTimeSyncRsp *response,
                                   STP_SYNC_STATUS status )
   : _requestID( response->reply.header.requestID ),
     _status( status ),
     _reqSendTime( response->reqSendTimeSec,
                   response->reqSendTimeNanoSec ),
     _reqReceiveTime( response->reqReceiveTimeSec,
                      response->reqReceiveTimeNanoSec ),
     _rspSendTime( response->rspSendTimeSec,
                   response->rspSendTimeNanoSec ),
     _rspReceiveTime( response->rspReceiveTimeSec,
                      response->rspReceiveTimeNanoSec ),
     _reqTimeError( response->reqTimeError ),
     _rspTimeError( response->rspTimeError ),
     _sendDelay( 0LL ),
     _receiveDelay( 0LL ),
     _offset( 0LL ),
     _delay( 0LL ),
     _cost( 0LL )
   {
      _calculate() ;
      _syncTick = pmdGetDBTick() ;
   }

   _stpSyncRecord::~_stpSyncRecord()
   {
   }

   stpSyncRecord &_stpSyncRecord::operator =( const stpSyncRecord &record )
   {
      _requestID = record._requestID ;
      _syncTick = record._syncTick ;
      _status = record._status ;
      _reqSendTime = record._reqSendTime ;
      _reqReceiveTime = record._reqReceiveTime ;
      _rspSendTime = record._rspSendTime ;
      _rspReceiveTime = record._reqSendTime ;
      _reqTimeError = record._reqTimeError ;
      _rspTimeError = record._rspTimeError ;
      _sendDelay = record._sendDelay ;
      _receiveDelay = record._receiveDelay ;
      _offset = record._offset ;
      _delay = record._delay ;
      _cost = record._cost ;

      return (*this) ;
   }

   BOOLEAN _stpSyncRecord::isValid() const
   {
      // delay between source and client should less than time errors
      // NOTE: generally, time cost contains network delay, we also test the
      // cost to make sure it didn't take much time in source side
      return ( 0 <= _delay &&
               _delay < OSS_MAX( _reqTimeError, _rspTimeError ) &&
               0 <= _cost &&
               _cost < OSS_MAX( _reqTimeError, _rspTimeError ) ) ;
   }

   ossPoolString _stpSyncRecord::toString() const
   {
      StringBuilder ss ;
      ss << "request send time [" <<
            _reqSendTime.toMicroSecond() << "], " <<
            "request receive time [" <<
            _reqReceiveTime.toMicroSecond() << "], " <<
            "response send time [" <<
            _rspSendTime.toMicroSecond() << "], " <<
            "response receive time [" <<
            _rspReceiveTime.toMicroSecond() << "], " <<
            "offset [" << _offset << "], " <<
            "delay [" << _delay << "], " <<
            "send delay [" << _sendDelay << "], " <<
            "receive delay [" << _receiveDelay << "], " <<
            "total cost [" << _cost << "], " <<
            "old time error [" << _reqTimeError << "], " <<
            "new time error [" << _rspTimeError << "]" ;
      return ss.poolStr() ;
   }

   INT32 _stpSyncRecord::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         builder.append( STP_FIELD_NAME_REQUEST_ID, (INT64)_requestID ) ;
         builder.append( STP_FIELD_NAME_SYNC_STATUS,
                         stpGetSyncStatusName( _status ) ) ;
         builder.append( STP_FIELD_NAME_DELAY, _delay ) ;
         builder.append( STP_FIELD_NAME_OFFSET, _offset ) ;

         builder.append( STP_FIELD_NAME_SYNC_PASSED,
                         (INT64)( pmdGetTickSpanTime( _syncTick ) ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to format synchronize record to BSON, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _stpSyncRecord::fromBSON( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement element ;

         // get request ID
         element = object.getField( STP_FIELD_NAME_REQUEST_ID ) ;
         PD_CHECK( NumberLong == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_REQUEST_ID ) ;
         _requestID = (UINT64)( element.numberLong() ) ;

         // get synchronize status
         element = object.getField( STP_FIELD_NAME_SYNC_STATUS ) ;
         PD_CHECK( String == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not string",
                   STP_FIELD_NAME_SYNC_STATUS ) ;
         _status = stpGetSyncStatusByName( element.valuestr() ) ;

         // get delay
         element = object.getField( STP_FIELD_NAME_DELAY ) ;
         PD_CHECK( NumberLong == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_DELAY ) ;
         _delay = element.numberLong() ;

         // get offset
         element = object.getField( STP_FIELD_NAME_OFFSET ) ;
         PD_CHECK( NumberLong == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET ) ;
         _offset = element.numberLong() ;

         // get synchronize passed tick
         element = object.getField( STP_FIELD_NAME_SYNC_PASSED ) ;
         PD_CHECK( NumberLong == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_SYNC_PASSED ) ;
         _syncTick = element.numberLong() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse synchronize record from BSON, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   void _stpSyncRecord::_calculate()
   {
      // CLIENT    SOURCE
      //   |         |
      //   T1 -----> T2
      //   |         |
      //   T4 <----- T3
      //   |         |

      // send delay = T2 - T1
      _sendDelay = _reqReceiveTime.diff( _reqSendTime ) ;

      // receive delay = T4 - T3
      _receiveDelay = _rspReceiveTime.diff( _rspSendTime ) ;

      // offset = ( send delay - receive delay ) / 2
      //        = ( ( T2 - T1 ) - ( T4 - T3 ) ) / 2
      _offset = ( _sendDelay - _receiveDelay ) / 2 ;

      // network delay = send delay + receive delay
      //               = ( T2 - T1 ) + ( T4 - T3 )
      _delay = _sendDelay + _receiveDelay ;

      // time cost = T4 - T1
      _cost = _rspReceiveTime.diff( _reqSendTime ) ;
   }

   /*
      _stpSyncStats implement
    */
   _stpSyncStats::_stpSyncStats()
   : _syncCount( 0LL ),
     _validCount( 0LL ),
     _maxDelay( 0LL ),
     _minDelay( 0LL ),
     _initOffset( 0LL ),
     _maxPosOffset( 0LL ),
     _maxNegOffset( 0LL ),
     _minPosOffset( 0LL ),
     _minNegOffset( 0LL ),
     _posOffsetCount( 0LL ),
     _negOffsetCount( 0LL ),
     _lastDelay( 0LL ),
     _lastOffset( 0LL ),
     _updateTick( 0LL )
   {
   }

   _stpSyncStats::_stpSyncStats( const stpSyncStats &stats )
   : _syncCount( stats._syncCount ),
     _validCount( stats._validCount ),
     _maxDelay( stats._maxDelay ),
     _minDelay( stats._minDelay ),
     _initOffset( stats._initOffset ),
     _maxPosOffset( stats._maxPosOffset ),
     _maxNegOffset( stats._maxNegOffset ),
     _minPosOffset( stats._minPosOffset ),
     _minNegOffset( stats._minNegOffset ),
     _posOffsetCount( stats._posOffsetCount ),
     _negOffsetCount( stats._negOffsetCount ),
     _lastDelay( stats._lastDelay ),
     _lastOffset( stats._lastOffset ),
     _updateTick( stats._updateTick ),
     _histList( stats._histList )
   {
   }

   _stpSyncStats::~_stpSyncStats()
   {
   }

   stpSyncStats &_stpSyncStats::operator =( const stpSyncStats &stats )
   {
      _syncCount = stats._syncCount ;
      _validCount = stats._validCount ;
      _maxDelay = stats._maxDelay ;
      _minDelay = stats._minDelay ;
      _maxPosOffset = stats._maxPosOffset ;
      _maxNegOffset = stats._maxNegOffset ;
      _minPosOffset = stats._minPosOffset ;
      _minNegOffset = stats._minNegOffset ;
      _posOffsetCount = stats._posOffsetCount ;
      _negOffsetCount = stats._negOffsetCount ;
      _lastDelay = stats._lastDelay ;
      _lastOffset = stats._lastOffset ;
      _updateTick = stats._updateTick ;
      _histList = stats._histList ;

      return ( *this ) ;
   }

   void _stpSyncStats::reset()
   {
      _syncCount = 0LL ;
      _validCount = 0LL ;
      _maxDelay = 0LL ;
      _minDelay = 0LL ;
      _initOffset = 0LL ;
      _maxPosOffset = 0LL ;
      _maxNegOffset = 0LL ;
      _minPosOffset = 0LL ;
      _minNegOffset = 0LL ;
      _posOffsetCount = 0LL ;
      _negOffsetCount = 0LL ;
      _lastDelay = 0LL ;
      _lastOffset = 0LL ;
      _updateTick = 0LL ;

      _histList.clear() ;
   }

   void _stpSyncStats::incSyncCount()
   {
      ++ _syncCount ;
   }

   void _stpSyncStats::updateStats( const stpSyncRecord &record,
                                    BOOLEAN isValid,
                                    UINT32 maxSyncHist )
   {
      UINT64 delay = (UINT64)( OSS_MAX( 0LL, record.getDelay() ) ) ;
      INT64 offset = record.getOffset() ;

      // update delays
      _maxDelay = OSS_MAX( delay, _maxDelay ) ;
      _minDelay = ( _minDelay == 0 ) ?
                  delay :
                  OSS_MIN( delay, _minDelay ) ;
      _lastDelay = delay ;

      // update offsets
      if ( isValid )
      {
         // record is valid
         ++ _validCount ;
         if ( 0LL == _initOffset )
         {
            // update initial offset
            _initOffset = offset ;
         }
         else if ( offset >= 0LL )
         {
            // update positive offsets
            _maxPosOffset = OSS_MAX( offset, _maxPosOffset ) ;
            _minPosOffset = ( 0 == _minPosOffset ) ?
                            offset :
                            OSS_MIN( offset, _minPosOffset ) ;
            ++ _posOffsetCount ;
         }
         else
         {
            // update negative offsets
            _maxNegOffset = OSS_MIN( offset, _maxNegOffset ) ;
            _minNegOffset = ( 0 == _minNegOffset ) ?
                            offset :
                            OSS_MAX( offset, _minNegOffset ) ;
            ++ _negOffsetCount ;
         }

         _lastOffset = offset ;
      }

      _addHist( record, maxSyncHist ) ;
      _updateTick = record.getSyncTick() ;
   }

   void _stpSyncStats::updateStats( const stpSyncStats &stats )
   {
      if ( 0LL == _initOffset )
      {
         // update initial offset
         _initOffset = stats._initOffset ;
      }
      if ( stats._syncCount > 0LL )
      {
         // update synchronize statistics
         _syncCount += stats._syncCount ;
         _maxDelay = OSS_MAX( stats._maxDelay, _maxDelay ) ;
         _minDelay = ( 0 == _minDelay ) ?
                     stats._minDelay :
                     OSS_MIN( stats._minDelay, _minDelay ) ;
         _maxPosOffset = OSS_MAX( stats._maxPosOffset, _maxPosOffset ) ;
         _minPosOffset = ( 0 == _minPosOffset ) ?
                         stats._minPosOffset :
                         OSS_MIN( stats._minPosOffset, _minPosOffset ) ;
         _maxNegOffset = OSS_MIN( stats._maxNegOffset, _maxNegOffset ) ;
         _minNegOffset = ( 0 == _minNegOffset ) ?
                         stats._minNegOffset :
                         OSS_MAX( stats._minNegOffset, _minNegOffset ) ;
         _posOffsetCount += stats._posOffsetCount ;
         _negOffsetCount += stats._negOffsetCount ;
      }
      if ( stats._validCount > 0LL )
      {
         // update valid synchronize statistics
         _validCount += stats._validCount ;
      }
      _lastDelay = stats._lastDelay ;
      _lastOffset = stats._lastOffset ;

      if ( _updateTick < stats._updateTick )
      {
         _updateTick = stats._updateTick ;
      }

      _histList.clear() ;
   }

   INT32 _stpSyncStats::toBSON( BSONObjBuilder &builder,
                                BOOLEAN isCurrent ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         // build fields into BSON format
         builder.append( STP_FIELD_NAME_SYNC_COUNT, (INT64)_syncCount ) ;
         builder.append( STP_FIELD_NAME_VALID_COUNT, (INT64)_validCount ) ;
         builder.append( STP_FIELD_NAME_MIN_DELAY, (INT64)_minDelay ) ;
         builder.append( STP_FIELD_NAME_MAX_DELAY, (INT64)_maxDelay ) ;
         builder.append( STP_FIELD_NAME_INIT_OFFSET, _initOffset ) ;

         BSONObjBuilder negBuilder(
                     builder.subobjStart( STP_FIELD_NAME_NEG_OFFSET ) ) ;

         negBuilder.append( STP_FIELD_NAME_OFFSET_COUNT,
                            (INT64)_negOffsetCount ) ;
         negBuilder.append( STP_FIELD_NAME_OFFSET_MIN, _minNegOffset ) ;
         negBuilder.append( STP_FIELD_NAME_OFFSET_MAX, _maxNegOffset ) ;

         negBuilder.doneFast() ;

         BSONObjBuilder posBuilder(
                     builder.subobjStart( STP_FIELD_NAME_POS_OFFSET ) ) ;

         posBuilder.append( STP_FIELD_NAME_OFFSET_COUNT,
                                     (INT64)_posOffsetCount ) ;
         posBuilder.append( STP_FIELD_NAME_OFFSET_MIN, _minPosOffset ) ;
         posBuilder.append( STP_FIELD_NAME_OFFSET_MAX, _maxPosOffset ) ;

         posBuilder.doneFast() ;

         builder.append( STP_FIELD_NAME_LAST_DELAY, (INT64)_lastDelay ) ;
         builder.append( STP_FIELD_NAME_LAST_OFFSET, getLastOffset() ) ;
         builder.append( STP_FIELD_NAME_UPDATE_PASSED,
                         (INT64)( pmdGetTickSpanTime( _updateTick ) ) ) ;

         if ( isCurrent )
         {
            BSONArrayBuilder histArrBuilder(
                        builder.subarrayStart( STP_FIELD_NAME_SYNC_HISTORY ) ) ;

            for ( STP_SYNC_REC_LIST::const_iterator iter = _histList.begin() ;
                  _histList.end() != iter ;
                  ++ iter )
            {
               BSONObjBuilder histBuilder( histArrBuilder.subobjStart() ) ;

               rc = iter->toBSON( histBuilder ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to format history synchronize "
                            "record to BSON, rc: %d", rc ) ;

               histBuilder.doneFast() ;
            }

            histArrBuilder.doneFast() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for synchronize statistics, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _stpSyncStats::fromBSON( const bson::BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement element, subElement ;
         BSONObj subObject ;

         // get synchronize count
         element = object.getField( STP_FIELD_NAME_SYNC_COUNT ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_SYNC_COUNT ) ;
         _syncCount = (UINT64)( element.numberLong() ) ;

         // get validated synchronize count
         element = object.getField( STP_FIELD_NAME_VALID_COUNT ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_VALID_COUNT ) ;
         _validCount = (UINT64)( element.numberLong() ) ;

         // get minimum delay
         element = object.getField( STP_FIELD_NAME_MIN_DELAY ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_MIN_DELAY ) ;
         _minDelay = (UINT64)( element.numberLong() ) ;

         // get maximum delay
         element = object.getField( STP_FIELD_NAME_MAX_DELAY ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_MAX_DELAY ) ;
         _maxDelay = (UINT64)( element.numberLong() ) ;

         // get initial offset
         element = object.getField( STP_FIELD_NAME_INIT_OFFSET ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_INIT_OFFSET ) ;
         _initOffset = element.numberLong() ;

         element = object.getField( STP_FIELD_NAME_NEG_OFFSET ) ;
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not array",
                   STP_FIELD_NAME_NEG_OFFSET ) ;

         subObject = element.embeddedObject() ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_COUNT ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_COUNT ) ;
         _negOffsetCount = (UINT64)( subElement.numberLong() ) ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_MIN ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_MIN ) ;
         _minNegOffset = subElement.numberLong() ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_MAX ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_MAX ) ;
         _maxNegOffset = subElement.numberLong() ;

         element = object.getField( STP_FIELD_NAME_POS_OFFSET ) ;
         PD_CHECK( Object == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not array",
                   STP_FIELD_NAME_POS_OFFSET ) ;

         subObject = element.embeddedObject() ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_COUNT ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_COUNT ) ;
         _posOffsetCount = (UINT64)( subElement.numberLong() ) ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_MIN ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_MIN ) ;
         _minPosOffset = subElement.numberLong() ;

         subElement = subObject.getField( STP_FIELD_NAME_OFFSET_MAX ) ;
         PD_CHECK( NumberLong == subElement.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_OFFSET_MAX ) ;
         _maxPosOffset = subElement.numberLong() ;

         // get last delay
         element = object.getField( STP_FIELD_NAME_LAST_DELAY ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_LAST_DELAY ) ;
         _lastDelay = (UINT64)( element.numberLong() ) ;

         // get last offset
         element = object.getField( STP_FIELD_NAME_LAST_OFFSET ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_LAST_OFFSET ) ;
         _lastOffset = element.numberLong() ;

         // get last update tick
         element = object.getField( STP_FIELD_NAME_UPDATE_PASSED ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not number long",
                   STP_FIELD_NAME_UPDATE_PASSED ) ;
         _updateTick = (UINT64)( element.numberLong() ) ;

         element = object.getField( STP_FIELD_NAME_SYNC_HISTORY ) ;
         if ( Array == element.type() )
         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               stpSyncRecord record ;
               subElement = iter.next() ;
               PD_CHECK( Object == subElement.type(), SDB_SYS, error, PDERROR,
                         "Failed to get element from field [%s], "
                         "it is not object", STP_FIELD_NAME_SYNC_HISTORY ) ;

               rc = record.fromBSON( subElement.embeddedObject() ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse synchronize "
                            "record from BSON, rc: %d", rc ) ;

               _histList.push_back( record ) ;
            }
         }
         else
         {
            PD_CHECK( EOO == element.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not array or empty",
                      STP_FIELD_NAME_SYNC_HISTORY ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse BSON to synchronize statistics, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   void _stpSyncStats::_addHist( const stpSyncRecord &record,
                                 UINT32 maxSyncHist )
   {

      if ( maxSyncHist > 0 )
      {
         try
         {
            // release old records to keep size
            while ( _histList.size() > maxSyncHist - 1 )
            {
               _histList.pop_front() ;
            }
            // add new records
            _histList.push_back( record ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to add synchronize history, "
                    "occur exception: %s", e.what() ) ;
         }
      }
   }

}
