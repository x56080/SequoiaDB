/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = stpLogicalTime.hpp

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

#ifndef STP_LOGICAL_TIME_HPP_
#define STP_LOGICAL_TIME_HPP_

#include "stpCommon.hpp"
#include "stpToolCommon.hpp"
#include "utilPooledObject.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"

namespace engine
{

   /*
      STP_GET_TIME_MODE
    */
   // Mode to sample ( get ) time from system
   enum STP_SAMPLE_TIME_MODE
   {
      // not to get time ( get 0 )
      STP_SAMPLE_TIME_NONE,
      // get real time
      STP_SAMPLE_TIME_REAL,
      // get monotonic time ( time after machine boot )
      STP_SAMPLE_TIME_MONOTONIC
   } ;

   /*
      _stpHPTime define
    */
   class _stpHPTime ;
   typedef class _stpHPTime stpHPTime ;

   class _stpLogicalTimeBase ;
   typedef class _stpLogicalTimeBase stpLogicalTimeBase ;

   class _stpLogicalTimeNS ;
   typedef class _stpLogicalTimeNS stpLogicalTimeNS ;

   class _stpLogicalTimeUS ;
   typedef class _stpLogicalTimeUS stpLogicalTimeUS ;

   // _stpHPTime represents for high precision time in nanoseconds
   class _stpHPTime : public utilPooledObject
   {
   public:
      // constructor and destructor
      _stpHPTime()
      : _second( 0LL ),
        _nanoSecond( 0LL )
      {
      }

      // constructor to sample time
      _stpHPTime( STP_SAMPLE_TIME_MODE mode )
      : _second( 0LL ),
        _nanoSecond( 0LL )
      {
         sample( mode ) ;
      }

      _stpHPTime( UINT64 second, UINT64 nanoSecond )
      : _second( second ),
        _nanoSecond( nanoSecond )
      {
      }

      _stpHPTime( const stpHPTime &time )
      : _second( time._second ),
        _nanoSecond( time._nanoSecond )
      {
      }

      ~_stpHPTime()
      {
      }

   public:
      // operators
      OSS_INLINE stpHPTime &operator =( const stpHPTime &time )
      {
         _second = time._second ;
         _nanoSecond = time._nanoSecond ;

         return ( *this ) ;
      }

      // plus two high precision time
      friend stpHPTime operator +( const stpHPTime &lhs, const stpHPTime &rhs ) ;
      // subtract two high precision time
      friend stpHPTime operator -( const stpHPTime &lhs, const stpHPTime &rhs ) ;
      // plus high precision time with nanoseconds
      friend stpHPTime operator +( const stpHPTime &lhs, UINT64 rhs ) ;
      // subtract high precision time with nanoseconds
      friend stpHPTime operator -( const stpHPTime &lhs, UINT64 rhs ) ;

      OSS_INLINE BOOLEAN operator ==( const stpHPTime &time ) const
      {
         return ( _second == time._second &&
                  _nanoSecond == time._nanoSecond ) ;
      }

      OSS_INLINE BOOLEAN operator !=( const stpHPTime &time ) const
      {
         return !( operator ==( time ) ) ;
      }

      OSS_INLINE BOOLEAN operator <( const stpHPTime &time ) const
      {
         return ( _second < time._second ||
                  ( _second == time._second &&
                    _nanoSecond < time._nanoSecond ) ) ;
      }

      OSS_INLINE BOOLEAN operator >( const stpHPTime &time ) const
      {
         return time.operator <( *this ) ;
      }

      OSS_INLINE BOOLEAN operator <=( const stpHPTime &time ) const
      {
         return !( operator >( time ) ) ;
      }

      OSS_INLINE BOOLEAN operator >=( const stpHPTime &time ) const
      {
         return !( operator <( time ) ) ;
      }

   public:
      // get second component
      OSS_INLINE UINT64 getSecond() const
      {
         return _second ;
      }

      // get nanosecond component
      OSS_INLINE UINT64 getNanoSecond() const
      {
         return _nanoSecond ;
      }

      // check if it is 0
      OSS_INLINE BOOLEAN isZero() const
      {
         return ( 0LL == _second && 0LL == _nanoSecond ) ;
      }

