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
   : _requestID( 0 ),
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

   _stpSyncRecord::_stpSyncRecord( const stpTimeSyncRsp *response )
   : _requestID( response->reply.header.requestID ),
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
   }

   _stpSyncRecord::~_stpSyncRecord()
   {
   }

   stpSyncRecord &_stpSyncRecord::operator =( const stpSyncRecord &record )
   {
      _requestID = record._requestID ;
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
     _maxValidDelay( 0LL ),
     _minDelay( 0LL ),
     _initOffset( 0LL ),
     _maxPosOffset( 0LL ),
     _maxNegOffset( 0LL ),
     _maxValidPosOffset( 0LL ),
     _maxValidNegOffset( 0LL ),
     _minPosOffset( 0LL ),
     _minNegOffset( 0LL )
   {
   }

   _stpSyncStats::_stpSyncStats( const stpSyncStats &stats )
   : _syncCount( stats._syncCount ),
     _validCount( stats._validCount ),
     _maxDelay( stats._maxDelay ),
     _maxValidDelay( stats._maxValidDelay ),
     _minDelay( stats._minDelay ),
     _initOffset( stats._initOffset ),
     _maxPosOffset( stats._maxPosOffset ),
     _maxNegOffset( stats._maxNegOffset ),
     _maxValidPosOffset( stats._maxValidPosOffset ),
     _maxValidNegOffset( stats._maxValidNegOffset ),
     _minPosOffset( stats._minPosOffset ),
     _minNegOffset( stats._minPosOffset )
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
      _maxValidDelay = stats._maxValidDelay ;
      _minDelay = stats._minDelay ;
      _maxPosOffset = stats._maxPosOffset ;
      _maxNegOffset = stats._maxNegOffset ;
      _maxValidPosOffset = stats._maxValidPosOffset ;
      _maxValidNegOffset = stats._maxValidNegOffset ;
      _minPosOffset = stats._minPosOffset ;
      _minNegOffset = stats._minNegOffset ;

      return ( *this ) ;
   }

   void _stpSyncStats::reset()
   {
      _syncCount = 0LL ;
      _validCount = 0LL ;
      _maxDelay = 0LL ;
      _maxValidDelay = 0LL ;
      _minDelay = 0LL ;
      _initOffset = 0LL ;
      _maxPosOffset = 0LL ;
      _maxNegOffset = 0LL ;
      _maxValidPosOffset = 0LL ;
      _maxValidNegOffset = 0LL ;
      _minPosOffset = 0LL ;
      _minNegOffset = 0LL ;
   }

   void _stpSyncStats::incSyncCount()
   {
      ++ _syncCount ;
   }

   void _stpSyncStats::updateStats( const stpSyncRecord &record,
                                    BOOLEAN isValid )
   {
      UINT64 delay = (UINT64)( OSS_MAX( 0LL, record.getDelay() ) ) ;
      INT64 offset = record.getOffset() ;
      _maxDelay = OSS_MAX( delay, _maxDelay ) ;
      _minDelay = OSS_MIN( delay, _minDelay ) ;
      if ( offset >= 0LL )
      {
         // update positive offsets
         _maxPosOffset = OSS_MAX( offset, _maxPosOffset ) ;
         _minPosOffset = OSS_MIN( offset, _minPosOffset ) ;
      }
      else
      {
         // update negative offsets
         _maxNegOffset = OSS_MIN( offset, _maxNegOffset ) ;
         _minNegOffset = OSS_MAX( offset, _minNegOffset ) ;
      }
      if ( isValid )
      {
         // record is valid
         ++ _validCount ;
         if ( 0LL == _initOffset )
         {
            // update initial offset
            _initOffset = offset ;
         }
         // update delay
         _maxValidDelay = OSS_MAX( delay, _maxValidDelay ) ;
         if ( offset >= 0LL )
         {
            // update positive offset
            _maxValidPosOffset = OSS_MAX( offset, _maxValidPosOffset ) ;
         }
         else
         {
            // update negative offset
            _maxValidNegOffset = OSS_MIN( offset, _maxValidNegOffset ) ;
         }
      }
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
         _minDelay = OSS_MIN( stats._minDelay, _minDelay ) ;
         _maxPosOffset = OSS_MAX( stats._maxPosOffset, _maxPosOffset ) ;
         _minPosOffset = OSS_MIN( stats._minPosOffset, _minPosOffset ) ;
         _maxNegOffset = OSS_MIN( stats._maxNegOffset, _maxNegOffset ) ;
         _minNegOffset = OSS_MAX( stats._minNegOffset, _minNegOffset ) ;
      }
      if ( stats._validCount > 0LL )
      {
         // update valid synchronize statistics
         _validCount += stats._validCount ;
         _maxValidDelay = OSS_MAX( stats._maxValidDelay, _maxValidDelay ) ;
         _maxValidPosOffset = OSS_MAX( stats._maxValidPosOffset,
                                       _maxValidPosOffset ) ;
         _maxValidNegOffset = OSS_MIN( stats._maxValidNegOffset,
                                       _maxValidNegOffset ) ;
      }
   }

   INT32 _stpSyncStats::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         // build fields into BSON format
         builder.append( STP_FIELD_NAME_SYNC_COUNT, (INT64)getSyncCount() ) ;
         builder.append( STP_FIELD_NAME_VALID_COUNT, (INT64)getValidCount() ) ;
         builder.append( STP_FIELD_NAME_MAX_DELAY, (INT64)getMaxDelay() ) ;
         builder.append( STP_FIELD_NAME_MIN_DELAY, (INT64)getMinDelay() ) ;
         builder.append( STP_FIELD_NAME_INIT_OFFSET, getInitOffset() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build BSON for synchronize statistics, "
                 "occurred unexpected error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
