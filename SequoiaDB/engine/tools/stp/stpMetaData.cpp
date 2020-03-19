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

using namespace bson ;
using namespace std ;

namespace engine
{

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
         builder.append( STP_FIELD_NAME_VERSION, (INT32)_version ) ;
         // append synchronize interval
         builder.append( STP_FIELD_NAME_SYNC_INTERVAL, (INT32)_syncInterval ) ;

         // append synchronize hardware time
         rc = _syncHWTime.toBSON( STP_FIELD_NAME_SYNC_HW_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_SYNC_HW_TIME, rc ) ;

         // append based hardware time
         rc = _baseHWTime.toBSON( STP_FIELD_NAME_BASE_HW_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_BASE_HW_TIME, rc ) ;

         // append based real time
         rc = _baseRealTime.toBSON( STP_FIELD_NAME_BASE_REAL_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      STP_FIELD_NAME_BASE_REAL_TIME, rc ) ;

         // append offset
         builder.append( STP_FIELD_NAME_OFFSET, _offset ) ;
         // append slew rate
         builder.append( STP_FIELD_NAME_SLEW_RATE, (INT64)_slewRate ) ;
         // append time error
         builder.append( STP_FIELD_NAME_TIME_ERROR, (INT32)_timeError ) ;

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

      // calculate slew rate
      // slew rate = old slew rate * source interval / local interval
      _slewRate = (UINT64)( (double)( _slewRate ) *
                            (double)( sourceInterval ) /
                            (double)( localInterval ) ) ;

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
