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

   Source File Name = stpSyncStats.hpp

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

#ifndef STP_SYNC_STATS_HPP__
#define STP_SYNC_STATS_HPP__

#include "stpCBCommon.hpp"
#include "stpLogicalTime.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "ossUtil.hpp"
#include "stpMsg.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _stpSyncRecord define
    */
   class _stpSyncRecord ;
   typedef class _stpSyncRecord stpSyncRecord ;
   typedef ossPoolList< stpSyncRecord > STP_SYNC_REC_LIST ;

   // _stpSyncRecord saves information for a time synchronize round trip
   class _stpSyncRecord : public utilPooledObject
   {
   public:
      // constructors and destructor
      _stpSyncRecord() ;
      _stpSyncRecord( const stpSyncRecord &record ) ;
      _stpSyncRecord( const stpTimeSyncRsp *response ) ;
      ~_stpSyncRecord() ;

   public:
      // operators
      stpSyncRecord &operator =( const stpSyncRecord &record ) ;

   public:
      // get functions
      OSS_INLINE UINT64 getRequestID() const
      {
         return _requestID ;
      }

      OSS_INLINE const stpHPTime &getReqSendTime() const
      {
         return _reqSendTime ;
      }

      OSS_INLINE const stpHPTime &getReqReceiveTime() const
      {
         return _reqReceiveTime ;
      }

      OSS_INLINE const stpHPTime &getRspSendTime() const
      {
         return _rspSendTime ;
      }

      OSS_INLINE const stpHPTime &getRspReceiveTime() const
      {
         return _rspReceiveTime ;
      }

      OSS_INLINE UINT32 getReqTimeError() const
      {
         return _reqTimeError ;
      }

      OSS_INLINE UINT32 getRspTimeError() const
      {
         return _rspTimeError ;
      }

      OSS_INLINE INT64 getSendSkew() const
      {
         return _sendDelay ;
      }

      OSS_INLINE INT64 getReceiveSkew() const
      {
         return _receiveDelay ;
      }

      OSS_INLINE INT64 getOffset() const
      {
         return _offset ;
      }

      OSS_INLINE INT64 getDelay() const
      {
         return _delay ;
      }

   public:
      // check if record is valid for update offset
      BOOLEAN isValid() const ;

      // format record into string
      ossPoolString toString() const ;

   protected:
      // internal calculation function
      void _calculate() ;

   protected:
      // request ID for time synchronize request
      UINT64   _requestID ;
      // send time of time synchronize request ( T1 in synchronize client )
      stpHPTime _reqSendTime ;
      // receive time of time synchronize request ( T2 in synchronize source )
      stpHPTime _reqReceiveTime ;
      // send time of time synchronize response ( T3 in synchronize source )
      stpHPTime _rspSendTime ;
      // receive time of time synchronize response ( T4 in synchronize client )
      stpHPTime _rspReceiveTime ;
      // time error in nanoseconds of request ( from synchronize client )
      UINT32   _reqTimeError ;
      // time error in nanoseconds of response ( from synchronize source )
      // tell client to update with this time error
      UINT32   _rspTimeError ;

      // internal result of calculations
      // send delay between client and source ( T2 - T1 )
      INT64    _sendDelay ;
      // receive delay between source and client ( T4 - T3 )
      INT64    _receiveDelay ;
      // offset between source and client
      INT64    _offset ;
      // network delay between source and client
      INT64    _delay ;
      // time cost for synchronize round trip ( measured with client's time )
      // generally, time cost contains network delay and process time in
      // source
      INT64    _cost ;
   } ;

   /*
      _stpSyncStats define
    */
   // _stpSyncStats saves statistics of synchronize between client and source
   class _stpSyncStats ;
   typedef class _stpSyncStats stpSyncStats ;

   class _stpSyncStats : public utilPooledObject
   {
   public:
      // constructors and destructor
      _stpSyncStats() ;
      _stpSyncStats( const stpSyncStats &stats ) ;
      ~_stpSyncStats() ;

   public:
      // operators
      stpSyncStats &operator =( const stpSyncStats &stats ) ;

   public:
      // get and set functions
      OSS_INLINE UINT64 getSyncCount() const
      {
         return _syncCount ;
      }

      OSS_INLINE void setSyncCount( UINT64 count )
      {
         _syncCount = count ;
      }

      OSS_INLINE UINT64 getValidCount() const
      {
         return _validCount ;
      }

      OSS_INLINE void setValidCount( UINT64 count )
      {
         _validCount = count ;
      }

      OSS_INLINE UINT64 getMaxDelay() const
      {
         return _maxDelay ;
      }

      OSS_INLINE void setMaxDelay( UINT64 delay )
      {
         _maxDelay = delay ;
      }

      OSS_INLINE UINT64 getMinDelay() const
      {
         return _minDelay ;
      }

      OSS_INLINE void setMinDelay( UINT64 delay )
      {
         _minDelay = delay ;
      }

      OSS_INLINE INT64 getInitOffset() const
      {
         return _initOffset ;
      }

      OSS_INLINE void setInitOffset( INT64 initOffset )
      {
         _initOffset = initOffset ;
      }

      OSS_INLINE INT64 getMaxPosOffset() const
      {
         return _maxPosOffset ;
      }

      OSS_INLINE void setMaxPosOffset( INT64 offset )
      {
         _maxPosOffset = offset ;
      }

      OSS_INLINE INT64 getMaxNegOffset() const
      {
         return _maxNegOffset ;
      }

      OSS_INLINE void setMaxNegOffset( INT64 offset )
      {
         _maxNegOffset = offset ;
      }

      OSS_INLINE INT64 getMaxValidPosOffset() const
      {
         return _maxValidPosOffset ;
      }

      OSS_INLINE void setMaxValidPosOffset( INT64 offset )
      {
         _maxValidPosOffset = offset ;
      }

      OSS_INLINE INT64 getMaxValidNegOffset() const
      {
         return _maxValidNegOffset ;
      }

      OSS_INLINE void setMaxValidNegOffset( INT64 offset )
      {
         _maxValidNegOffset = offset ;
      }

      OSS_INLINE INT64 getMinPosOffset() const
      {
         return _minPosOffset ;
      }

      OSS_INLINE void setMinPosOffset( UINT64 offset )
      {
         _minPosOffset = offset ;
      }

      OSS_INLINE INT64 getMinNegOffset() const
      {
         return _minNegOffset ;
      }

      OSS_INLINE void setMinMegOffset( UINT64 offset )
      {
         _minNegOffset = offset ;
      }

   public:
      // reset statistics
      void reset() ;
      // increase synchronize count ( called on sending synchronize request )
      void incSyncCount() ;
      // update statistics by a synchronize record
      void updateStats( const stpSyncRecord &record, BOOLEAN isValid ) ;
      // update statistics by another history ( merge histories )
      void updateStats( const stpSyncStats &stats ) ;

      // format history to BSON object
      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

   protected:
      // count of synchronizes ( how many times to send synchronize requests )
      UINT64   _syncCount ;
      // count of valid synchronizes ( delay is valid )
      UINT64   _validCount ;
      // maximum delay of all synchronizes
      UINT64   _maxDelay ;
      // maximum delay of all valid synchronizes
      UINT64   _maxValidDelay ;
      // minimum delay of all synchronizes
      UINT64   _minDelay ;
      // first offset of all synchronizes
      INT64    _initOffset ;
      // maximum positive offset of all synchronizes ( behind source )
      INT64    _maxPosOffset ;
      // maximum negative offset of all synchronizes ( ahead source )
      INT64    _maxNegOffset ;
      // maximum positive offset of all valid synchronizes ( behind source )
      INT64    _maxValidPosOffset ;
      // maximum negative offset of all valid synchronizes ( ahead source )
      INT64    _maxValidNegOffset ;
      // minimum positive offset of all synchronizes ( behind source )
      INT64    _minPosOffset ;
      // minimum negative offset of all synchronizes ( ahead source )
      INT64    _minNegOffset ;
   } ;

}

#endif // STP_SYNC_STATS_HPP__