      // reset to 0
      OSS_INLINE void reset()
      {
         _second = 0LL ;
         _nanoSecond = 0LL ;
      }

      // convert to nanoseconds
      // WARNING: may overflow
      OSS_INLINE UINT64 toNanoSecond() const
      {
         return STP_SEC_TO_NANOSEC( _second ) + _nanoSecond ;
      }

      // parse from nanoseconds
      OSS_INLINE void fromNanoSecond( UINT64 nanoSecond )
      {
         _second = 0LL ;
         _nanoSecond = nanoSecond ;
         _normalize() ;
      }

      // convert to microseconds
      OSS_INLINE UINT64 toMicroSecond() const
      {
         return STP_SEC_TO_MICROSEC( _second ) +
                STP_NANOSEC_TO_MICROSEC( _nanoSecond ) ;
      }

      // parse from second
      OSS_INLINE void fromSecond( UINT64 second )
      {
         _second = second ;
         _nanoSecond = 0LL ;
      }

      // parse from milliseconds
      OSS_INLINE void fromMilliSecond( UINT64 milliSecond )
      {
         _second = STP_MILLISEC_TO_SEC( milliSecond ) ;
         _nanoSecond =
               STP_MILLISEC_TO_NANOSEC( milliSecond % OSS_ONE_THOUSAND ) ;
      }

      // parse from microseconds
      OSS_INLINE void fromMicroSecond( UINT64 microSecond )
      {
         _second = microSecond / OSS_ONE_MILLION ;
         _nanoSecond = microSecond % OSS_ONE_MILLION * OSS_ONE_THOUSAND ;
      }

