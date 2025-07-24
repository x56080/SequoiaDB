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

   Source File Name = stpMetaData.hpp

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

#ifndef STP_META_DATA_HPP_
#define STP_META_DATA_HPP_

#include "stpCommon.hpp"
#include "stpLogicalTime.hpp"
#include "oss.hpp"
#include "ossRWMutex.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"

namespace engine
{

   // sleep time ( 100ms ) for retry getting logical time
   #define STP_GET_TIME_RETRY_INTERVAL ( 100 )
   // sleep time ( 1ms ) for quick retry getting logical time
   #define STP_GET_TIME_MIN_RETRY_INTERVAL ( 1 )

   /*
      _stpMetaData define
    */
   class _stpMetaData ;
   typedef class _stpMetaData stpMetaData ;

   // _stpMetaData contains components to calculate logical time
   class _stpMetaData : public SDBObject
   {
   public:
      // constructor and destructor
      _stpMetaData() ;
      ~_stpMetaData() ;

   public:
      // get and set functions
      OSS_INLINE UINT32 getVersion() const
      {
         return _version ;
      }

      OSS_INLINE void setVersion( UINT32 version )
      {
         _version = version ;
      }

      OSS_INLINE UINT32 getSyncInterval() const
      {
         return _syncInterval ;
      }

      OSS_INLINE void setSyncInterval( UINT32 syncInterval )
      {
         _syncInterval = syncInterval ;
      }

      OSS_INLINE const stpHPTime &getSyncHardwareTime() const
      {
         return _syncHWTime ;
      }

      OSS_INLINE void setSyncHardwareTime( const stpHPTime &syncHWTime )
      {
         _syncHWTime = syncHWTime ;

         // wake up watchers
         wakeUpSyncWatchers() ;
      }

      OSS_INLINE const stpHPTime &getBaseHardwareTime() const
      {
         return _baseHWTime ;
      }

      OSS_INLINE void setBaseHardwareTime( const stpHPTime &baseHardwareTime )
      {
         _baseHWTime = baseHardwareTime ;
      }

      OSS_INLINE const stpHPTime &getBaseRealTime() const
      {
         return _baseRealTime ;
      }

      OSS_INLINE void setBaseRealTime( const stpHPTime &baseRealTime )
      {
         _baseRealTime = baseRealTime ;
      }

      OSS_INLINE INT64 getOffset() const
      {
         return _offset ;
      }

      OSS_INLINE void setOffset( INT64 offset )
      {
         _offset = offset ;
      }

      OSS_INLINE UINT64 getSlewRate() const
      {
         return _slewRate ;
      }

      OSS_INLINE void setSlewRate( UINT64 slewRate )
      {
         _slewRate = slewRate ;
      }

      OSS_INLINE UINT32 getTimeError() const
      {
         return _timeError ;
      }

      OSS_INLINE void setTimeError( UINT32 timeError )
      {
         _timeError = timeError ;
      }

      // cut time error to maximum time error
      OSS_INLINE void cutTimeError( UINT32 maxTimeError )
      {
         if ( _timeError > maxTimeError )
         {
            _timeError = maxTimeError ;
         }
      }

      // check if meta data has been synchronized
      OSS_INLINE BOOLEAN hasSynchronized() const
      {
         return ( !( _syncHWTime.isZero() ) ) ;
      }

      // update synchronize time
      OSS_INLINE void updateSyncTime()
      {
         // update synchronize time with hardware time
         _syncHWTime.sampleMonotonic() ;

         // wake up watchers
         wakeUpSyncWatchers() ;
      }

   public:
      // get logical time value in high precision format
      // only time without time error
      OSS_INLINE stpHPTime getLTValue() const
      {
         return _getLTValue() ;
      }

      // get logical time value in microseconds
      // only time without time error
      OSS_INLINE UINT64 getLTValueUS() const
      {
         return _getLTValue().toMicroSecond() ;
      }

      // initialize meta data
      void initialize( UINT64 timeUS, UINT32 syncInterval ) ;
      // reset meta data to given time
      void reset( UINT64 timeUS ) ;

      // quick interface to get non-monotonic logical time
      // NOTE: timeout in one second
      INT32 getLogicalTimeNS( stpLogicalTimeNS &time ) const ;

      // get logical time in nanoseconds
      // NOTE: check whether time is synchronized if needed
      INT32 getLogicalTimeNS( stpLogicalTimeNS &time,
                              BOOLEAN monotonic,
                              BOOLEAN checkSync,
                              UINT32 &waitTimeUS ) const ;
      // get logical time in microseconds
      // NOTE: check whether time is synchronized if needed
      INT32 getLogicalTimeUS( stpLogicalTimeUS &time,
                              BOOLEAN monotonic,
                              BOOLEAN checkSync,
                              UINT32 &waitTimeUS ) const ;

