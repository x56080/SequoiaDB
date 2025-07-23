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

   Source File Name = catSequenceManager.cpp

   Descriptive Name = Sequence manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catSequenceManager.hpp"
#include "catGTSDef.hpp"
#include "catCommon.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "rtn.hpp"
#include "rtnContextBuff.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   class _catSequenceManager::_operateSequence
   {
   public:
      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence& seq,
                                _pmdEDUCB* eduCB,
                                INT16 w )
      {
         return SDB_OK ;
      }
      virtual ~_operateSequence() {}
   } ;

   class _catSequenceManager::_acquireSequence:
         public _catSequenceManager::_operateSequence
   {
   public:
      _acquireSequence( _catSequenceAcquirer &acquirer ):
            _acquirer( acquirer )
      {
      }

      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence &seq,
                                _pmdEDUCB *eduCB,
                                INT16 w ) ;
   private:
      INT32 _acquireAscendingSequence( _catSequence& seq,
                                       _catSequenceAcquirer& acquirer,
                                       BOOLEAN& needUpdate ) ;
      INT32 _acquireDescendingSequence( _catSequence& seq,
                                        _catSequenceAcquirer& acquirer,
                                        BOOLEAN& needUpdate ) ;
   private:
      _catSequenceAcquirer& _acquirer ;
   } ;

   class _catSequenceManager::_adjustSequence:
         public _catSequenceManager::_operateSequence
   {
   public:
      _adjustSequence( INT64 expectValue ) :
            _expectValue( expectValue )
      {
      }

      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence &seq,
                                _pmdEDUCB *eduCB,
                                INT16 w ) ;
   private:
      void _adjustAscendingSequence( _catSequence& seq,
                                     INT64 expectValue,
                                     BOOLEAN& needUpdate ) ;
      void _adjustDecendingSequence( _catSequence& seq,
                                     INT64 expectValue,
                                     BOOLEAN& needUpdate ) ;
   private:
      INT64 _expectValue ;
   } ;

   _catSequenceManager::_catSequenceManager()
   {
   }

   _catSequenceManager::~_catSequenceManager()
   {
      _cleanCache( FALSE ) ;
   }

   INT32 _catSequenceManager::active()
   {
      _cleanCache( FALSE ) ;
      return SDB_OK ;
   }

   INT32 _catSequenceManager::deactive()
   {
      _cleanCache( TRUE ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_CREATE_SEQ, "_catSequenceManager::createSequence" )
   INT32 _catSequenceManager::createSequence( const std::string& name,
                                              const BSONObj& options,
                                              _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      utilGlobalID ID = UTIL_GLOBAL_NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_CREATE_SEQ ) ;

      _catSequence sequence = _catSequence( name ) ;

      rc = _catSequence::validateFieldNames( options ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid options of sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if( options.hasField( CAT_SEQUENCE_CURRENT_VALUE ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Field(%s) not allowed when create sequence",
                 CAT_SEQUENCE_CURRENT_VALUE ) ;
         goto error ;
      }

      rc = sequence.setOptions( options, TRUE, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if( options.hasField( CAT_SEQUENCE_ID ) )
      {
         ele = options.getField( CAT_SEQUENCE_ID ) ;
         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                    ele.type(), CAT_SEQUENCE_OID ) ;
            goto error ;
         }
         ID = ele.Long() ;
      }
      else
      {
         rc = catUpdateGlobalID( eduCB, w, ID ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get globalID when create sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      sequence.setID( ID ) ;
      sequence.setOID( OID::gen() ) ;
      sequence.setVersion( 0 ) ;
      sequence.setInitial( TRUE ) ;

      rc = sequence.validate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = sequence.toBSONObj( obj, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      {
         CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
         BUCKET_XLOCK( bucket ) ;

         CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
         if ( bucket.end() != iter )
         {
            rc = SDB_SEQUENCE_EXIST ;
            PD_LOG( PDERROR, "Sequence %s is already existing, rc=%d", name.c_str(), rc ) ;
            goto error ;
         }

         rc = _insertSequence( obj, eduCB, w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to insert sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_CREATE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_DROP_SEQ, "_catSequenceManager::dropSequence" )
   INT32 _catSequenceManager::dropSequence( const std::string& name,
                                            _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_DROP_SEQ ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _catSequence* sequence = (*iter).second ;
         bucket.erase( name ) ;
         SDB_OSS_DEL sequence ;
      }

      rc = _deleteSequence( name, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_DROP_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_ALTER_SEQ, "_catSequenceManager::alterSequence" )
   INT32 _catSequenceManager::alterSequence( const std::string& name,
                                             const BSONObj& options,
                                             _pmdEDUCB* eduCB,
                                             INT16 w,
                                             bson::BSONObj * oldOptions,
                                             UINT32 * alterMask )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      UINT32 fieldMask = UTIL_ARG_FIELD_EMPTY ;
      _catSequence* cache = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_ALTER_SEQ ) ;

      _catSequence sequence = _catSequence( name ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         // found from cache
         cache = (*iter).second ;
         sequence.copyFrom( *cache ) ;
      }
      else
      {
         // not found from cache, acquire from collection
         BSONObj seqObj ;
         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence.setOptions( seqObj, FALSE, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      if ( NULL != oldOptions )
      {
         // safe to be copied the old values under x-lock
         rc = sequence.toBSONObj( *oldOptions, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save old BSONObj for "
                      "sequence[%s] before update, rc: %d",
                      name.c_str(), rc ) ;
      }

      // update the sequence now
      rc = sequence.setOptions( options, FALSE, FALSE, &fieldMask ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != alterMask )
      {
         // update the alter mask if needed
         *alterMask = fieldMask ;
      }

      if ( UTIL_ARG_FIELD_EMPTY == fieldMask )
      {
         // nothing changed
         goto done ;
      }

      sequence.increaseVersion() ;

      rc = sequence.validate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = sequence.toBSONObj( obj, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = _updateSequence( name, obj, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != cache )
      {
         cache->copyFrom( sequence ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_ALTER_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSequenceManager::acquireSequence( const std::string& name,
                                               const utilSequenceID ID,
                                               _catSequenceAcquirer& acquirer,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      _acquireSequence func( acquirer ) ;
      return _doOnSequence( name, ID, eduCB, w, func ) ;
   }

   INT32 _catSequenceManager::adjustSequence( const std::string &name,
                                              const utilSequenceID ID,
                                              INT64 expectValue,
                                              _pmdEDUCB *eduCB, INT16 w )
   {
      _adjustSequence func( expectValue ) ;
      return _doOnSequence( name, ID, eduCB, w, func ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_RESET_SEQ, "_catSequenceManager::resetSequence" )
   INT32 _catSequenceManager::resetSequence( const std::string& name,
                                             _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      _catSequence* cache = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_RESET_SEQ ) ;

      _catSequence sequence = _catSequence( name ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
         sequence.copyFrom( *cache ) ;
      }
      else
      {
         BSONObj seqObj ;
         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence.setOptions( seqObj, FALSE, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      sequence.setCachedValue( sequence.startValue() ) ;
      sequence.setCurrentValue( sequence.startValue() ) ;
      sequence.setInitial( TRUE ) ;
      sequence.setExceeded( FALSE ) ;
      sequence.increaseVersion() ;

      rc = sequence.toBSONObj( obj, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = _updateSequence( name, obj, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to reset sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != cache )
      {
         cache->copyFrom( sequence ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_RESET_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ, "_catSequenceManager::_acquireSequence::operator" )
   INT32 _catSequenceManager::_acquireSequence::operator()( _catSequenceManager *pMgr,
                                                            _catSequence& seq,
                                                            _pmdEDUCB* eduCB,
                                                            INT16 w )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needUpdate = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ ) ;

      if ( seq.increment() > 0 )
      {
         rc = _acquireAscendingSequence( seq, _acquirer, needUpdate ) ;
      }
      else
      {
         rc = _acquireDescendingSequence( seq, _acquirer, needUpdate );
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to acquire sequence[%s], rc=%d",
                 seq.name().c_str(), rc ) ;
         goto error ;
      }

      _acquirer.ID = seq.ID() ;

      if ( needUpdate )
      {
         BSONObj options ;
         try
         {
            options = BSON( CAT_SEQUENCE_CURRENT_VALUE << seq.currentValue()
                         << CAT_SEQUENCE_INITIAL << (bool) seq.initial() ) ;
         }
         catch( std::exception& e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                    e.what(), rc ) ;
            goto error ;
         }

         rc = pMgr->_updateSequence( seq.name(), options, eduCB, w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                    seq.name().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ, "_catSequenceManager::_acquireSequence::_acquireAscendingSequence" )
   INT32 _catSequenceManager::_acquireSequence::_acquireAscendingSequence(
         _catSequence& seq,
         _catSequenceAcquirer& acquirer,
         BOOLEAN& needUpdate )
   {
      INT32 rc = SDB_OK ;
      INT64 nextValue = 0 ;
      INT64 fetchInc = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ ) ;

      SDB_ASSERT( seq.increment() > 0, "increment should be > 0" ) ;

      needUpdate = FALSE ;

      if ( seq.initial() )
      {
         nextValue = seq.startValue() ;
         seq.setInitial( FALSE ) ;
         needUpdate = TRUE ;
      }
      else if ( seq.exceeded() )
      {
         // reach the maximum limit
         if ( !seq.cycled() )
         {
            rc = SDB_SEQUENCE_EXCEEDED ;
            PD_LOG( PDERROR, "Sequence[%s] value(%lld) is reach the maximum value(%lld)",
                    seq.name().c_str(), seq.currentValue(), seq.maxValue() ) ;
            goto error ;
         }
         else
         {
            // restart from minValue
            nextValue = seq.minValue() ;
            seq.setCurrentValue( nextValue ) ;
            seq.setCachedValue( nextValue ) ;
            seq.setExceeded( FALSE ) ;
            needUpdate = TRUE ;
         }
      }
      else
      {
         // safe to increase value
         nextValue = seq.cachedValue() + seq.increment() ;
      }

      fetchInc = ( seq.acquireSize() - 1 ) * (INT64) seq.increment() ;

      // use minus to avoid overflow
      if ( seq.currentValue() - nextValue >= fetchInc )
      {
         seq.setCachedValue( nextValue + fetchInc ) ;
         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = seq.acquireSize() ;
         acquirer.increment = seq.increment() ;
      }
      else
      {
         INT64 cachedInc = seq.cacheSize() * (INT64) seq.increment() ;
         if ( seq.currentValue() <= seq.maxValue() - cachedInc )
         {
            seq.setCachedValue( nextValue + fetchInc ) ;
            seq.setCurrentValue( seq.currentValue() + cachedInc ) ;
         }
         else
         {
            INT64 newCurrentValue = seq.currentValue() +
               ( seq.maxValue() - seq.currentValue() ) / seq.increment() * seq.increment() ;
            seq.setCurrentValue( newCurrentValue ) ;
            // use minus to avoid overflow
            if ( seq.currentValue() - nextValue >= fetchInc )
            {
               seq.setCachedValue( nextValue + fetchInc ) ;
            }
            else
            {
               INT64 newCachedValue = seq.cachedValue() +
                  ( seq.currentValue() - seq.cachedValue() ) / seq.increment() * seq.increment() ;
               seq.setCachedValue( newCachedValue ) ;
            }
         }

         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = ( seq.cachedValue() - nextValue ) / seq.increment() + 1 ;
         acquirer.increment = seq.increment() ;
         needUpdate = TRUE ;
      }

      // reach the maxValue, so mark exceeded
      if ( seq.cachedValue() == seq.maxValue() ||
           seq.cachedValue() > seq.maxValue() - seq.increment() )
      {
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ, "_catSequenceManager::_acquireSequence::_acquireDescendingSequence" )
   INT32 _catSequenceManager::_acquireSequence::_acquireDescendingSequence(
         _catSequence& seq,
         _catSequenceAcquirer& acquirer,
         BOOLEAN& needUpdate )
   {
      INT32 rc = SDB_OK ;
      INT64 nextValue = 0 ;
      INT64 fetchInc = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ ) ;

      SDB_ASSERT( seq.increment() < 0, "increment should be < 0" ) ;

      needUpdate = FALSE ;

      if ( seq.initial() )
      {
         nextValue = seq.startValue() ;
         seq.setInitial( FALSE ) ;
         needUpdate = TRUE ;
      }
      else if ( seq.exceeded() )
      {
         // reach the minimum limit
         if ( !seq.cycled() )
         {
            rc = SDB_SEQUENCE_EXCEEDED ;
            PD_LOG( PDERROR, "Sequence[%s] value(%lld) is reach the minimum value(%lld)",
                    seq.name().c_str(), seq.currentValue(), seq.minValue() ) ;
            goto error ;
         }
         else
         {
            // restart from maxValue
            nextValue = seq.maxValue() ;
            seq.setCurrentValue( nextValue ) ;
            seq.setCachedValue( nextValue ) ;
            seq.setExceeded( FALSE ) ;
            needUpdate = TRUE ;
         }
      }
      else
      {
         // safe to decrease value
         nextValue = seq.cachedValue() + seq.increment() ;
      }

      fetchInc = ( seq.acquireSize() - 1 ) * (INT64) seq.increment() ;

      // use minus to avoid overflow
      if ( nextValue - seq.currentValue() >= -fetchInc )
      {
         seq.setCachedValue( nextValue + fetchInc ) ;
         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = seq.acquireSize() ;
         acquirer.increment = seq.increment() ;
      }
      else
      {
         INT64 cachedInc = seq.cacheSize() * (INT64) seq.increment() ;
         if ( seq.currentValue() >= seq.minValue() - cachedInc )
         {
            seq.setCachedValue( nextValue + fetchInc ) ;
            seq.setCurrentValue( seq.currentValue() + cachedInc ) ;
         }
         else
         {
            INT64 newCurrentValue = seq.currentValue() +
               ( seq.minValue() - seq.currentValue() ) / seq.increment() * seq.increment() ;
            seq.setCurrentValue( newCurrentValue ) ;
            // use minus to avoid overflow
            if ( nextValue - seq.currentValue() >= -fetchInc )
            {
               seq.setCachedValue( nextValue + fetchInc ) ;
            }
            else
            {
               INT64 newCachedValue = seq.cachedValue() +
                  ( seq.currentValue() - seq.cachedValue() ) / seq.increment() * seq.increment() ;
               seq.setCachedValue( newCachedValue ) ;
            }
         }

         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = ( seq.cachedValue() - nextValue ) / seq.increment() + 1 ;
         acquirer.increment = seq.increment() ;
         needUpdate = TRUE ;
      }

      // reach the minValue, so mark exceeded
      if ( seq.cachedValue() == seq.minValue() ||
           seq.cachedValue() < seq.minValue() - seq.increment() )
      {
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE, "_catSequenceManager::_adjustSequence::operator" )
   INT32 _catSequenceManager::_adjustSequence::operator()(
         _catSequenceManager *pMgr,
         _catSequence& seq,
         _pmdEDUCB* eduCB,
         INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE ) ;

      BOOLEAN needUpdate = FALSE ;
      if ( seq.increment() > 0 )
      {
         _adjustAscendingSequence( seq, _expectValue, needUpdate ) ;
      }
      else
      {
         _adjustDecendingSequence( seq, _expectValue, needUpdate ) ;
      }

      if ( needUpdate )
      {
         BSONObj options ;
         try
         {
            options = BSON( CAT_SEQUENCE_CURRENT_VALUE << seq.currentValue()
                         << CAT_SEQUENCE_INITIAL << (bool) seq.initial() ) ;
         }
         catch( std::exception& e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                    e.what(), rc ) ;
            goto error ;
         }

         rc = pMgr->_updateSequence( seq.name(), options, eduCB, w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                    seq.name().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ, "_catSequenceManager::_adjustSequence::_adjustAscendingSequence" )
   void _catSequenceManager::_adjustSequence::_adjustAscendingSequence(
         _catSequence &seq,
         INT64 expectValue,
         BOOLEAN &needUpdate )
   {
      /*
         adjust rule:
         if expectValue >= maxValue
            mark sequence as exceed
         if maxValue > expectValue >= currentValue
            adjust currentValue to be greater than expectValue
         if currentValue > expectValue >= nextValue
            adjust nextValue to be greater than expectValue
         if nextValue > expectValue
            ignore
       */
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ ) ;

      UINT64 diff = 0 ;
      UINT64 reminder = 0 ;
      UINT64 diffInc = 0 ;
      INT64 newCachedValue = 0 ;
      INT64 newNextValue = 0 ;

      // When sequence is initial, cachedValue is inexistent.
      // We can ignore it, because when the conflict occurs, coord will retry.
      // At that time, we can handle it.
      if ( seq.initial() || seq.exceeded() ||
           seq.cachedValue() + seq.increment() > expectValue )
      {
         goto done ;
      }

      diff = ( UINT64 )( expectValue - seq.cachedValue() ) ;
      reminder = diff % seq.increment() ;
      diffInc = reminder == 0 ? diff : (diff - reminder + seq.increment()) ;
      newCachedValue = seq.cachedValue() + diffInc ;
      if ( diffInc < diff || // check for diffInc overflow
           newCachedValue < seq.cachedValue() || // check for newCachedValue overflow
           newCachedValue > seq.maxValue() - seq.increment() )
      {
         seq.setCachedValue( seq.maxValue() ) ;
         seq.setCurrentValue( seq.maxValue() ) ;
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
         goto done ;
      }

      newNextValue = newCachedValue + seq.increment() ;
      if ( newNextValue <= seq.currentValue() )
      {
         seq.setCachedValue( newCachedValue ) ;
      }
      else
      {
         seq.setCachedValue( newCachedValue ) ;
         seq.setCurrentValue( newNextValue ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXIT ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ, "_catSequenceManager::_adjustSequence::_adjustDecendingSequence" )
   void _catSequenceManager::_adjustSequence::_adjustDecendingSequence(
         _catSequence& seq,
         INT64 expectValue,
         BOOLEAN& needUpdate )
   {
      /*
         adjust rule:
         if expectValue <= minValue
            mark sequence as exceed
         if minValue < expectValue <= currentValue
            adjust currentValue to be less than expectValue
         if currentValue < expectValue <= nextValue
            adjust nextValue to be less than expectValue
         if nextValue < expectValue
            ignore
       */
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ ) ;

      UINT64 diff = 0 ;
      UINT64 reminder = 0 ;
      UINT64 diffInc = 0 ;
      INT64 newCachedValue = 0 ;
      INT64 newNextValue = 0 ;

      // When sequence is initial, cachedValue is inexistent.
      // We can ignore it, because when the conflict occurs, coord will retry.
      // At that time, we can handle it.
      if ( seq.initial() || seq.exceeded() ||
           seq.cachedValue() + seq.increment() < expectValue )
      {
         goto done ;
      }

      diff = ( UINT64 )( seq.cachedValue() - expectValue ) ;
      reminder = diff % (-seq.increment()) ;
      diffInc = reminder == 0 ? diff : (diff - reminder - seq.increment()) ;
      newCachedValue = seq.cachedValue() - diffInc ;
      if ( diffInc < diff || // check for diffInc overflow
           newCachedValue > seq.cachedValue() || // check for newCachedValue overflow
           newCachedValue < seq.minValue() - seq.increment() )
      {
         seq.setCachedValue( seq.minValue() ) ;
         seq.setCurrentValue( seq.minValue() ) ;
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
         goto done ;
      }

      newNextValue = newCachedValue + seq.increment() ;
      if ( newNextValue >= seq.currentValue() )
      {
         seq.setCachedValue( newCachedValue ) ;
      }
      else
      {
         seq.setCachedValue( newCachedValue ) ;
         seq.setCurrentValue( newNextValue ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXIT ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE, "_catSequenceManager::_doOnSequence" )
   INT32 _catSequenceManager::_doOnSequence( const std::string& name,
                                             const utilSequenceID ID,
                                             _pmdEDUCB* eduCB,
                                             INT16 w,
                                             _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN noCache = FALSE ;
      utilSequenceID cachedID = UTIL_SEQUENCEID_NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE ) ;

      rc = _doOnSequenceBySLock( name, ID, eduCB, w, func, noCache, cachedID ) ;
      if ( SDB_OK == rc )
      {
         // if no sequence cache, should get bucket by XLock
         if ( noCache )
         {
            rc = _doOnSequenceByXLock( name, ID, eduCB, w, func ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }
      else
      {
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            // remove cache if sequence not exist
            _removeCacheByID( name, cachedID ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK, "_catSequenceManager::_doOnSequenceBySLock" )
   INT32 _catSequenceManager::_doOnSequenceBySLock( const std::string& name,
                                                    const utilSequenceID ID,
                                                    _pmdEDUCB* eduCB,
                                                    INT16 w,
                                                    _operateSequence& func,
                                                    BOOLEAN& noCache,
                                                    utilSequenceID &cachedSeqID )
   {
      INT32 rc = SDB_OK ;
      _catSequence* cache = NULL ;
      BOOLEAN cacheLocked = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_SLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
         noCache = FALSE ;
      }
      else
      {
         // no sequence cache, should get bucket by XLock
         noCache = TRUE ;
         goto done ;
      }

      cache->lock() ;
      cacheLocked = TRUE ;

      // check ID
      if ( ID != UTIL_SEQUENCEID_NULL && ID != cache->ID() )
      {
         PD_LOG( PDERROR, "Mismatch ID(%llu) for sequence[%s, %llu]",
                 ID, cache->name().c_str(), cache->ID() ) ;
         rc = SDB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      rc = func( this, *cache, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to process sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         // if rc==SDB_SEQUENCE_NOT_EXIST, we should delete the cache
         // here we get SLOCK, so we need to get XLOCK to delete the cache
         // check the ID consistency when delete cache
         cachedSeqID = cache->ID() ;
         goto error ;
      }

   done:
      if ( cacheLocked )
      {
         cache->unlock() ;
         cacheLocked = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK, "_catSequenceManager::_doOnSequenceByXLock" )
   INT32 _catSequenceManager::_doOnSequenceByXLock( const std::string& name,
                                                   const utilSequenceID ID,
                                                   _pmdEDUCB* eduCB,
                                                   INT16 w,
                                                   _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      _catSequence* cache = NULL ;
      _catSequence* sequence = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
      }
      else
      {
         BSONObj seqObj ;

         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         sequence = SDB_OSS_NEW _catSequence( name ) ;
         if ( NULL == sequence )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence->setOptions( seqObj, FALSE, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         try
         {
            bucket.insert( CAT_SEQ_MAP::value_type( name, sequence ) ) ;
         }
         catch( std::exception& e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to insert sequence[%s] to cache", name.c_str() ) ;
            goto error ;
         }

         cache = sequence ;
         sequence = NULL ;
      }

      // check id
      if ( ID != UTIL_SEQUENCEID_NULL && ID != cache->ID() )
      {
         PD_LOG( PDERROR, "Mismatch ID(%llu) for sequence[%s, %llu]",
                 ID, cache->name().c_str(), cache->ID() ) ;
         rc = SDB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      rc = func( this, *cache, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to process sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            // remove cache if sequence not exist
            // we have got XLOCK, so we can delete it directly
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            cache = NULL ;
         }
         goto error ;
      }

   done:
      SAFE_OSS_DELETE( sequence ) ;
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__INSERT_SEQ, "_catSequenceManager::_insertSequence" )
   INT32 _catSequenceManager::_insertSequence( BSONObj& sequence,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__INSERT_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;

      rc = rtnInsert( GTS_SEQUENCE_COLLECTION_NAME,
                      sequence, 1, 0, eduCB,
                      dmsCB, dpsCB, w ) ;
      if ( SDB_OK != rc && SDB_IXM_DUP_KEY != rc )
      {
         BSONObj hint ;
         rtnDelete( GTS_SEQUENCE_COLLECTION_NAME,
                    sequence, hint, 0, eduCB,
                    dmsCB, dpsCB ) ;
         goto error ;
      }
      else if ( SDB_IXM_DUP_KEY == rc )
      {
         rc = SDB_SEQUENCE_EXIST ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__INSERT_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DELETE_SEQ, "_catSequenceManager::_deleteSequence" )
   INT32 _catSequenceManager::_deleteSequence( const std::string& name,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      INT64 deleteNum = 0 ;
      BSONObj hint ;
      BSONObj matcher ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DELETE_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;

      try
      {
         matcher = BSON( CAT_SEQUENCE_NAME << name ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build delete matcher for sequence[%s], exception=%s",
                 name.c_str(), e.what() ) ;
         goto error ;
      }

      rc = rtnDelete( GTS_SEQUENCE_COLLECTION_NAME,
                      matcher, hint, 0, eduCB,
                      dmsCB, dpsCB, w, &deleteNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( 0 == deleteNum )
      {
         rc = SDB_SEQUENCE_NOT_EXIST ;
         PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }
      else if ( deleteNum > 1 )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected delete num[%lld] for sequence[%s], rc=%d",
                 deleteNum, name.c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DELETE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__UPDATE_SEQ, "_catSequenceManager::_updateSequence" )
   INT32 _catSequenceManager::_updateSequence( const std::string& name,
                                               const BSONObj& options,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj updator ;
      BSONObj hint ;
      INT64 updateNum = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__UPDATE_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;

      SDB_ASSERT( !options.hasField( CAT_SEQUENCE_NAME ), "can't have name" ) ;

      try
      {
         matcher = BSON( CAT_SEQUENCE_NAME << name ) ;
         updator = BSON( "$set" << options ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build matcher or updator for sequence[%s], exception=%s",
                 name.c_str(), e.what() ) ;
         goto error ;
      }

      rc = rtnUpdate( GTS_SEQUENCE_COLLECTION_NAME,
                      matcher, updator, hint, 0, eduCB,
                      dmsCB, dpsCB, w, &updateNum ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( 0 == updateNum )
      {
         rc = SDB_SEQUENCE_NOT_EXIST ;
         PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }
      else if ( updateNum > 1 )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected update num[%lld] for sequence[%s], rc=%d",
                 updateNum, name.c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__UPDATE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__FIND_SEQ, "_catSequenceManager::_findSequence" )
   INT32 _catSequenceManager::_findSequence( const std::string& name,
                                             BSONObj& sequence,
                                             _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__FIND_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;

      try
      {
         matcher = BSON( CAT_SEQUENCE_NAME << name ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build update matcher for sequence[%s], exception=%s",
                 name.c_str(), e.what() ) ;
         goto error ;
      }

      rc = rtnQuery( GTS_SEQUENCE_COLLECTION_NAME,
                     selector, matcher, orderBy, hint, 0, eduCB, 0, 1,
                     dmsCB, rtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore ( contextID, 1, buffObj, eduCB, rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "Failed to get sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            sequence = result.copy() ;
            isExist = TRUE ;
            break ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_SEQUENCE_NOT_EXIST ;
         PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, eduCB ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__FIND_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID, "_catSequenceManager::_removeCacheByID" )
   BOOLEAN _catSequenceManager::_removeCacheByID( const std::string& name, utilSequenceID ID )
   {
      BOOLEAN removed = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _catSequence* cache = (*iter).second ;
         // if id not equal, means the cache has been changed, can't delete
         if ( cache->ID() == ID )
         {
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            removed = TRUE ;
         }
      }

      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID, removed ) ;
      return removed ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__CLEAN_CACHE, "_catSequenceManager::_cleanCache" )
   void _catSequenceManager::_cleanCache( BOOLEAN needFlush )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__CLEAN_CACHE ) ;

      pmdEDUCB* eduCB = pmdGetThreadEDUCB() ;

      for ( CAT_SEQ_MAP::bucket_iterator bucketIt = _sequenceCache.begin() ;
            bucketIt != _sequenceCache.end() ;
            bucketIt++ )
      {
         CAT_SEQ_MAP::Bucket& bucket = *bucketIt ;
         BUCKET_XLOCK( bucket ) ;

         for ( CAT_SEQ_MAP::map_const_iterator it = bucket.begin() ;
               it != bucket.end() ;
               it++ )
         {
            _catSequence* cache = (*it).second ;
            BSONObj options ;
            rc = SDB_OK ;

            if ( needFlush )
            {
               try
               {
                  options = BSON( CAT_SEQUENCE_CURRENT_VALUE << cache->cachedValue()
                               << CAT_SEQUENCE_INITIAL << (bool) cache->initial() ) ;

                  rc = _updateSequence( cache->name(), options, eduCB, 1 ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDWARNING, "Failed to flush sequence[%s], rc=%d",
                             cache->name().c_str(), rc ) ;
                  }
               }
               catch( std::exception& e )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDWARNING, "Failed to flush sequence[%s], exception: %s, rc=%d",
                          cache->name().c_str(), e.what(), rc ) ;
               }
            }

            SDB_OSS_DEL ( cache ) ;
         }

         bucket.clear() ;
      }

      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__CLEAN_CACHE, rc ) ;
   }
}