      // format time to BSON object
      OSS_INLINE INT32 toBSON( bson::BSONObjBuilder &builder ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            // append second component
            builder.append( STP_FIELD_NAME_SECOND, (INT64)_second ) ;
            // append nanosecond component
            builder.append( STP_FIELD_NAME_NANO_SECOND, (INT64)_nanoSecond ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // format time to BSON with field name
      OSS_INLINE INT32 toBSON( bson::BSONObj &object ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONObjBuilder builder ;
            rc = toBSON( builder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            object = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // parse time from BSON object
      OSS_INLINE INT32 fromBSON( const bson::BSONObj &object )
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONElement element ;

            // parse second component
            element = object.getField( STP_FIELD_NAME_SECOND ) ;
            if ( bson::NumberLong != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            _second = (UINT64)( element.numberLong() ) ;

            // parse nanosecond component
            element = object.getField( STP_FIELD_NAME_NANO_SECOND ) ;
            if ( bson::NumberLong != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            _nanoSecond = (UINT64)( element.numberLong() ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // format time to BSON timestamp
      OSS_INLINE INT32 toBSONTimestamp( bson::BSONObjBuilder &builder,
                                        const CHAR *fieldName )
      {
         INT32 rc = SDB_OK ;

         try
         {
            // BSON timestamp is for local time, need convert from UTC
            stpHPTime tempTime( *this ) ;
            UINT64 milliSec =
                  STP_SEC_TO_MILLISEC( tempTime._second ) +
                  STP_NANOSEC_TO_MILLISEC( tempTime._nanoSecond ) ;
            UINT32 microSecInc =
                  STP_NANOSEC_TO_MICROSEC(
                        tempTime._nanoSecond % OSS_ONE_MILLION ) ;
            builder.appendTimestamp( fieldName, milliSec, microSecInc ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // format time to BSON timestamp
      OSS_INLINE INT32 toBSONTimestamp( bson::BSONObjBuilder &builder )
      {
         return toBSONTimestamp( builder, STP_FIELD_NAME_TIMESTAMP ) ;
      }

      // parse time from BSON timestamp
      OSS_INLINE INT32 fromBSONTimestamp( const bson::BSONElement &element )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( bson::Timestamp != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            fromMilliSecond( (UINT64)( element.timestampTime() ) ) ;
            adjust( STP_MICROSEC_TO_NANOSEC( element.timestampInc() ) ) ;
         }
         catch ( exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // parse time from BSON element
      OSS_INLINE INT32 fromBSONElement( const bson::BSONElement &element,
                                        BOOLEAN isRealTime,
                                        BOOLEAN &isSimpleMode )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( !( ( bson::Object == element.type() ) ||
                    ( isRealTime && bson::Timestamp == element.type() ) ||
                    ( isRealTime && bson::Date == element.type() ) ||
                    ( element.isNumber() ) ) )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            if ( bson::Object == element.type() )
            {
               rc = fromBSON( element.embeddedObject() ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
            else if ( bson::Timestamp == element.type() ||
                      bson::Date == element.type() )
            {
               rc = fromBSONTimestamp( element ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               isSimpleMode = TRUE ;
            }
            else
            {
               fromMicroSecond( (UINT64)( element.numberLong() ) ) ;
               isSimpleMode = TRUE ;
            }
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // sample time from system by different mode
      OSS_INLINE void sample( STP_SAMPLE_TIME_MODE mode )
      {
         switch ( mode )
         {
            case STP_SAMPLE_TIME_REAL :
            {
               // get real time
               sampleReal() ;
               break ;
            }
            case STP_SAMPLE_TIME_MONOTONIC :
            {
               // get monotonic time ( time since machine boot )
               sampleMonotonic() ;
               break ;
            }
            default :
            {
               // set to 0
               _second = 0LL ;
               _nanoSecond = 0LL ;
               break ;
            }
         }
      }

      // sample real time from system
      OSS_INLINE void sampleReal()
      {
#if defined (_WINDOWS)
         // get current time from OSS
         UINT64 value = 0LL ;
         ossTimestamp tm ;
         ossGetCurrentTime( tm ) ;
         _second = (UINT64)( tm.time ) ;
         _nanoSecond = (UINT64)( tm.microtm * OSS_ONE_THOUSAND ) ;
#else
         // get time from clock
         struct timespec ts ;
         if ( 0 == clock_gettime( CLOCK_REALTIME, &ts ) )
         {
            _second = (UINT64)( ts.tv_sec ) ;
            _nanoSecond = (UINT64)( ts.tv_nsec ) ;
         }
#endif
      }

      // sample monotonic time from system
      OSS_INLINE void sampleMonotonic()
      {
#if defined (_WINDOWS)
         LARGE_INTEGER freq ;
         LARGE_INTEGER count ;
         // get time from performance frequency and counter
         if ( QueryPerformanceFrequency( &freq ) &&
              QueryPerformanceCounter( &count ) &&
              freq.QuadPart != 0LL )
         {
            FLOAT64 scale = (FLOAT64)OSS_ONE_BILLION / (FLOAT64)( freq.QuadPart ) ;
            UINT64 value = (UINT64)( (FLOAT64)( count.QuadPart ) * scale ) ;
            _second = value / OSS_ONE_BILLION ;
            _nanoSecond = value % OSS_ONE_BILLION ;
         }
#else
         // get time by clock
         struct timespec ts ;
         // for performance consideration, we use CLOCK_MONOTONIC
         if ( 0 == clock_gettime( CLOCK_MONOTONIC, &ts ) )
         {
            _second = (UINT64)( ts.tv_sec ) ;
            _nanoSecond = (UINT64)( ts.tv_nsec ) ;
         }
#endif
      }

      // scale time by slew rate ( given ratio to default rate )
      OSS_INLINE void scale( UINT64 slewRate )
      {
         // slew rate must larger than 0
         if ( slewRate > 0 )
         {
            // calculate real slew rate
            FLOAT64 rate = (FLOAT64)slewRate / (FLOAT64)STP_DEF_SLEWRATE ;
            if ( rate > 1.0 )
            {
               // rate is larger than 1, multiple directly
               _second = (UINT64)( (FLOAT64)_second * rate ) ;
               _nanoSecond = (UINT64)( (FLOAT64)_nanoSecond * rate ) ;
            }
            else
            {
               // rate is smaller than 1, need save fraction of seconds
               // component to nanosecond component
               FLOAT64 fraction = (FLOAT64)_second * rate ;
               // calculate second component
               _second = (UINT64)fraction ;
               // calculate nanosecond component
               fraction = fraction - (FLOAT64)_second ;
               _nanoSecond = (UINT64)( fraction * (FLOAT64)OSS_ONE_BILLION +
                                       (FLOAT64)_nanoSecond * rate ) ;
            }
            // normalize time
            _normalize() ;
         }
      }

      // adjust time by offset ( in nanosecond )
      OSS_INLINE void adjust( INT64 offset )
      {
         if ( offset > 0 )
         {
            // offset is positive, add to nanosecond
            _nanoSecond += offset ;
         }
         else if ( offset < 0 )
         {
            // offset is negative
            offset = -1 * offset ;
            UINT64 second = offset / OSS_ONE_BILLION ;
            UINT64 nanoSecond = offset % OSS_ONE_BILLION ;
            if ( _nanoSecond > nanoSecond )
            {
               // if nanosecond component could cover given nanosecond,
               // subtract directly
               _nanoSecond -= nanoSecond ;
            }
            else if ( _second > 0LL )
            {
               // nanosecond component could not cover given nanosecond
               // borrow one second to do the subtraction
               -- _second ;
               _nanoSecond = _nanoSecond + OSS_ONE_BILLION - nanoSecond ;
            }
            else
            {
               // given offset is larger than this time, set to zero
               _nanoSecond = 0LL ;
            }
            // subtract second component
            if ( _second > second )
            {
               _second -= second ;
            }
         }
         // normalize time
         _normalize() ;
      }

      // get diff by nanoseonds
      OSS_INLINE INT64 diff( const stpHPTime &time ) const
      {
         INT64 result = 0LL ;

         // compare given time, to calculate absolute value
         if ( operator >( time ) )
         {
            // given time is smaller
            stpHPTime tempTime = ( *this ) - time ;
            result = tempTime._toNanoSecond() ;
         }
         else
         {
            // given time is larger
            stpHPTime tempTime = time - ( *this ) ;
            result = -1 * (INT64)( tempTime._toNanoSecond() ) ;
         }

         return result ;
      }

      INT32 *getWatchAddress() const
      {
         // watch on the low bytes of second
#if defined ( SDB_BIG_ENDIAN )
         return (INT32 *)( &_second ) + 1 ;
#else
         return (INT32 *)( &_second ) ;
#endif
      }

      INT32 getWatchValue() const
      {
         return *getWatchAddress() ;
      }

   protected:
      // normalize ( nanosecond component should less than 1,000,000,000 )
      OSS_INLINE void _normalize()
      {
         _second += ( _nanoSecond / OSS_ONE_BILLION ) ;
         _nanoSecond %= OSS_ONE_BILLION ;
      }

      // format to nanosecond ( will be cut by lose high bits )
      OSS_INLINE UINT64 _toNanoSecond() const
      {
         return STP_SEC_TO_NANOSEC( _second ) + _nanoSecond ;
      }

   protected:
      // second component
      UINT64 _second ;
      // nanosecond component
      UINT64 _nanoSecond ;
   } ;

   // operators of high precision time
   // plus two high precision time
   OSS_INLINE stpHPTime operator +( const stpHPTime &lhs, const stpHPTime &rhs )
   {
      stpHPTime result ;

      result._second = lhs._second + rhs._second ;
      result._nanoSecond = lhs._nanoSecond + rhs._nanoSecond ;
      result._normalize() ;

      return result ;
   }

   // subtract two high precision time
   OSS_INLINE stpHPTime operator -( const stpHPTime &lhs, const stpHPTime &rhs )
   {
      stpHPTime result ;

      if ( lhs > rhs )
      {
         result._second = lhs._second - rhs._second ;
         if ( rhs._nanoSecond > lhs._nanoSecond )
         {
            // not enough nanoseconds in left, borrow 1 second to do the
            // subtraction
            -- result._second ;
            result._nanoSecond = lhs._nanoSecond + OSS_ONE_BILLION -
                                  rhs._nanoSecond ;
         }
         else
         {
            // has enough nanoseconds in left, subtract directly
            result._nanoSecond = lhs._nanoSecond - rhs._nanoSecond ;
         }
      }

      return result ;
   }

   // plus high precision time with nanoseconds
   OSS_INLINE stpHPTime operator +( const stpHPTime &lhs, UINT64 rhs )
   {
      stpHPTime result = lhs ;
      result.adjust( (INT64)rhs ) ;
      return result ;
   }

   // subtract high precision time with nanoseconds
   OSS_INLINE stpHPTime operator -( const stpHPTime &lhs, UINT64 rhs )
   {
      stpHPTime result = lhs ;
      result.adjust( (INT64)( -1 * rhs ) ) ;
      return result ;
   }

   /*
      _tpLogicalTimeBase define
    */
   // _tpLogicalTimeBase is base class for logical time in different units
   class _stpLogicalTimeBase : public utilPooledObject
   {
   public:
      // construct and destructor
      _stpLogicalTimeBase()
      : _timeError( 0 )
      {
      }

      _stpLogicalTimeBase( const stpLogicalTimeBase &time )
      : _timeError( time._timeError )
      {
      }

      _stpLogicalTimeBase( UINT32 timeError )
      : _timeError( timeError )
      {
      }

      ~_stpLogicalTimeBase()
      {
      }

      // set time error ( in nanoseconds )
      OSS_INLINE void setTimeError( UINT32 timeError )
      {
         _timeError = timeError ;
      }

      // get time error ( in nanoseconds )
      OSS_INLINE UINT32 getTimeError() const
      {
         return _timeError ;
      }

      // get time error ( in microseconds )
      OSS_INLINE UINT32 getTimeErrorUS() const
      {
         return STP_NANOSEC_TO_MICROSEC( _timeError ) ;
      }

   protected:
      // get max time error between two logical times ( in nanoseconds )
      OSS_INLINE UINT64 _getMaxTimeErrorNS(
                                       const stpLogicalTimeBase &time ) const
      {
         return OSS_MAX( _timeError, time._timeError ) ;
      }

      // get max time error between two logical times ( in microseconds )
      OSS_INLINE UINT64 _getMaxTimeErrorUS(
                                       const stpLogicalTimeBase &time ) const
      {
         return STP_NANOSEC_TO_MICROSEC(
                                    OSS_MAX( _timeError, time._timeError ) ) ;
      }

   protected:
      // time error in nanoseconds
      UINT32 _timeError ;
   } ;

   /*
      _stpLogicalTimeNS define
    */
   // _stpLogicalTimeNS is logical time measured in nanoseconds
   class _stpLogicalTimeNS : public stpLogicalTimeBase
   {
   public:
      // constructor and destructor
      _stpLogicalTimeNS()
      : stpLogicalTimeBase(),
        _time()
      {
      }

      _stpLogicalTimeNS( const stpLogicalTimeNS &time )
      : stpLogicalTimeBase( time ),
        _time( time._time )
      {
      }

      _stpLogicalTimeNS( const stpHPTime &time, UINT32 timeError )
      : stpLogicalTimeBase( timeError ),
        _time( time )
      {
      }

      ~_stpLogicalTimeNS()
      {
      }

      // convert from microseconds to nanoseconds
      _stpLogicalTimeNS( const stpLogicalTimeUS &time ) ;

   public:
      // operators
      OSS_INLINE stpLogicalTimeNS &operator =( const stpLogicalTimeNS &time )
      {
         _time = time._time ;
         _timeError = time._timeError ;
         return (*this) ;
      }

      // convert from microseconds
      OSS_INLINE stpLogicalTimeNS &operator =( const stpLogicalTimeUS &time ) ;

      // equal with time error
      OSS_INLINE BOOLEAN operator ==( const stpLogicalTimeNS &time ) const
      {
         // compare two logical time
         // max time error = max( time error of T1, time error of T2 )
         // if T1 - max time error < T2 < T1 + max time error, they are equal
         // ( within time error )
         UINT64 timeError = _getMaxTimeErrorNS( time ) ;
         return ( ( time._time - timeError ) < _time &&
                  ( time._time + timeError ) > _time ) ;
      }

      // less than with time error
      OSS_INLINE BOOLEAN operator <( const stpLogicalTimeNS &time ) const
      {
         // compare two logical time
         // max time error = max( time error of T1, time error of T2 )
         // if T1 <= T2 - max time error, then T1 < T2 ( within time error )
         return _time <= ( time._time - _getMaxTimeErrorNS( time ) ) ;
      }

      // larger than with time error
      OSS_INLINE BOOLEAN operator >( const stpLogicalTimeNS &time ) const
      {
         return ( time < ( *this ) ) ;
      }

   public:
      // set time in high precision time
      OSS_INLINE void setTime( const stpHPTime &time )
      {
         _time = time ;
      }

      // get time in high precision time
      OSS_INLINE const stpHPTime &getTime() const
      {
         return _time ;
      }

      // reset time
      OSS_INLINE void reset()
      {
         _time.reset() ;
         _timeError = 0 ;
      }

   public:
      // format logical time into BSON format
      OSS_INLINE INT32 toBSON( bson::BSONObjBuilder &builder ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONObjBuilder subBuilder(
                  builder.subobjStart( STP_FIELD_NAME_TIMESTAMP ) ) ;

            // build time into BSON format
            rc = _time.toBSON( subBuilder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            subBuilder.doneFast() ;

            // append time error
            builder.append( STP_FIELD_NAME_TIME_ERROR,
                            (INT32)_timeError ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // format logical time into BSON format
      OSS_INLINE INT32 toBSON( bson::BSONObj &object ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONObjBuilder builder ;

            rc = toBSON( builder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            object = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // parse logical time from BSON object
      OSS_INLINE INT32 fromBSON( const bson::BSONObj &object )
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONElement element ;

            // parse time component
            element = object.getField( STP_FIELD_NAME_TIMESTAMP ) ;
            if ( bson::Object != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            rc = _time.fromBSON( element.embeddedObject() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            // parse time error component
            element = object.getField( STP_FIELD_NAME_TIME_ERROR ) ;
            if ( bson::NumberInt != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            _timeError = (UINT32)( element.numberInt() ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

   protected:
      // time in high precision time ( nanoseconds )
      stpHPTime _time ;
   } ;

   /*
      _stpLogicalTimeUS define
    */
   // _stpLogicalTimeUS is logical time measured in microseconds
   class _stpLogicalTimeUS : public stpLogicalTimeBase
   {
   public:
      // constructor and destructor
      _stpLogicalTimeUS()
      : stpLogicalTimeBase(),
        _time( 0LL )
      {
      }

      _stpLogicalTimeUS( const stpLogicalTimeUS &time )
      : stpLogicalTimeBase( time ),
        _time( time._time )
      {
      }

      _stpLogicalTimeUS( UINT64 time, UINT32 timeError )
      : stpLogicalTimeBase( timeError ),
        _time( time )
      {
      }

      ~_stpLogicalTimeUS()
      {
      }

      // convert from nanoseconds to microseconds
      OSS_INLINE _stpLogicalTimeUS( const stpLogicalTimeNS &time ) ;

   public:
      // operators
      OSS_INLINE stpLogicalTimeUS &operator =( const stpLogicalTimeUS &time )
      {
         _time = time._time ;
         _timeError = time._timeError ;
         return (*this) ;
      }

      // convert from nanoseconds
      OSS_INLINE stpLogicalTimeUS &operator =( const stpLogicalTimeNS &time ) ;

      // equal with time error
      OSS_INLINE BOOLEAN operator ==( const stpLogicalTimeUS &time ) const
      {
         // compare two logical time
         // max time error = max( time error of T1, time error of T2 )
         // if T1 - max time error < T2 < T1 + max time error, they are equal
         // ( within time error )
         UINT32 timeError = _getMaxTimeErrorUS( time ) ;
         return ( time._time < _time + timeError &&
                  time._time + timeError > _time ) ;
      }

      // non-equal with time error
      OSS_INLINE BOOLEAN operator !=( const stpLogicalTimeUS &time ) const
      {
         return !( operator ==( time ) ) ;
      }

      // less than with time error
      OSS_INLINE BOOLEAN operator <( const stpLogicalTimeUS &time ) const
      {
         // compare two logical time
         // max time error = max( time error of T1, time error of T2 )
         // if T1 + max time error <= T2 , then T1 < T2 ( within time error )
         return _time + _getMaxTimeErrorUS( time ) <= time._time ;
      }

      // larger than with time error
      OSS_INLINE BOOLEAN operator >( const stpLogicalTimeUS &time ) const
      {
         return ( time < ( *this ) ) ;
      }

      // not lager than with time error
      OSS_INLINE BOOLEAN operator <=( const stpLogicalTimeUS &time ) const
      {
         return !( time < ( *this ) ) ;
      }

      // not less than with time error
      OSS_INLINE BOOLEAN operator >=( const stpLogicalTimeUS &time ) const
      {
         return !( ( *this ) < time ) ;
      }

   public:
      // set time in microseconds
      OSS_INLINE void setTime( UINT64 time )
      {
         _time = time ;
      }

      // get time in microseconds
      OSS_INLINE UINT64 getTime() const
      {
         return _time ;
      }

      // reset time
      OSS_INLINE void reset()
      {
         _time = 0 ;
         _timeError = 0 ;
      }

      // check whether time is validated
      OSS_INLINE BOOLEAN isValid() const
      {
         return ( 0 != _time &&
                  0 != _timeError ) ;
      }

      // get time with upper time error
      OSS_INLINE UINT64 getUpperTime() const
      {
         return _time + (UINT64)( getTimeErrorUS() ) ;
      }

      // get time with lower time error
      OSS_INLINE UINT64 getLowerTime() const
      {
         return _time - (UINT64)( getTimeErrorUS() ) ;
      }

      // get logical time with upper time error
      OSS_INLINE stpLogicalTimeUS getUpperLogicalTime() const
      {
         return stpLogicalTimeUS( getUpperTime(), _timeError ) ;
      }

      // get logical time with lower time error
      OSS_INLINE stpLogicalTimeUS getLowerLogicalTime() const
      {
         return stpLogicalTimeUS( getLowerTime(), _timeError ) ;
      }

      // format logical time into BSON format
      OSS_INLINE INT32 toBSON( bson::BSONObjBuilder &builder ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            // append time
            builder.append( STP_FIELD_NAME_TIMESTAMP, (INT64)_time ) ;

            // append time error
            builder.append( STP_FIELD_NAME_TIME_ERROR,
                            (INT32)_timeError ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // format logical time into BSON format
      OSS_INLINE INT32 toBSON( bson::BSONObj &object ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONObjBuilder builder ;

            rc = toBSON( builder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            object = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      // parse logical time from BSON object
      OSS_INLINE INT32 fromBSON( const bson::BSONObj &object )
      {
         INT32 rc = SDB_OK ;

         try
         {
            bson::BSONElement element ;

            // parse time component
            element = object.getField( STP_FIELD_NAME_TIMESTAMP ) ;
            if ( bson::NumberLong != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            _time = (UINT64)( element.numberLong() ) ;

            // parse time error component
            element = object.getField( STP_FIELD_NAME_TIME_ERROR ) ;
            if ( bson::NumberInt != element.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            _timeError = (UINT32)( element.numberInt() ) ;
         }
         catch ( std::exception &e )
         {
            (void)e ;
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

   protected:
      // time in microseconds
      UINT64 _time ;
   } ;

   // convert from microseconds to nanoseconds
   OSS_INLINE _stpLogicalTimeNS::_stpLogicalTimeNS( const stpLogicalTimeUS &time )
   : stpLogicalTimeBase( time ),
     _time()
   {
      _time.fromMicroSecond( time.getTime() ) ;
   }

   // convert from microseconds to nanoseconds
   OSS_INLINE stpLogicalTimeNS &_stpLogicalTimeNS::operator =(
                                                const stpLogicalTimeUS &time )
   {
      _time.fromMicroSecond( time.getTime() ) ;
      _timeError = time.getTimeError() ;
      return (*this) ;
   }

   // convert from nanoseconds to microseconds
   OSS_INLINE _stpLogicalTimeUS::_stpLogicalTimeUS( const stpLogicalTimeNS &time )
   : stpLogicalTimeBase( time ),
     _time( time.getTime().toMicroSecond() )
   {
   }

   // convert from nanoseconds to microseconds
   OSS_INLINE stpLogicalTimeUS &_stpLogicalTimeUS::operator =(
                                                const stpLogicalTimeNS &time )
   {
      _time = time.getTime().toMicroSecond() ;
      _timeError = time.getTimeError() ;
      return (*this) ;
   }

}

#endif // STP_LOGICAL_TIME_HPP_
