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

   Source File Name = pmdFGMgr.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/06/2020  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_FT_MGR_HPP
#define PMD_FT_MGR_HPP

#include "core.hpp"
#include "oss.hpp"
#include "pmdDef.hpp"
#include "ossMemPool.hpp"
#include "ossLatch.hpp"

namespace engine
{

   /*
      _ftSampleItem define
   */
   struct _ftSampleItem
   {
      volatile INT32    _count ;

      _ftSampleItem()
      {
         reset() ;
      }

      void reset()
      {
         _count = 0 ;
      }

      void inc()
      {
         ++_count ;
      }
   } ;
   typedef _ftSampleItem ftSampleItem ;

   /*
      _ftSampleSysItem define
   */
   struct _ftSampleSysItem
   {
      UINT64            _expectLsn ;
      UINT64            _completeLsn ;
      UINT32            _lsnQueSize ;
      UINT64            _primaryLsn ;
      UINT64            _lsnDiff ;
      UINT64            _diskWriteCostTime ;   // ms

      _ftSampleSysItem()
      {
         reset() ;
      }

      void reset()
      {
         _expectLsn = 0 ;
         _completeLsn = 0 ;
         _lsnQueSize = 0 ;
         _primaryLsn = 0 ;
         _lsnDiff = 0 ;
         _diskWriteCostTime = 0 ;
      }
   } ;
   typedef _ftSampleSysItem ftSampleSysItem ;

   /*
      _ftSampleWndItem define
   */
   struct _ftSampleWndItem
   {
      friend class _ftSampleWindow ;

      ftSampleItem      _err[ FT_ERR_MAX ] ;
      ftSampleItem      _risk[ FT_RISK_MAX ] ;
      ftSampleSysItem   _sys ;
      UINT64            _time ;

      _ftSampleWndItem()
      {
         _time = 0 ;
         _pos = 0 ;
      }

      void reset( UINT64 time )
      {
         for ( INT32 i = 0 ; i < FT_ERR_MAX ; ++i )
         {
            _err[i].reset() ;
         }
         for ( INT32 i = 0 ; i < FT_RISK_MAX ; ++i )
         {
            _risk[i].reset() ;
         }
         _sys.reset() ;
         _time = time ;
      }

      void clean()
      {
         for ( INT32 i = 0 ; i < FT_ERR_MAX ; ++i )
         {
            _err[i].reset() ;
         }
         for ( INT32 i = 0 ; i < FT_RISK_MAX ; ++i )
         {
            _risk[i].reset() ;
         }
      }

      UINT32 getPos() const
      {
         return _pos ;
      }

      _ftSampleWndItem& operator+= ( const _ftSampleWndItem &rhs )
      {
         for ( INT32 i = 0 ; i < FT_ERR_MAX ; ++i )
         {
            _err[ i ]._count += rhs._err[ i ]._count ;
         }

         for ( INT32 i = 0 ; i < FT_RISK_MAX ; ++i )
         {
            _risk[ i ]._count += rhs._risk[ i ]._count ;
         }

         return *this ;
      }

      UINT64   allErr() const
      {
         UINT64 count = 0 ;

         for ( INT32 i = FT_ERR_NONE + 1 ; i < FT_ERR_MAX ; ++i )
         {
            count += _err[i]._count ;
         }

         return count ;
      }

   private:
      UINT32            _pos ;
   } ;
   typedef _ftSampleWndItem ftSampleWndItem ;

   /*
      _ftSampleWindow define
   */
   class _ftSampleWindow
   {
      public:
         _ftSampleWindow() ;
         ~_ftSampleWindow() ;

         void                 clean() ;

      public:
         ftSampleWndItem*     first() ;
         ftSampleWndItem*     current() ;
         ftSampleWndItem*     prev( UINT32 curPos, UINT32 step = 1 ) ;
         ftSampleWndItem*     next( UINT32 curPos, UINT32 step = 1 ) ;

         BOOLEAN              isEmpty() const ;
         BOOLEAN              isFull() const ;
         UINT32               getCount() const ;

         ftSampleWndItem*     slideForward( UINT64 dbTick ) ;

         /*
            Report
         */
         BOOLEAN              reportErr( PMD_FT_ERR_TYPE err ) ;
         BOOLEAN              reportRisk( PMD_FT_RISK_TYPE risk ) ;

      protected:
         BOOLEAN              isEmpty( UINT32 curPos ) const ;
         BOOLEAN              isFull( UINT32 curPos ) const ;

      private:
         ftSampleWndItem   _window[ PMD_FT_SAMPLE_WINDOW_SZ ] ;
         UINT32            _curPos ;
   } ;
   typedef _ftSampleWindow ftSampleWindow ;

   /*
      _ftShieldItem define
   */
   struct _ftShieldItem
   {
      UINT32   _shieldMask ;
      INT32    _shieldTime ;
      UINT64   _dbTick ;

