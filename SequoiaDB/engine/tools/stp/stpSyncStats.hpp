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
      _stpSyncRecord( const stpTimeSyncRsp *response,
                      STP_SYNC_STATUS status ) ;
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

      OSS_INLINE UINT64 getSyncTick() const
      {
         return _syncTick ;
      }

      OSS_INLINE STP_SYNC_STATUS getStatus() const
      {
         return _status ;
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

      // check if offset is in the range of time error
      BOOLEAN isOffsetInTimeError( FLOAT64 scale,
                                   FLOAT64 &ratio ) const ;

      // format record into string
      ossPoolString toString() const ;

      // format record into BSON
      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

      // parse record from BSON
      INT32 fromBSON( const bson::BSONObj &object ) ;

   protected:
      // internal calculation function
      void _calculate() ;

   protected:
      // request ID for time synchronize request
      UINT64      _requestID ;
      // tick to finish synchronize
      UINT64      _syncTick ;
      STP_SYNC_STATUS _status ;
      // send time of time synchronize request ( T1 in synchronize client )
      stpHPTime   _reqSendTime ;
      // receive time of time synchronize request ( T2 in synchronize source )
      stpHPTime   _reqReceiveTime ;
      // send time of time synchronize response ( T3 in synchronize source )
      stpHPTime   _rspSendTime ;
      // receive time of time synchronize response ( T4 in synchronize client )
      stpHPTime    _rspReceiveTime ;
      // time error in nanoseconds of request ( from synchronize client )
      UINT32      _reqTimeError ;
      // time error in nanoseconds of response ( from synchronize source )
      // tell client to update with this time error
      UINT32      _rspTimeError ;

      // internal result of calculations
      // send delay between client and source ( T2 - T1 )
      INT64       _sendDelay ;
      // receive delay between source and client ( T4 - T3 )
      INT64       _receiveDelay ;
      // offset between source and client
      INT64       _offset ;
      // network delay between source and client
      INT64       _delay ;
      // time cost for synchronize round trip ( measured with client's time )
      // generally, time cost contains network delay and process time in
      // source
      INT64       _cost ;
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

      OSS_INLINE INT64 getMinPosOffset() const
      {
         return _minPosOffset ;
      }

      OSS_INLINE void setMinPosOffset( INT64 offset )
      {
         _minPosOffset = offset ;
      }

      OSS_INLINE INT64 getMinNegOffset() const
      {
         return _minNegOffset ;
      }

      OSS_INLINE void setMinMegOffset( INT64 offset )
      {
         _minNegOffset = offset ;
      }

      OSS_INLINE UINT64 getLastDelay() const
      {
         return _lastDelay ;
      }

      OSS_INLINE void setLastDelay( UINT64 delay )
      {
         _lastDelay = delay ;
      }

      OSS_INLINE INT64 getLastOffset() const
      {
         return _lastOffset ;
      }

      OSS_INLINE void setLastOffset( INT64 offset )
      {
         _lastOffset = offset ;
      }

      OSS_INLINE UINT64 getUpdateTick() const
      {
         return _updateTick ;
      }

      OSS_INLINE void setUpdateTick( UINT64 updateTick )
      {
         _updateTick = updateTick ;
      }

      OSS_INLINE const STP_SYNC_REC_LIST &getHistList() const
      {
         return _histList ;
      }

   public:
      // reset statistics
      void reset() ;
      // increase synchronize count ( called on sending synchronize request )
      void incSyncCount() ;
      // update statistics by a synchronize record
      // WANRING: should be protected by source lock of stpSyncClientManager
      void updateStats( const stpSyncRecord &record, BOOLEAN isValid,
                        UINT32 maxSyncHist ) ;
      // update statistics by another history ( merge histories )
      // WANRING: should be protected by source lock of stpSyncClientManager
      void updateStats( const stpSyncStats &stats ) ;

      // format history to BSON object
      INT32 toBSON( bson::BSONObjBuilder &builder,
                    BOOLEAN isCurrent ) const ;

      // parse from BSONObj
      INT32 fromBSON( const bson::BSONObj &object ) ;

   protected:
      // add history record
      // WANRING: should be protected by source lock of stpSyncClientManager
      void _addHist( const stpSyncRecord &record, UINT32 maxSyncHist ) ;

   protected:
      // count of synchronizes ( how many times to send synchronize requests )
      UINT64   _syncCount ;
      // count of valid synchronizes ( delay is valid )
      UINT64   _validCount ;
      // maximum delay of all synchronizes
      UINT64   _maxDelay ;
      // minimum delay of all synchronizes
      UINT64   _minDelay ;
      // first offset of all synchronizes
      INT64    _initOffset ;
      // maximum positive offset of all synchronizes ( behind source )
      INT64    _maxPosOffset ;
      // maximum negative offset of all synchronizes ( ahead source )
      INT64    _maxNegOffset ;
      // minimum positive offset of all synchronizes ( behind source )
      INT64    _minPosOffset ;
      // minimum negative offset of all synchronizes ( ahead source )
      INT64    _minNegOffset ;
      // counts of positive offsets of all synchronizes
      UINT64   _posOffsetCount ;
      // counts of negative offsets of all synchronizes
      UINT64   _negOffsetCount ;
      // last delay
      UINT64   _lastDelay ;
      // last offset
      INT64    _lastOffset ;
      // last sync tick
      UINT64   _updateTick ;

      // history of synchronize records
      STP_SYNC_REC_LIST _histList ;
   } ;

}

#endif // STP_SYNC_STATS_HPP__
