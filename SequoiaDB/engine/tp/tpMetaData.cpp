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

   Source File Name = tpMetaData.cpp

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

#include "tpMetaData.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "tpToolCommon.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _tpMetaDataContent define
    */
   #define TP_META_EYECATCHER_HEAD ( 0xED3218AC )
   #define TP_META_EYECATCHER_TAIL ( 0xAC1832ED )

   typedef struct _tpMetaDataContent
   {
      _tpMetaDataContent()
      : _head( TP_META_EYECATCHER_HEAD ),
        _data(),
        _tail( TP_META_EYECATCHER_TAIL )
      {
      }

      UINT32      _head ;
      tpMetaData _data ;
      UINT32      _tail ;
   } tpMetaDataContent ;

   /*
      _tpMetaData implement
    */
   _tpMetaData::_tpMetaData()
   : _version( TP_VERSION ),
     _syncInterval( TP_DEF_SYNC_INTERVAL ),
     _syncTime(),
     _baseHWTime(),
     _baseRealTime(),
     _offset( 0LL ),
     _slewRate( TP_DEF_SLEWRATE ),
     _timeError( TP_DEF_TIME_ERROR )
   {
   }

   _tpMetaData::~_tpMetaData()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_INITIALIZE, "_tpMetaData::initialize" )
   void _tpMetaData::initialize( UINT64 timeUS, UINT32 syncInterval )
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_INITIALIZE ) ;

      _version = TP_VERSION ;
      _syncInterval = syncInterval ;
      _syncTime.reset() ;

      reset( timeUS ) ;

      PD_TRACE_EXIT( SDB__TPMETADATA_INITIALIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_RESET, "_tpMetaData::reset" )
   void _tpMetaData::reset( UINT64 timeUS )
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_RESET ) ;

      // set as default values
      _baseHWTime.sampleMonotonic() ;
      _baseRealTime.fromMicroSecond( timeUS ) ;
      _offset = 0LL ;
      _slewRate = TP_DEF_SLEWRATE ;
      _timeError = TP_DEF_TIME_ERROR ;

      PD_TRACE_EXIT( SDB__TPMETADATA_RESET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_GETLOGICALTIMENS, "_tpMetaData::getLogicalTimeNS" )
   INT32 _tpMetaData::getLogicalTimeNS( tpLogicalTimeNS &time,
                                        BOOLEAN checkSync ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETADATA_GETLOGICALTIMENS ) ;

      if ( checkSync )
      {
         tpHPTime syncTime = _syncTime ;
         UINT64 syncLimit = TP_SEC_TO_NANOSEC( _syncInterval +
                                               TP_MIN_SYNC_INTERVAL ) ;
         tpHPTime curHWTime( TP_TIME_SAMPLE_MONOTONIC ) ;
         PD_CHECK( curHWTime >= syncTime,
                   SDB_TP_SYNC_FAST, error, PDWARNING,
                   "Failed to get TP logical time, synchronized much faster "
                   "than source, last synchronized hardware time [%llu], "
                   "current hardware time [%llu]", syncTime, curHWTime ) ;
         PD_CHECK( curHWTime <= syncTime + syncLimit,
                   SDB_TP_SYNC_FAILED, error, PDERROR,
                   "Failed to get TP logical time, synchronize failed, "
                   "last synchronized hardware time [%llu], "
                   "current hardware time [%llu], synchronize limit [%llu]",
                   syncTime, curHWTime, syncLimit ) ;

         time.setTime( _getLTValue( curHWTime ) ) ;
         time.setTimeError( _timeError ) ;

         // it is not thread-safe, even after checking
         PD_CHECK( syncTime == _syncTime, SDB_TP_SYNC_BUSY, error, PDERROR,
                   "Failed to get TP logical time, "
                   "it is busy for synchronizing" ) ;
      }
      else
      {
         time.setTime( _getLTValue() ) ;
         time.setTimeError( _timeError ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPMETADATA_GETLOGICALTIMENS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_GETLOGICALTIMEUS, "_tpMetaData::getLogicalTimeUS" )
   INT32 _tpMetaData::getLogicalTimeUS( tpLogicalTimeUS &time,
                                        BOOLEAN checkSync ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETADATA_GETLOGICALTIMEUS ) ;

      tpLogicalTimeNS timeNS ;

      rc = getLogicalTimeNS( timeNS, checkSync ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logical time, rc: %d", rc ) ;

      time = timeNS ;

   done:
      PD_TRACE_EXITRC( SDB__TPMETADATA_GETLOGICALTIMEUS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_RESETSLEWRATE, "_tpMetaData::resetSlewRate" )
   void _tpMetaData::resetSlewRate()
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_RESETSLEWRATE ) ;

      setSlewRate( TP_DEF_SLEWRATE ) ;

      PD_TRACE_EXIT( SDB__TPMETADATA_RESETSLEWRATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_TOBSON, "_tpMetaData::toBSON" )
   INT32 _tpMetaData::toBSON( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPMETADATA_TOBSON ) ;

      try
      {
         BSONObjBuilder metaBuilder(
                           builder.subobjStart( TP_FIELD_NAME_META_DATA ) ) ;

         builder.append( TP_FIELD_NAME_VERSION, (INT32)_version ) ;
         builder.append( TP_FIELD_NAME_SYNC_INTERVAL, (INT32)_syncInterval ) ;

         rc = _syncTime.toBSON( TP_FIELD_NAME_SYNC_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      TP_FIELD_NAME_SYNC_TIME, rc ) ;

         rc = _baseHWTime.toBSON( TP_FIELD_NAME_BASE_HW_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      TP_FIELD_NAME_BASE_HW_TIME, rc ) ;

         rc = _baseRealTime.toBSON( TP_FIELD_NAME_BASE_REAL_TIME, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for [%s], rc: %d",
                      TP_FIELD_NAME_BASE_REAL_TIME, rc ) ;

         builder.append( TP_FIELD_NAME_OFFSET, _offset ) ;
         builder.append( TP_FIELD_NAME_SLEW_RATE, (INT64)_slewRate ) ;
         builder.append( TP_FIELD_NAME_TIME_ERROR, (INT32)_timeError ) ;

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
      PD_TRACE_EXITRC( SDB__TPMETADATA_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   ossPoolString _tpMetaData::toString() const
   {
      StringBuilder ss ;
      ss << "base HW time [" << _baseHWTime.toMicroSecond() << "], " <<
            "base real time [" << _baseRealTime.toMicroSecond() << "], " <<
            "offset [" << _offset << "], " <<
            "slew rate [" << _slewRate << "], " <<
            "time error [" << _timeError << "]" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_ADJUSTLOGICALTIME, "_tpMetaData::adjustLogicalTime" )
   void _tpMetaData::adjustLogicalTime( INT64 syncOffset )
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_ADJUSTLOGICALTIME ) ;

      if ( syncOffset >= 0LL )
      {
         _offset += syncOffset ;
         _syncTime.sampleMonotonic() ;
      }
      else
      {
         _offset += syncOffset ;
         _syncTime.sampleMonotonic() ;
         _syncTime.adjust( -1 * syncOffset ) ;
      }

      PD_TRACE_EXIT( SDB__TPMETADATA_ADJUSTLOGICALTIME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_ADJUSTLOGICALTIME_TE, "_tpMetaData::adjustLogicalTime" )
   void _tpMetaData::adjustLogicalTime( INT64 syncOffset,
                                        UINT32 timeError )
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_ADJUSTLOGICALTIME_TE ) ;

      adjustLogicalTime( syncOffset ) ;
      _timeError = timeError ;

      PD_TRACE_EXIT( SDB__TPMETADATA_ADJUSTLOGICALTIME_TE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_ADJUSTSLEWRATE, "_tpMetaData::adjustSlewRate" )
   void _tpMetaData::adjustSlewRate( UINT64 sourceInterval,
                                     UINT64 localInterval )
   {
      PD_TRACE_ENTRY( SDB__TPMETADATA_ADJUSTSLEWRATE ) ;

      _slewRate = (UINT64)( (double)( _slewRate ) *
                            (double)( sourceInterval ) /
                            (double)( localInterval ) ) ;

      PD_TRACE_EXIT( SDB__TPMETADATA_ADJUSTSLEWRATE ) ;
   }

   UINT32 _tpMetaData::getBufferSize()
   {
      return sizeof( tpMetaDataContent ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_GETBUFFER, "_tpMetaData::getBuffer" )
   tpMetaData *_tpMetaData::getBuffer( CHAR *buffer )
   {
      tpMetaData *data = NULL ;

      PD_TRACE_ENTRY( SDB__TPMETADATA_GETBUFFER ) ;

      if ( NULL != buffer )
      {
         tpMetaDataContent *content = (tpMetaDataContent *)( buffer ) ;
         if ( TP_META_EYECATCHER_HEAD == content->_head &&
              TP_META_EYECATCHER_TAIL == content->_tail )
         {
            data = &( content->_data ) ;
         }
      }

      PD_TRACE_EXIT( SDB__TPMETADATA_GETBUFFER ) ;

      return data ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPMETADATA_NEWBUFFER, "_tpMetaData::newBuffer" )
   tpMetaData *_tpMetaData::newBuffer( CHAR *buffer )
   {
      tpMetaData *data = NULL ;

      PD_TRACE_ENTRY( SDB__TPMETADATA_NEWBUFFER ) ;

      if ( NULL != buffer )
      {
         tpMetaDataContent *content = (tpMetaDataContent *)( buffer ) ;
         content->_head = TP_META_EYECATCHER_HEAD ;
         data = new( &( content->_data ) ) tpMetaData() ;
         content->_tail = TP_META_EYECATCHER_TAIL ;
      }

      PD_TRACE_EXIT( SDB__TPMETADATA_NEWBUFFER ) ;

      return data ;
   }

}