      _ftShieldItem()
      {
         _shieldMask = 0 ;
         _shieldTime = 0 ;
         _dbTick = 0 ;
      }

      bool operator== ( const _ftShieldItem &right ) const
      {
         if ( _shieldMask == right._shieldMask &&
              _shieldTime == right._shieldTime &&
              _dbTick == right._dbTick )
         {
            return TRUE ;
         }
         return FALSE ;
      }

      BOOLEAN isInShield( UINT32 mask, UINT64 dbTick ) const ;
   } ;
   typedef _ftShieldItem ftShieldItem ;

   /*
      _pmdFTMgr define
   */
   class _pmdFTMgr : public SDBObject
   {
      typedef ossPoolList< ftShieldItem >       LIST_SHIELD_ITEM ;

      public:
         _pmdFTMgr() ;
         ~_pmdFTMgr() ;

         /*
            confirmPeriod: unit( second )
         */
         INT32    init( UINT32 ftmask,
                        UINT32 confirmPeriod,
                        UINT32 confirmRatio,
                        INT32 ftLevel ) ;
         void     fini() ;

         void     run() ;

         void     setMask( UINT32 ftmask ) ;
         void     setConfirmPeriod( UINT32 confirmPeriod ) ;
         void     setConfrimRatio( UINT32 confirmRatio ) ;
         void     setFTLevel( INT32 ftLevel ) ;
         void     setSlowNodeInfo( UINT32 threshold, UINT32 increment ) ;
         void     setDiskDetectInfo( UINT64 threshold, UINT64 increment ) ;

         INT32    getFTLevel() const { return _ftLevel ; }

         UINT32   getConfirmedStat() const
         {
            return ( _confirmedStat | _heldMask ) ;
         }

         INT32    getIndoubtErr() const { return _indoubtErr ; }

         BOOLEAN  isCatchup() const { return _isCatchup ; }
         BOOLEAN  isStop() const ;

         void     reportErr( INT32 err, BOOLEAN isWrite ) ;
         void     reportErr( PMD_FT_ERR_TYPE errType ) ;

         void     holdStatus( UINT32 status ) ;
         void     unholdStatus( UINT32 status ) ;

         BOOLEAN  registerShield( const ftShieldItem &shieldItem ) ;
         BOOLEAN  unregShield( const ftShieldItem &shieldItem ) ;

      protected:
         ftSampleWndItem*  _sample( UINT64 dbTick ) ;
         UINT32            _confirm( ftSampleWndItem *current ) ;

         UINT64            _sumPrevnLsnDiff( ftSampleWndItem *pItem,
                                             UINT32 count ) ;

         UINT64            _sumPrevnDiskWriteCostTime( ftSampleWndItem *pItem,
                                                       UINT32 count ) ;

         BOOLEAN           _isInShield( UINT32 mask, UINT64 dbTick ) ;

      private:
         ftSampleWindow _sampleWnd ;
         UINT64         _lastSampleTick ;
         UINT64         _lastSucCount ;
         UINT64         _lastSucPageCount ;
         UINT64         _lastTransSucCount ;
         UINT64         _lastTransErrCount ;
         UINT32         _confirmPeriod ;  /// Unit(s)
         UINT32         _ftMask ;
         UINT32         _confirmRatio ;   /// %
         INT32          _ftLevel ;
         UINT64         _slowNodeThreshold ;
         UINT64         _slowNodeIncrement ;

         BOOLEAN        _isCatchup ;

         UINT32         _confirmedStat ;
         UINT32         _heldMask ;
         INT32          _indoubtErr ;

         LIST_SHIELD_ITEM     _lstShield ;
         ossSpinSLatch        _shieldLatch ;

         UINT64         _diskSlowThreshold ;
         UINT64         _diskSlowIncrement ;
   } ;
   typedef _pmdFTMgr pmdFTMgr ;

   /*
      Tool functions
   */
   void  ftReportErr( INT32 err, BOOLEAN isWrite = TRUE ) ;
   void  ftReportErr( PMD_FT_ERR_TYPE errType ) ;

   /*
      _pmdFTShield define
   */
   class _pmdFTShield
   {
      public:
         /*
            @ shieldTime: ms, 0 for no shield, < 0 for always shield
         */
         _pmdFTShield( INT32 shieldTime,
                       UINT32 shieldMask = PMD_FT_MASK_DEADSYNC | PMD_FT_MASK_SLOWNODE,
                       BOOLEAN shieldImmediately = TRUE ) ;
         ~_pmdFTShield() ;

         BOOLEAN shield() ;
         void    unShield() ;
         BOOLEAN isShield() const { return _hasReg ; }

      private:
         ftShieldItem      _item ;
         BOOLEAN           _hasReg ;
   } ;
   typedef _pmdFTShield pmdFTShield ;

}

#endif //PMD_FT_MGR_HPP

