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

   Source File Name = tpLogicalTime.hpp

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

#ifndef TP_LOGICAL_TIME_HPP_
#define TP_LOGICAL_TIME_HPP_

#include "tpCommon.hpp"
#include "tpToolCommon.hpp"
#include "utilPooledObject.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"

namespace engine
{

   /*
      TP_TIME_SAMPLE_MODE
    */
   enum TP_TIME_SAMPLE_MODE
   {
      TP_TIME_SAMPLE_NONE,
      TP_TIME_SAMPLE_REAL,
      TP_TIME_SAMPLE_MONOTONIC
   } ;

   /*
      _tpHPTime: high precision time define
    */
   class _tpHPTime ;
   typedef class _tpHPTime tpHPTime ;

   class _tpHPTime : public utilPooledObject
   {
   public:
      _tpHPTime()
      : _second( 0LL ),
        _nanoSecond( 0LL )
      {
      }

      _tpHPTime( TP_TIME_SAMPLE_MODE mode )
      : _second( 0LL ),
        _nanoSecond( 0LL )
      {
         sample( mode ) ;
      }

      _tpHPTime( UINT64 second, UINT64 nanoSecond )
      : _second( second ),
        _nanoSecond( nanoSecond )
      {
      }

      _tpHPTime( const tpHPTime &time )
      : _second( time._second ),
        _nanoSecond( time._nanoSecond )
      {
      }

      ~_tpHPTime()
      {
      }

   public:
      OSS_INLINE tpHPTime &operator =( const tpHPTime &time )
      {
         _second = time._second ;
         _nanoSecond = time._nanoSecond ;

         return ( *this ) ;
      }

      friend tpHPTime operator +( const tpHPTime &lhs, const tpHPTime &rhs ) ;
      friend tpHPTime operator -( const tpHPTime &lhs, const tpHPTime &rhs ) ;
      friend tpHPTime operator +( const tpHPTime &lhs, UINT64 rhs ) ;
      friend tpHPTime operator -( const tpHPTime &lhs, UINT64 rhs ) ;

      OSS_INLINE BOOLEAN operator ==( const tpHPTime &time ) const
      {
         return ( _second == time._second &&
                  _nanoSecond == time._nanoSecond ) ;
      }

      OSS_INLINE BOOLEAN operator !=( const tpHPTime &time ) const
      {
         return !( operator ==( time ) ) ;
      }

      OSS_INLINE BOOLEAN operator <( const tpHPTime &time ) const
      {
         return ( _second < time._second ||
                  ( _second == time._second &&
                    _nanoSecond < time._nanoSecond ) ) ;
      }

      OSS_INLINE BOOLEAN operator >( const tpHPTime &time ) const
      {
         return time.operator <( *this ) ;
      }

      OSS_INLINE BOOLEAN operator <=( const tpHPTime &time ) const
      {
         return !( operator >( time ) ) ;
      }

      OSS_INLINE BOOLEAN operator >=( const tpHPTime &time ) const
      {
         return !( operator <( time ) ) ;
      }

   public:
      OSS_INLINE UINT64 getSecond() const
      {
         return _second ;
      }

      OSS_INLINE UINT64 getNanoSecond() const
      {
         return _nanoSecond ;
      }

      OSS_INLINE BOOLEAN isZero() const
      {
         return ( 0LL == _second && 0LL == _nanoSecond ) ;
      }

      OSS_INLINE void reset()
      {
         _second = 0LL ;
         _nanoSecond = 0LL ;
      }

      OSS_INLINE UINT64 toNanoSecond() const
      {
         return TP_SEC_TO_NANOSEC( _second ) + _nanoSecond ;
      }

      OSS_INLINE void fromNanoSecond( UINT64 nanoSecond )
      {
         _second = 0LL ;
         _nanoSecond = nanoSecond ;
         _normalize() ;
      }

      OSS_INLINE UINT64 toMicroSecond() const
      {
         return TP_SEC_TO_MICROSEC( _second ) +
                TP_NANOSEC_TO_MICROSEC( _nanoSecond ) ;
      }