      // output meta data to BSON format
      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;
      // parse meta from BSON object
      INT32 fromBSON( const bson::BSONObj &object ) ;
      // output meta data to string format
      ossPoolString toString() const ;

      // reset to default slew rate
      void resetSlewRate() ;
      // adjust logical time by synchronized offset
      void adjustLogicalTime( INT64 syncOffset ) ;
      // adjust logical time by synchronized offset and time error
      void adjustLogicalTime( INT64 syncOffset, UINT32 timeError ) ;
      // adjust slew rate by interval between synchronized source and client
      void adjustSlewRate( UINT64 sourceInterval, UINT64 localInterval ) ;

      // get buffer size of shared memory
      static UINT32 getBufferSize() ;
      // get meta data from shared memory buffer
      static stpMetaData *getBuffer( CHAR *buffer ) ;
      // construct meta data from shared memory buffer
      static stpMetaData *newBuffer( CHAR *buffer ) ;

      void watchSync( INT32 oldWatcher, INT64 timeout ) const ;
      void wakeUpSyncWatchers() ;

      OSS_INLINE INT32 getSyncWatcher() const
      {
         return _syncHWTime.getWatchValue() ;
      }

   protected:
      // get current logical time ( without time error )
      OSS_INLINE stpHPTime _getLTValue() const
      {
         stpHPTime curHWTime ;
         curHWTime.sampleMonotonic() ;
         return _getLTValue( curHWTime ) ;
      }

      // get logical time with given current hardware time
      // ( without time error )
      OSS_INLINE stpHPTime _getLTValue( const stpHPTime &curHWTime ) const
      {
         // logical time = ( current HW time -
         //                  base HW time ) * slew rate / STP_DEF_SLEWRATE +
         //                offset + base real time
         // NOTE: HW ( hardware time ) is monotonic time from machine
         //       slew rate is to adjust speeds of CPU ticks between different
         //       machines
         stpHPTime result = curHWTime - _baseHWTime ;
         result.scale( _slewRate ) ;
         result.adjust( _offset ) ;
         result = result + _baseRealTime ;
         return result ;
      }

      // format high precision time to BSON format
      INT32 _timeToBSON( bson::BSONObjBuilder &builder,
                         const CHAR *fieldName,
                         const stpHPTime &hpTime ) const ;

      // parse high precision time from BSON format
      INT32 _timeFromBSON( const bson::BSONObj &object,
                           const CHAR *fieldName,
                           stpHPTime &hpTime ) ;

   protected:
      // version of meta data ( STP_VERSION )
      // NOTE: need increase version if structure of meta data is changed
      UINT32      _version ;
      // synchronize interval, measured in second
      UINT32      _syncInterval ;
      // last synchronize hardware time, measured in nanosecond
      // NOTE: if hardware time of logical time is in
      //       [ _syncHWTime, _syncHWTime + syncInterval ], the logical time is
      //       available for STP agent, otherwise, it is not available, STP
      //       agent should wait or retry
      stpHPTime    _syncHWTime ;
      // logical time components, measured in nanosecond
      // based hardware time
      stpHPTime    _baseHWTime ;
      // based real time ( makes the logical time around the real time )
      stpHPTime    _baseRealTime ;
      // offset to synchronize source
      INT64       _offset ;
      // slew rate between CPU ticks of synchronize source and client
      UINT64      _slewRate ;
      // time error, measured in nanosecond
      UINT32      _timeError ;
   } ;

   /*
      _stpMetaHolder define
    */
   // _stpMetaHolder holds pointer to meta data
   class _stpMetaHolder
   {
   public:
      // constructor and destructor
      _stpMetaHolder()
      : _metaData( NULL )
      {
      }

      ~_stpMetaHolder()
      {
      }

   public:
      // get meta data
      OSS_INLINE stpMetaData *getMetaData()
      {
         return _metaData ;
      }

      // set meta data
      OSS_INLINE void setMetaData( stpMetaData *metaData )
      {
         _metaData = metaData ;
      }

   protected:
      // pointer to meta data
      stpMetaData * _metaData ;
   } ;

   typedef class _stpMetaHolder stpMetaHolder ;

   /*
      _stpMetaReader define
    */
   // _stpMetaReader holds read-only meta data
   class _stpMetaReader
   {
   public:
      // constructor and destructor
      _stpMetaReader()
      : _metaData( NULL )
      {
      }

      ~_stpMetaReader()
      {
      }

   public:
      // get meta data
      OSS_INLINE const stpMetaData *getMetaData()
      {
         return _metaData ;
      }

      // set meta data
      OSS_INLINE void setMetaData( const stpMetaData *metaData )
      {
         _metaData = metaData ;
      }

   protected:
      // pointer to meta data
      const stpMetaData * _metaData ;
   } ;

   typedef class _stpMetaReader stpMetaReader ;

}

#endif // STP_META_DATA_HPP_
