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

   Source File Name = monDps.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MON_DPS_HPP__
#define MON_DPS_HPP__

#include "dpsDef.hpp"
#include "dpsTransLockDef.hpp"
#include "msgDef.hpp"
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{

   #define MON_TRANS_LOCK_MODE            "Mode"
   #define MON_TRANS_LOCK_COUNT           "Count"

   #define MON_TRANS_HOLDER               "Holder"
   #define MON_TRANS_WAITER               "Waiter"

   #define MON_TRANS_DURATION             "Duration"

   /*
      _monTransLockCur define
   */
   class _monTransLockCur : public SDBObject
   {
      public:
         _monTransLockCur()
         {
            _mode       = 0 ;
            _count      = 0 ;
            _beginTick.clear() ;
         }
         ~_monTransLockCur()
         {
         }

         void clear()
         {
            _mode       = 0 ;
            _count      = 0 ;
            _id.reset() ;
            _beginTick.clear() ;
         }

         OSS_INLINE BSONObj   toBson( BOOLEAN showCount = TRUE ) const ;
         OSS_INLINE void      toBson( BSONObjBuilder &builder,
                                      BOOLEAN showCount = TRUE ) const ;

      public:
         dpsTransLockId       _id ;
         DPS_TRANSLOCK_TYPE   _mode ;
         UINT32               _count ;
         ossTick              _beginTick ;
   } ;
   typedef _monTransLockCur                     monTransLockCur ;
   typedef vector< monTransLockCur >            VEC_TRANSLOCKCUR ;

   static UINT64 calDurationInMicroseconds( const ossTick beginTick )
   {
      UINT64 durationInMicroseconds = 0 ;
      UINT32 seconds = 0, microseconds = 0 ;
      ossTickConversionFactor factor ;
      ossTick endTick ;
      ossTickDelta delta ;

      endTick.sample() ;
      delta = endTick - beginTick ;
      delta.convertToTime( factor, seconds, microseconds ) ;
      durationInMicroseconds = (UINT64)( seconds * 1000 + microseconds / 1000 );

      return durationInMicroseconds ;
   }

   /*
      _monTransLockCur implement
   */
   OSS_INLINE void _monTransLockCur::toBson( BSONObjBuilder &builder,
                                             BOOLEAN showCount ) const
   {
      _id.toBson( builder ) ;

      builder.append( MON_TRANS_LOCK_MODE, lockModeToString( _mode ) ) ;

      if ( showCount )
      {
         builder.append( MON_TRANS_LOCK_COUNT, (INT32)_count ) ;
      }

      UINT64 duration = calDurationInMicroseconds( _beginTick ) ;
      builder.append( MON_TRANS_DURATION, (INT64)duration ) ;
   }

   OSS_INLINE BSONObj _monTransLockCur::toBson( BOOLEAN showCount ) const
   {
      BSONObjBuilder builder( 128 ) ;
      toBson( builder, showCount ) ;
      return builder.obj() ;
   }

   /*
      _monTransLockInfo define
   */
   class _monTransLockInfo : public SDBObject
   {
      public:
         /*
            _lockItem define
         */
         class _lockItem
         {
            public:
               _lockItem()
               {
                  _eduID = 0 ;
                  _mode  = 0 ;
                  _count = 0 ;
                  _beginTick.clear() ;
               }
               ~_lockItem() {}

               void toBson( BSONObjBuilder &builder, BOOLEAN showCount ) const
               {
                  builder.append( FIELD_NAME_SESSIONID, (INT64)_eduID ) ;
                  builder.append( MON_TRANS_LOCK_MODE,
                                  lockModeToString( _mode ) ) ;
                  if ( showCount )
                  {
                     builder.append( MON_TRANS_LOCK_COUNT, (INT32)_count ) ;
                  }

                  UINT64 duration = calDurationInMicroseconds( _beginTick ) ;
                  builder.append( MON_TRANS_DURATION, (INT64)duration ) ;
               }

            public:
               EDUID                   _eduID ;
               DPS_TRANSLOCK_TYPE      _mode ;
               UINT32                  _count ;
               ossTick                 _beginTick ;
         } ;
         typedef _lockItem lockItem ;
         typedef vector< lockItem >          VEC_LOCKITEM ;

      public:
         _monTransLockInfo() {}
         ~_monTransLockInfo() {}

         OSS_INLINE BSONObj   toBson() const ;
         OSS_INLINE void      toBson( BSONObjBuilder &builder ) const ;

      public:
         dpsTransLockId             _id ;
         VEC_LOCKITEM               _vecHolder ;
         VEC_LOCKITEM               _vecWaiter ;
   } ;
   typedef _monTransLockInfo              monTransLockInfo ;
   typedef vector< monTransLockInfo >     VEC_TRANSLOCKINFO ;

   /*
      monTransLockInfo implement
   */
   OSS_INLINE void _monTransLockInfo::toBson( BSONObjBuilder &builder ) const
   {
      VEC_LOCKITEM::const_iterator cit ;

      /// id
      BSONObjBuilder idBuilder( builder.subobjStart( FIELD_NAME_ID ) ) ;
      _id.toBson( idBuilder ) ;
      idBuilder.done() ;

      /// holder
      BSONArrayBuilder holderBuilder(
         builder.subarrayStart( MON_TRANS_HOLDER ) ) ;
      cit = _vecHolder.begin() ;
      while( cit != _vecHolder.end() )
      {
         BSONObjBuilder itemBuilder( holderBuilder.subobjStart() ) ;
         /// show count
         (*cit).toBson( itemBuilder, TRUE ) ;
         itemBuilder.done() ;

         ++cit ;
      }
      holderBuilder.done() ;

      /// waiter
      BSONArrayBuilder waiterBuilder(
         builder.subarrayStart( MON_TRANS_WAITER ) ) ;
      cit = _vecWaiter.begin() ;
      while( cit != _vecWaiter.end() )
      {
         BSONObjBuilder itemBuilder( holderBuilder.subobjStart() ) ;
         /// don't show count
         (*cit).toBson( itemBuilder, FALSE ) ;
         itemBuilder.done() ;

         ++cit ;
      }
      waiterBuilder.done() ;
   }

   OSS_INLINE BSONObj _monTransLockInfo::toBson() const
   {
      BSONObjBuilder builder( 512 ) ;
      toBson( builder ) ;
      return builder.obj() ;
   }

   /*
      _monTransInfo define
   */
   class _monTransInfo : public SDBObject
   {
      public:
         DPS_TRANS_ID         _transID ;
         DPS_LSN_OFFSET       _curTransLsn ;
         UINT64               _eduID ;
         UINT64               _relatedNID ;
         UINT64               _relatedEDUID ;
         UINT32               _relatedTID ;
         UINT32               _locksNum ;
         monTransLockCur      _waitLock ;
         VEC_TRANSLOCKCUR     _lockList ;

         UINT64               _usedLogSpace ;
         UINT64               _reservedLogSpace ;
         BOOLEAN              _lockEscalated ;

      public:
         _monTransInfo()
         {
            clear() ;
         }

         void clear()
         {
            _transID = DPS_INVALID_TRANS_ID ;
            _curTransLsn = DPS_INVALID_LSN_OFFSET ;
            _eduID = 0 ;
            _relatedNID = 0 ;
            _relatedEDUID = PMD_INVALID_EDUID ;
            _relatedTID = 0 ;
            _locksNum = 0 ;
            _lockList.clear() ;
            _waitLock.clear() ;
            _usedLogSpace = 0 ;
            _reservedLogSpace = 0 ;
            _lockEscalated = FALSE ;
         }
   } ;
   typedef _monTransInfo monTransInfo ;

}

#endif // MON_DPS_HPP__