      OSS_INLINE void fromMicroSecond( UINT64 microSecond )
      {
         _second = microSecond / OSS_ONE_MILLION ;
         _nanoSecond = microSecond % OSS_ONE_MILLION * OSS_ONE_THOUSAND ;
      }

      OSS_INLINE INT32 toBSON( bson::BSONObjBuilder &builder ) const
      {
         INT32 rc = SDB_OK ;

         try
         {
            builder.append( TP_FIELD_NAME_SECOND, (INT64)_second ) ;
            builder.append( TP_FIELD_NAME_NANO_SECOND, (INT64)_nanoSecond ) ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      OSS_INLINE INT32 toBSON( const CHAR *fieldName,
                               bson::BSONObjBuilder &builder ) const
      {
         INT32 rc = SDB_OK ;

         SDB_ASSERT( NULL != fieldName, "field name is invalid" ) ;

         try
         {
            bson::BSONObjBuilder subBuilder(
                                          builder.subobjStart( fieldName ) ) ;
            rc = toBSON( subBuilder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            subBuilder.doneFast() ;
         }
         catch ( std::exception &e )
         {
            rc = SDB_SYS ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      OSS_INLINE void sample( TP_TIME_SAMPLE_MODE mode )
      {
         switch ( mode )
         {
            case TP_TIME_SAMPLE_REAL :
            {
               sampleReal() ;
               break ;
            }
            case TP_TIME_SAMPLE_MONOTONIC :
            {
               sampleMonotonic() ;
               break ;
            }
            default :
            {
               _second = 0LL ;
               _nanoSecond = 0LL ;
               break ;
            }
         }
      }

      OSS_INLINE void sampleReal()
      {
#if defined (_WINDOWS)
         UINT64 value = 0LL ;
         ossTimestamp tm ;
         ossGetCurrentTime( tm ) ;
         _second = (UINT64)( tm.time ) ;
         _nanoSecond = (UINT64)( tm.microtm * OSS_ONE_THOUSAND ) ;
#else
         struct timespec ts ;
         if ( 0 == clock_gettime( CLOCK_REALTIME, &ts ) )
         {
            _second = (UINT64)( ts.tv_sec ) ;
            _nanoSecond = (UINT64)( ts.tv_nsec ) ;
         }
#endif
      }

      OSS_INLINE void sampleMonotonic()
      {
#if defined (_WINDOWS)
         LARGE_INTEGER freq ;
         LARGE_INTEGER count ;
         if ( QueryPerformanceFrequency( &freq ) &&
              QueryPerformanceCounter( &count ) &&
              freq.QuadPart != 0LL )
         {
            double scale = (double)OSS_ONE_BILLION / (double)( freq.QuadPart ) ;
            UINT64 value = (UINT64)( (double)( count.QuadPart ) * scale ) ;
            _second = value / OSS_ONE_BILLION ;
            _nanoSecond = value % OSS_ONE_BILLION ;
         }
#else
         struct timespec ts ;
         if ( 0 == clock_gettime( CLOCK_MONOTONIC_RAW, &ts ) )
         {
            _second = (UINT64)( ts.tv_sec ) ;
            _nanoSecond = (UINT64)( ts.tv_nsec ) ;
         }
#endif
      }

      OSS_INLINE void scale( UINT64 slewRate )
      {
         if ( slewRate > 0 )
         {
            double rate = (double)slewRate / (double)TP_DEF_SLEWRATE ;
            if ( rate > 1.0 )
            {
               _second = (UINT64)( (double)_second * rate ) ;
               _nanoSecond = (UINT64)( (double)_nanoSecond * rate ) ;
            }
            else
            {
               double fraction = (double)_second * rate ;
               _second = (UINT64)fraction ;
               fraction = fraction - (double)_second ;
               _nanoSecond = (UINT64)( fraction * (double)OSS_ONE_BILLION +
                                        (double)_nanoSecond * rate ) ;
            }
            _normalize() ;
         }
      }

      OSS_INLINE void adjust( INT64 offset )
      {
         if ( offset > 0 )
         {
            _nanoSecond += offset ;
         }
         else if ( offset < 0 )
         {
            offset = -1 * offset ;
            UINT64 second = offset / OSS_ONE_BILLION ;
            UINT64 nanoSecond = offset % OSS_ONE_BILLION ;
            if ( _nanoSecond > nanoSecond )
            {
               _nanoSecond -= nanoSecond ;
            }
            else if ( _second > 0LL )
            {
               -- _second ;
               _nanoSecond = _nanoSecond + OSS_ONE_BILLION - nanoSecond ;
            }
            if ( _second > second )
            {
               _second -= second ;
            }
         }
         _normalize() ;
      }

      OSS_INLINE INT64 diff( const tpHPTime &time ) const
      {
         INT64 result = 0LL ;

         if ( operator >( time ) )
         {
            tpHPTime tempTime = ( *this ) - time ;
            result = tempTime.toNanoSecond() ;
         }
         else
         {
            tpHPTime tempTime = time - ( *this ) ;
            result = -1 * (INT64)( tempTime.toNanoSecond() ) ;
         }

         return result ;
      }

   protected:
      OSS_INLINE void _normalize()
      {
         _second += ( _nanoSecond / OSS_ONE_BILLION ) ;
         _nanoSecond %= OSS_ONE_BILLION ;
      }

   protected:
      UINT64 _second ;
      UINT64 _nanoSecond ;
   } ;


   OSS_INLINE tpHPTime operator +( const tpHPTime &lhs, const tpHPTime &rhs )
   {
      tpHPTime result ;

      result._second = lhs._second + rhs._second ;
      result._nanoSecond = lhs._nanoSecond + rhs._nanoSecond ;
      result._normalize() ;

      return result ;
   }

   OSS_INLINE tpHPTime operator -( const tpHPTime &lhs, const tpHPTime &rhs )
   {
      tpHPTime result ;

      if ( lhs > rhs )
      {
         result._second = lhs._second - rhs._second ;
         if ( rhs._nanoSecond > lhs._nanoSecond )
         {
            -- result._second ;
            result._nanoSecond = lhs._nanoSecond + OSS_ONE_BILLION -
                                  rhs._nanoSecond ;
         }
         else
         {
            result._nanoSecond = lhs._nanoSecond - rhs._nanoSecond ;
         }
      }

      return result ;
   }

   OSS_INLINE tpHPTime operator +( const tpHPTime &lhs, UINT64 rhs )
   {
      tpHPTime result = lhs ;
      result.adjust( (INT64)rhs ) ;
      return result ;
   }

   OSS_INLINE tpHPTime operator -( const tpHPTime &lhs, UINT64 rhs )
   {
      tpHPTime result = lhs ;
      result.adjust( (INT64)( -1 * rhs ) ) ;
      return result ;
   }

   /*
      _tpLogicalTimeBase define
    */
   class _tpLogicalTimeBase ;
   typedef class _tpLogicalTimeBase tpLogicalTimeBase ;

   class _tpLogicalTimeBase : public utilPooledObject
   {
   public:
      _tpLogicalTimeBase()
      : _timeError( 0 )
      {
      }

      _tpLogicalTimeBase( const tpLogicalTimeBase &time )
      : _timeError( time._timeError )
      {
      }

      _tpLogicalTimeBase( UINT32 timeError )
      : _timeError( timeError )
      {
      }

      ~_tpLogicalTimeBase()
      {
      }

      OSS_INLINE void setTimeError( UINT32 timeError )
      {
         _timeError = timeError ;
      }

      OSS_INLINE UINT32 getTimeError() const
      {
         return _timeError ;
      }

   protected:
      OSS_INLINE UINT64 _getMaxTimeError( const tpLogicalTimeBase &time ) const
      {
         return OSS_MAX( _timeError, time._timeError ) ;
      }

   protected:
      UINT32   _timeError ;
   } ;

   /*
      _tpLogicalTimeNS define
    */
   class _tpLogicalTimeNS ;
   typedef class _tpLogicalTimeNS tpLogicalTimeNS ;

   class _tpLogicalTimeNS : public tpLogicalTimeBase
   {
   public:
      _tpLogicalTimeNS()
      : tpLogicalTimeBase(),
        _time()
      {
      }

      _tpLogicalTimeNS( const tpLogicalTimeNS &time )
      : tpLogicalTimeBase( time ),
        _time( time._time )
      {
      }

      _tpLogicalTimeNS( const tpHPTime &time, UINT32 timeError )
      : tpLogicalTimeBase( timeError ),
        _time( time )
      {
      }

      ~_tpLogicalTimeNS()
      {
      }

   public:
      OSS_INLINE tpLogicalTimeNS &operator =( const tpLogicalTimeNS &time )
      {
         _time = time._time ;
         _timeError = time._timeError ;
         return (*this) ;
      }

      // equal with time error
      OSS_INLINE BOOLEAN operator ==( const tpLogicalTimeNS &time ) const
      {
         UINT64 timeError = _getMaxTimeError( time ) ;
         return ( ( time._time - timeError ) < _time &&
                  ( time._time + timeError ) > _time ) ;
      }

      // less than with time error
      OSS_INLINE BOOLEAN operator <( const tpLogicalTimeNS &time ) const
      {
         return _time <= ( time._time - _getMaxTimeError( time ) ) ;
      }

   public:
      OSS_INLINE void setTime( const tpHPTime &time )
      {
         _time = time ;
      }

      OSS_INLINE const tpHPTime &getTime() const
      {
         return _time ;
      }

   protected:
      tpHPTime _time ;
   } ;

   /*
      _tpLogicalTimeUS define
    */
   class _tpLogicalTimeUS ;
   typedef class _tpLogicalTimeUS tpLogicalTimeUS ;

   class _tpLogicalTimeUS : public tpLogicalTimeBase
   {
   public:
      _tpLogicalTimeUS()
      : tpLogicalTimeBase(),
        _time( 0LL )
      {
      }

      _tpLogicalTimeUS( const tpLogicalTimeUS &time )
      : tpLogicalTimeBase( time ),
        _time( time._time )
      {
      }

      _tpLogicalTimeUS( const tpLogicalTimeNS &time )
      : tpLogicalTimeBase( time ),
        _time( time.getTime().toMicroSecond() )
      {
      }

      _tpLogicalTimeUS( UINT64 time, UINT32 timeError )
      : tpLogicalTimeBase( timeError ),
        _time( time )
      {
      }

      ~_tpLogicalTimeUS()
      {
      }

   public:
      OSS_INLINE tpLogicalTimeUS &operator =( const tpLogicalTimeUS &time )
      {
         _time = time._time ;
         _timeError = time._timeError ;
         return (*this) ;
      }

      OSS_INLINE tpLogicalTimeUS &operator =( const tpLogicalTimeNS &time )
      {
         _time = time.getTime().toMicroSecond() ;
         _timeError = time.getTimeError() ;
         return (*this) ;
      }

      // equal with time error
      OSS_INLINE BOOLEAN operator ==( const tpLogicalTimeUS &time ) const
      {
         UINT32 timeError = _getMaxTimeError( time ) ;
         return ( time._time - timeError < _time &&
                  time._time + timeError > _time ) ;
      }

      // less than with time error
      OSS_INLINE BOOLEAN operator <( const tpLogicalTimeUS &time ) const
      {
         return _time <= time._time - _getMaxTimeError( time ) ;
      }

   public:
      OSS_INLINE void setTimestamp( UINT64 time )
      {
         _time = time ;
      }

      OSS_INLINE UINT64 getTimestamp() const
      {
         return _time ;
      }

   protected:
      UINT64   _time ;
   } ;

}

#endif // TP_LOGICAL_TIME_HPP_
