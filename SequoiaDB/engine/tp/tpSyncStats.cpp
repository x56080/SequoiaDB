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

   Source File Name = tpSyncStats.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpSyncStats.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _tpSyncRecord implement
    */
   _tpSyncRecord::_tpSyncRecord()
   : _requestID( 0 ),
     _reqSendTime(),
     _reqReceiveTime(),
     _rspSendTime(),
     _rspReceiveTime(),
     _reqTimeError( TP_DEF_TIME_ERROR ),
     _rspTimeError( TP_DEF_TIME_ERROR ),
     _sendSkew( 0L ),
     _receiveSkew( 0L ),
     _offset( 0L ),
     _delay( 0L )
   {
   }

   _tpSyncRecord::_tpSyncRecord( const tpSyncRecord &record )
   : _requestID( record._requestID ),
     _reqSendTime( record._reqSendTime ),
     _reqReceiveTime( record._reqReceiveTime ),
     _rspSendTime( record._rspSendTime ),
     _rspReceiveTime( record._rspReceiveTime ),
     _reqTimeError( record._reqTimeError ),
     _rspTimeError( record._rspTimeError ),
     _sendSkew( record._sendSkew ),
     _receiveSkew( record._receiveSkew ),
     _offset( record._offset ),
     _delay( record._delay )
   {
   }

   _tpSyncRecord::_tpSyncRecord( const MsgTpTimeSyncRsp *response )
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
     _sendSkew( 0L ),
     _receiveSkew( 0L ),
     _offset( 0L ),
     _delay( 0L )
   {
      _calculate() ;
   }

   _tpSyncRecord::~_tpSyncRecord()
   {
   }

   tpSyncRecord &_tpSyncRecord::operator =( const tpSyncRecord &record )
   {
      _requestID = record._requestID ;
      _reqSendTime = record._reqSendTime ;
      _reqReceiveTime = record._reqReceiveTime ;
      _rspSendTime = record._rspSendTime ;
      _rspReceiveTime = record._reqSendTime ;
      _reqTimeError = record._reqTimeError ;
      _rspTimeError = record._rspTimeError ;
      _sendSkew = record._sendSkew ;
      _receiveSkew = record._receiveSkew ;
      _offset = record._offset ;
      _delay = record._delay ;

      return (*this) ;
   }

   BOOLEAN _tpSyncRecord::isValid() const
   {
      return ( 0 <= _delay &&
               _delay < OSS_MAX( _reqTimeError, _rspTimeError ) ) ;
   }

   ossPoolString _tpSyncRecord::toString() const
   {
      StringBuilder ss ;
      ss << "request send time [" << _reqSendTime.toMicroSecond() << "], " <<
            "request receive time [" << _reqReceiveTime.toMicroSecond() << "], " <<
            "response send time [" << _rspSendTime.toMicroSecond() << "], " <<
            "response receive time [" << _rspReceiveTime.toMicroSecond() << "], " <<
            "offset [" << _offset << "], " <<
            "delay [" << _delay << "], " <<
            "time error [" << _rspTimeError << "]" ;
      return ss.poolStr() ;
   }

   void _tpSyncRecord::_calculate()
   {
      _sendSkew = _reqReceiveTime.diff( _reqSendTime ) ;
      _receiveSkew = _rspReceiveTime.diff( _rspSendTime ) ;
      _offset = ( _sendSkew - _receiveSkew ) / 2 ;
      _delay = _sendSkew + _receiveSkew ;
   }

   /*
      _tpSyncStats implement
    */
   _tpSyncStats::_tpSyncStats()
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

   _tpSyncStats::_tpSyncStats( const tpSyncStats &stats )
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

   _tpSyncStats::~_tpSyncStats()
   {
   }

   tpSyncStats &_tpSyncStats::operator =( const tpSyncStats &stats )
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

   void _tpSyncStats::reset()
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

   void _tpSyncStats::updateSync()
   {
      ++ _syncCount ;
   }

   void _tpSyncStats::updateStats( const tpSyncRecord &record,
                                   BOOLEAN isValid )
   {
      UINT64 delay = (UINT64)( OSS_MAX( 0LL, record.getDelay() ) ) ;
      INT64 offset = record.getOffset() ;
      _maxDelay = OSS_MAX( delay, _maxDelay ) ;
      _minDelay = OSS_MIN( delay, _minDelay ) ;
      if ( 0LL == _initOffset )
      {
         _initOffset = offset ;
      }
      if ( offset >= 0LL )
      {
         _maxPosOffset = OSS_MAX( offset, _maxPosOffset ) ;
         _minPosOffset = OSS_MIN( offset, _minPosOffset ) ;
      }
      else
      {
         _maxNegOffset = OSS_MIN( offset, _maxNegOffset ) ;
         _minNegOffset = OSS_MAX( offset, _minNegOffset ) ;
      }
      if ( isValid )
      {
         ++ _validCount ;
         _maxValidDelay = OSS_MAX( delay, _maxValidDelay ) ;
         if ( offset >= 0LL )
         {
            _maxValidPosOffset = OSS_MAX( offset, _maxValidPosOffset ) ;
         }
         else
         {
            _maxValidNegOffset = OSS_MIN( offset, _maxValidNegOffset ) ;
         }
      }
   }

   void _tpSyncStats::updateStats( const tpSyncStats &stats )
   {
      if ( 0LL == _initOffset )
      {
         _initOffset = stats._initOffset ;
      }
      if ( stats._syncCount > 0LL )
      {
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

         _validCount += stats._validCount ;
         _maxValidDelay = OSS_MAX( stats._maxValidDelay, _maxValidDelay ) ;
         _maxValidPosOffset = OSS_MAX( stats._maxValidPosOffset,
                                       _maxValidPosOffset ) ;
         _maxValidNegOffset = OSS_MIN( stats._maxValidNegOffset,
                                       _maxValidNegOffset ) ;
      }
   }

   INT32 _tpSyncStats::toBSON( bson::BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         builder.append( TP_FIELD_NAME_SYNC_COUNT, (INT64)getSyncCount() ) ;
         builder.append( TP_FIELD_NAME_VALID_COUNT, (INT64)getValidCount() ) ;
         builder.append( TP_FIELD_NAME_MAX_DELAY, (INT64)getMaxDelay() ) ;
         builder.append( TP_FIELD_NAME_MIN_DELAY, (INT64)getMinDelay() ) ;
         builder.append( TP_FIELD_NAME_INIT_OFFSET, getInitOffset() ) ;
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
