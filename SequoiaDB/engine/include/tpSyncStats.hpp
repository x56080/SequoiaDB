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

   Source File Name = tpSyncStats.hpp

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

#ifndef TP_SYNC_STATS_HPP__
#define TP_SYNC_STATS_HPP__

#include "tpCBCommon.hpp"
#include "tpLogicalTime.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "ossUtil.hpp"
#include "msgTp.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _tpSyncRecord define
    */
   class _tpSyncRecord ;
   typedef class _tpSyncRecord tpSyncRecord ;
   typedef ossPoolList< tpSyncRecord > TP_SYNC_REC_LIST ;

   class _tpSyncRecord : public utilPooledObject
   {
   public:
      _tpSyncRecord() ;
      _tpSyncRecord( const tpSyncRecord &record ) ;
      _tpSyncRecord( const MsgTpTimeSyncRsp *response ) ;
      ~_tpSyncRecord() ;

   public:
      tpSyncRecord &operator =( const tpSyncRecord &record ) ;

   public:
      OSS_INLINE UINT64 getRequestID() const
      {
         return _requestID ;
      }

      OSS_INLINE const tpHPTime &getReqSendTime() const
      {
         return _reqSendTime ;
      }

      OSS_INLINE const tpHPTime &getReqReceiveTime() const
      {
         return _reqReceiveTime ;
      }

      OSS_INLINE const tpHPTime &getRspSendTime() const
      {
         return _rspSendTime ;
      }

      OSS_INLINE const tpHPTime &getRspReceiveTime() const
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
         return _sendSkew ;
      }

      OSS_INLINE INT64 getReceiveSkew() const
      {
         return _receiveSkew ;
      }

      OSS_INLINE INT64 getOffset() const
      {
         return _offset ;
      }

      OSS_INLINE INT64 getDelay() const
      {
         return _delay ;
      }

      BOOLEAN isValid() const ;
      ossPoolString toString() const ;

   protected:
      void _calculate() ;

   protected:
      UINT64   _requestID ;
      tpHPTime _reqSendTime ;
      tpHPTime _reqReceiveTime ;
      tpHPTime _rspSendTime ;
      tpHPTime _rspReceiveTime ;
      UINT32   _reqTimeError ;
      UINT32   _rspTimeError ;
      INT64    _sendSkew ;
      INT64    _receiveSkew ;
      INT64    _offset ;
      INT64    _delay ;
   } ;

   /*
      _tpSyncStats define
    */
   class _tpSyncStats ;
   typedef class _tpSyncStats tpSyncStats ;

   class _tpSyncStats : public utilPooledObject
   {
   public:
      _tpSyncStats() ;
      _tpSyncStats( const tpSyncStats &stats ) ;
      ~_tpSyncStats() ;

   public:
      tpSyncStats &operator =( const tpSyncStats &stats ) ;

   public:
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
      void reset() ;
      void updateSync() ;
      void updateStats( const tpSyncRecord &record, BOOLEAN isValid ) ;
      void updateStats( const tpSyncStats &stats ) ;

      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;

   protected:
      UINT64   _syncCount ;
      UINT64   _validCount ;
      UINT64   _maxDelay ;
      UINT64   _maxValidDelay ;
      UINT64   _minDelay ;
      INT64    _initOffset ;
      INT64    _maxPosOffset ;
      INT64    _maxNegOffset ;
      INT64    _maxValidPosOffset ;
      INT64    _maxValidNegOffset ;
      INT64    _minPosOffset ;
      INT64    _minNegOffset ;
   } ;

}

#endif // TP_SYNC_STATS_HPP__
