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

   Source File Name = tpMetaData.hpp

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

#ifndef TP_META_DATA_HPP_
#define TP_META_DATA_HPP_

#include "tpCommon.hpp"
#include "tpLogicalTime.hpp"
#include "oss.hpp"
#include "ossRWMutex.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.h"

namespace engine
{

   class _tpMetaData ;
   typedef class _tpMetaData tpMetaData ;

   /*
      _tpMetaData define
    */
   class _tpMetaData : public SDBObject
   {
   public:
      _tpMetaData() ;
      ~_tpMetaData() ;

   public:
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

      OSS_INLINE BOOLEAN hasSynchronized() const
      {
         return ( !( _syncTime.isZero() ) ) ;
      }

      OSS_INLINE void updateSyncTime()
      {
         _syncTime.sampleMonotonic() ;
      }

      OSS_INLINE const tpHPTime &getBaseHardwareTime() const
      {
         return _baseHWTime ;
      }

      OSS_INLINE void setBaseHardwareTime( const tpHPTime &baseHardwareTime )
      {
         _baseHWTime = baseHardwareTime ;
      }

      OSS_INLINE const tpHPTime &getBaseRealTime() const
      {
         return _baseRealTime ;
      }

      OSS_INLINE void setBaseRealTime( const tpHPTime &baseRealTime )
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

      OSS_INLINE void cutTimeError( UINT32 maxTimeError )
      {
         if ( _timeError > maxTimeError )
         {
            _timeError = maxTimeError ;
         }
      }

   public:
      OSS_INLINE tpHPTime getLTValue() const
      {
         return _getLTValue() ;
      }

      OSS_INLINE UINT64 getLTValueUS() const
      {
         return _getLTValue().toMicroSecond() ;
      }

      void initialize( UINT64 timeUS, UINT32 syncInterval ) ;
      void reset( UINT64 timeUS ) ;

      INT32 getLogicalTimeNS( tpLogicalTimeNS &time,
                              BOOLEAN checkSync ) const ;
      INT32 getLogicalTimeUS( tpLogicalTimeUS &time,
                              BOOLEAN checkSync ) const ;

      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;
      ossPoolString toString() const ;

      void resetSlewRate() ;
      void adjustLogicalTime( INT64 syncOffset ) ;
      void adjustLogicalTime( INT64 syncOffset, UINT32 timeError ) ;
      void adjustSlewRate( UINT64 sourceInterval, UINT64 localInterval ) ;

      static UINT32 getBufferSize() ;
      static tpMetaData *getBuffer( CHAR *buffer ) ;
      static tpMetaData *newBuffer( CHAR *buffer ) ;

   protected:
      OSS_INLINE tpHPTime _getLTValue() const
      {
         tpHPTime curHWTime ;
         curHWTime.sampleMonotonic() ;
         return _getLTValue( curHWTime ) ;
      }

      OSS_INLINE tpHPTime _getLTValue( const tpHPTime &curHWTime ) const
      {
         // logical time = ( current HW time -
         //                  base HW time +
         //                  base real time ) * slew rate / TP_DEF_SLEWRATE +
         //                offset
         // NOTE: HW ( hardware time ) is monotonic time from machine
         //       slew rate is to adjust speeds of CPU ticks between different
         //       machines
         tpHPTime result = curHWTime - _baseHWTime + _baseRealTime ;
         result.scale( _slewRate ) ;
         result.adjust( _offset ) ;
         return result ;
      }

   protected:
      UINT32      _version ;
      // synchronize interval, measured in second
      UINT32      _syncInterval ;
      // last synchronize time, measured in nanosecond
      tpHPTime    _syncTime ;
      // logical time components, measured in nanosecond
      // based hardware time
      tpHPTime    _baseHWTime ;
      // based real time ( makes the logical time around the real time )
      tpHPTime    _baseRealTime ;
      INT64       _offset ;
      UINT64      _slewRate ;
      // time error, measured in nanosecond
      UINT32      _timeError ;
   } ;

   /*
      _tpMetaHolder define
    */
   class _tpMetaHolder
   {
   public:
      _tpMetaHolder()
      : _metaData( NULL )
      {
      }

      ~_tpMetaHolder()
      {
      }

   public:
      OSS_INLINE tpMetaData *getMetaData()
      {
         return _metaData ;
      }

      OSS_INLINE void setMetaData( tpMetaData *metaData )
      {
         _metaData = metaData ;
      }

   protected:
      tpMetaData * _metaData ;
   } ;

   typedef class _tpMetaHolder tpMetaHolder ;

   /*
      _tpMetaReader define
    */
   class _tpMetaReader
   {
   public:
      _tpMetaReader()
      : _metaData( NULL )
      {
      }

      ~_tpMetaReader()
      {
      }

   public:
      OSS_INLINE const tpMetaData *getMetaData()
      {
         return _metaData ;
      }

      OSS_INLINE void setMetaData( const tpMetaData *metaData )
      {
         _metaData = metaData ;
      }

   protected:
      const tpMetaData * _metaData ;
   } ;

   typedef class _tpMetaReader tpMetaReader ;

}

#endif // TP_META_DATA_HPP_
