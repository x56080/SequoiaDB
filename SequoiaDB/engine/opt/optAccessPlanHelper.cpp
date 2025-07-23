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

   Source File Name = optAccessPlanHelper.cpp

   Descriptive Name = Optimizer Access Plan Helper

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for helper of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/02/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "optAccessPlanHelper.hpp"
#include "optAccessPlan.hpp"
#include "dpsUtil.hpp"
#include "dmsIndexBuilder.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"

namespace engine
{

   /*
      _optAccessPlanConfigHolder implement
    */
   _optAccessPlanConfigHolder::_optAccessPlanConfigHolder ()
   : _config()
   {
   }

   _optAccessPlanConfigHolder::_optAccessPlanConfigHolder( const optAccessPlanConfig &config )
   : _config( config )
   {

   }

   _optAccessPlanConfigHolder::~_optAccessPlanConfigHolder ()
   {
   }

   /*
      _optAccessPlanHelper implement
    */
   _optAccessPlanHelper::_optAccessPlanHelper ( pmdEDUCB *eduCB,
                                                OPT_PLAN_CACHE_LEVEL cacheLevel,
                                                const optAccessPlanConfig &planConfig,
                                                const mthNodeConfig &mthConfig,
                                                const rtnExplainOptions *expOptions )
   : _mthMatchTreeHolder(),
     _optAccessPlanConfigHolder( planConfig ),
     _mthMatchConfigHolder( mthConfig ),
     _eduCB( eduCB ),
     _cacheLevel( cacheLevel ),
     _normalizer( getMatchConfigPtr() ),
     _isAutoHint( FALSE ),
     _isPredEstimated( FALSE ),
     _estSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _predSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _scanSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _estCPUCost( OPT_MTH_OPTR_DEFAULT_SELECTIVITY ),
     _hasNonGTIndex( FALSE ),
     _expOptions( expOptions )
   {
      // Adjust with cache level
      setMthEnableParameterized( mthConfig._enableParameterized &&
                                 ( cacheLevel >= OPT_PLAN_PARAMETERIZED ) ) ;
      setMthEnableFuzzyOptr( mthConfig._enableFuzzyOptr &&
                             ( cacheLevel >= OPT_PLAN_FUZZYOPTR ) ) ;
   }

   _optAccessPlanHelper::~_optAccessPlanHelper ()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_CLEAR, "_optAccessPlanHelper::clear" )
   void _optAccessPlanHelper::clear ()
   {
      _mthMatchTreeHolder::setMatchTree( NULL ) ;
      _predicateSet.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_SETMTH, "_optAccessPlanHelper::setMatchTree" )
   void _optAccessPlanHelper::setMatchTree ( _mthMatchTree *matchTree )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP_SETMTH ) ;

      _mthMatchTreeHolder::setMatchTree( matchTree ) ;
      _predicateSet.clear() ;

      _isPredEstimated     = FALSE ;
      _estSelectivity      = OPT_MTH_DEFAULT_SELECTIVITY ;
      _predSelectivity     = OPT_MTH_DEFAULT_SELECTIVITY ;
      _scanSelectivity     = OPT_MTH_DEFAULT_SELECTIVITY ;
      _estCPUCost          = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;

      PD_TRACE_EXIT( SDB__OPTAPHELP_SETMTH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_GETEST, "_optAccessPlanHelper::getEstimation" )
   void _optAccessPlanHelper::getEstimation ( optCollectionStat *pCollectionStat,
                                              double &estSelectivity,
                                              UINT32 &estCPUCost )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP_GETEST ) ;

      if ( !_isPredEstimated )
      {
         _evalEstimation( pCollectionStat ) ;
      }
      estSelectivity = _estSelectivity ;
      estCPUCost = _estCPUCost ;

      PD_TRACE_EXIT( SDB__OPTAPHELP_GETEST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_CHKGLOBTRANS_IDX, "_optAccessPlanHelper::checkGlobTrans" )
   INT32 _optAccessPlanHelper::checkGlobTrans( const rtnQueryOptions &options,
                                               dmsStorageUnit *su,
                                               dmsMBContext *mbContext,
                                               ixmIndexCB &indexCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_CHKGLOBTRANS_IDX ) ;

      DPS_TRANS_ID transID ;
      UINT64 transBeginTime = DPS_INVALID_TRANS_TIME ;
      UINT64 ixRebuildTime = DPS_INVALID_TRANS_TIME ;

      // no need to check index rebuild time for global transaction:
      // in below cases
      // - write operators
      // - non global transaction
      // - RR is not enabled ( RR requires --transon, --globtranson, --mvccon )
      // NOTE: write operators requires latest values and uncommited values,
      //       while rebuild phase will add latest values from disk to index,
      //       and uncommited values to memory index tree, so any indexes could
      //       be available for write operators from global transactions
      if ( options.isWriteOp() ||
           !_eduCB->isGlobTrans() ||
           ( _eduCB->getTransExecutor()->getTransIsolation() != 
             TRANS_ISOLATION_RR ) )
      {
         goto done ;
      }

      PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                "Failed to check global transaction, "
                "storage unit is invalid" ) ;
      PD_CHECK( NULL != mbContext, SDB_SYS, error, PDERROR,
                "Failed to check global transaction, "
                "meta-block context is invalid" ) ;
      PD_CHECK( indexCB.isInitialized(), SDB_SYS, error, PDERROR,
                "Failed to check global transaction, "
                "index control block is invalid" ) ;

      transID = _eduCB->getTransID() ;
      transBeginTime = transID.getLogicalTime() ;
      ixRebuildTime = indexCB.getRebuildTime() ;

      if ( DPS_INVALID_TRANSID_SN == ixRebuildTime )
      {
         // this index is rebuild before global transaction feature enabled
         // should be seen by all transactions
         goto done ;
      }
      else if ( DPS_MAX_TRANSID_SN == ixRebuildTime )
      {
         // if the rebuild time is maximum value, it means the rebuild time
         // of index had not been set yet
         // add this index into global transaction index list, so we could
         // try to set rebuild time for this index, which would allow this
         // index available for later global transactions
         OID indexOID ;

         rc = indexCB.getIndexID( indexOID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get OID for index, "
                      "rc: %d", rc ) ;

         try
         {
            _invalidGTIndexes.insert( indexOID ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to save invalid global transaction "
                    "index, error: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         // no matter if rebuild time is set or not, the rebuild time is
         // behind this transaction definitely
         PD_LOG( PDWARNING, "Failed to check index rebuild time for index "
                 "[%s.%s, %s], it had not been set yet", su->CSName(),
                 mbContext->mb()->_collectionName, indexCB.getName() ) ;
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

      // global transaction is started after index rebuild
      // ( plus a maximum time error for network delay consideration ),
      // the old version is missing for this transaction, so this index is not
      // available for this transaction
      PD_CHECK( transBeginTime > STP_MAX_TIME_ERROR_US &&
                transBeginTime - STP_MAX_TIME_ERROR_US > ixRebuildTime,
                SDB_DMS_INVALID_INDEXCB, error, PDWARNING,
                "Failed to check index rebuild time [%llu] of "
                "index [%s.%s %s] for global transaction %s, "
                "transaction time [%llu], index rebuild time [%llu]",
                ixRebuildTime,
                su->CSName(),
                mbContext->mb()->_collectionName,
                indexCB.getName(),
                dpsTransIDToString( transID ).c_str(),
                transBeginTime,
                ixRebuildTime ) ;

   done:
      if ( SDB_OK != rc )
      {
         _hasNonGTIndex = TRUE ;
      }
      PD_TRACE_EXITRC( SDB__OPTAPHELP_CHKGLOBTRANS_IDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_CHKGLOBTRANS_PLAN, "_optAccessPlanHelper::checkGlobTrans" )
   INT32 _optAccessPlanHelper::checkGlobTrans( const rtnQueryOptions &options,
                                               dmsStorageUnit *su,
                                               dmsMBContext *mbContext,
                                               _optAccessPlan *plan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_CHKGLOBTRANS_PLAN ) ;

      DPS_TRANS_ID transID ;
      UINT64 transBeginTime = DPS_INVALID_TRANS_TIME ;
      UINT64 planIxRebuildTime = DPS_INVALID_TRANS_TIME ;
      BOOLEAN lockedHere = FALSE ;

      // no need to check index rebuild time for global transaction:
      // - write operators
      // - non global transaction
      // - RR is not enabled ( RR requires --transon, --globtranson, --mvccon )
      // NOTE: write operators requires latest values and uncommited values,
      //       while rebuild phase will add latest values from disk to index,
      //       and uncommited values to memory index tree, so any indexes could
      //       be available for write operators from global transactions
      if ( options.isWriteOp() ||
           !_eduCB->isGlobTrans() ||
           ( _eduCB->getTransExecutor()->getTransIsolation() != 
             TRANS_ISOLATION_RR ) )
      {
         goto done ;
      }

      PD_CHECK( NULL != su, SDB_SYS, error, PDERROR,
                "Failed to check global transaction, "
                "storage unit is invalid" ) ;
      PD_CHECK( NULL != mbContext, SDB_SYS, error, PDERROR,
                "Failed to check global transaction, "
                "meta-block context is invalid" ) ;
      PD_CHECK( NULL != plan, SDB_SYS, error, PDERROR,
                "Failed to check index rebuild time, "
                "plan is invalid" ) ;

      // only check index scan
      if ( IXSCAN != plan->getScanType() )
      {
         goto done ;
      }

      transID = _eduCB->getTransID() ;
      transBeginTime = transID.getLogicalTime() ;
      planIxRebuildTime = plan->getIxRebuildTime() ;

      if ( DPS_INVALID_TRANSID_SN == planIxRebuildTime )
      {
         // this index is rebuild before global transaction feature enabled
         // should be seen by all transactions
         goto done ;
      }
      else if ( DPS_MAX_TRANSID_SN == planIxRebuildTime )
      {
         // if the rebuild time is maximum value, it means the rebuild time
         // of index had not been set yet when creating this plan
         // check if the rebuild time is available now, and update
         // index rebuild time of this plan if available

         dmsExtentID indexExtentID = DMS_INVALID_EXTENT ;

         if ( !mbContext->isMBLock() )
         {
            rc = mbContext->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get shared lock for "
                         "meta-block context, rc: %d", rc ) ;

            lockedHere = TRUE ;
         }

         if ( OPT_PLAN_TYPE_MAINCL == plan->getPlanType() )
         {
            // for main-collection plan, we need to get index CB by index name
            // of sub-collection
            rc = su->index()->getIndexCBExtent( mbContext,
                                                plan->getIndexName(),
                                                indexExtentID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s] for "
                         "collection, rc: %d", plan->getIndexName(), rc ) ;

         }
         else
         {
            // for general plan, get index CB from plan directly
            indexExtentID = plan->getIndexCBExtent() ;
         }

         // get rebuild time from index CB
         if ( DMS_INVALID_EXTENT != indexExtentID )
         {
            UINT64 ixRebuildTime = DPS_INVALID_TRANS_TIME ;
            ixmIndexCB indexCB( plan->getIndexCBExtent(), su->index(), NULL ) ;

            PD_CHECK( indexCB.isInitialized(),
                      SDB_DMS_INVALID_INDEXCB, error, PDERROR,
                      "Failed to get index CB for index [%s.%s, %s], "
                      "it is not initialized",
                      su->CSName(), mbContext->mb()->_collectionName,
                      plan->getIndexName() ) ;

            ixRebuildTime = indexCB.getRebuildTime() ;

            // has different index rebuild time, update plan's index rebuild
            // time with the one from index CB ( which maybe updated later )
            if ( ixRebuildTime != planIxRebuildTime )
            {
               plan->setIxRebuildTime( ixRebuildTime ) ;
               planIxRebuildTime = plan->getIxRebuildTime() ;
            }
         }

         // still has an maximum value, means we still failed to get rebuild
         // time for given index, this plan could not be used for current
         // global transaction
         if ( DPS_MAX_TRANSID_SN == planIxRebuildTime )
         {
            PD_LOG( PDWARNING, "Failed to check index rebuild time for index "
                    "[%s.%s, %s], it had not been set yet", su->CSName(),
                    mbContext->mb()->_collectionName, plan->getIndexName() ) ;
            rc = SDB_DMS_INVALID_INDEXCB ;
            goto error ;
         }
      }

      // global transaction is started after index rebuild
      // ( plus a maximum time error for network delay consideration ),
      // the old version is missing for this transaction, so this index is not
      // available for this transaction
      PD_CHECK( transBeginTime > STP_MAX_TIME_ERROR_US &&
                transBeginTime - STP_MAX_TIME_ERROR_US > planIxRebuildTime,
                SDB_DMS_INVALID_INDEXCB, error, PDWARNING,
                "Failed to check index rebuild time of plan [%s] "
                "global transaction %s, "
                "transaction time [%llu], index rebuild time [%llu]",
                plan->toString().c_str(),
                dpsTransIDToString( transID ).c_str(),
                transBeginTime,
                planIxRebuildTime ) ;

   done:
      if ( lockedHere )
      {
         mbContext->mbUnlock() ;
      }
      if ( SDB_OK != rc )
      {
         _hasNonGTIndex = TRUE ;
      }
      PD_TRACE_EXITRC( SDB__OPTAPHELP_CHKGLOBTRANS_PLAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_UPDATEIDXREBUILDTIME, "_optAccessPlanHelper::updateIxRebuildTime" )
   INT32 _optAccessPlanHelper::updateIxRebuildTime( dmsStorageUnit *su,
                                                    dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_UPDATEIDXREBUILDTIME ) ;

      BOOLEAN lockedHere = FALSE ;

      // check if have invalid global transaction indexes
      if ( _invalidGTIndexes.empty() )
      {
         goto done ;
      }

      SDB_ASSERT( NULL != su, "storage unit is invalid" ) ;
      SDB_ASSERT( NULL != mbContext, "meta-block context is invalid" ) ;

      // could not process with shared lock, in that case, we need to switch to
      // exclusive lock, which will have a little period when the meta-block
      // context is not locked, the meta may be manipulated by other threads
      PD_CHECK( SHARED != mbContext->mbLockType(),
                SDB_SYS, error, PDWARNING,
                "Failed to set index rebuild time, meta-block context should "
                "be exclusive locked or unlocked" ) ;

      if ( EXCLUSIVE != mbContext->mbLockType() )
      {
         rc = mbContext->mbTryLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock meta-block context, "
                      "rc: %d", rc ) ;
         lockedHere = TRUE ;
      }

      for ( OPT_INDEX_SET::iterator iter = _invalidGTIndexes.begin() ;
            _invalidGTIndexes.end() != iter ;
            ++ iter )
      {
         dmsExtentID indexExtent = DMS_INVALID_EXTENT ;
         const OID &indexOID = ( *iter ) ;
         rc = su->index()->getIndexCBExtent( mbContext, indexOID,
                                             indexExtent ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get index for oid [%s], "
                      "rc: %d", indexOID.toString().c_str(), rc ) ;

         // get index CB, and update rebuild time
         {
            ixmIndexCB indexCB( indexExtent, su->index(), NULL ) ;
            PD_CHECK( indexCB.isInitialized(),
                      SDB_DMS_INVALID_INDEXCB, error, PDWARNING,
                      "Failed to get index CB, it is not initialized" ) ;

            dmsIndexBuilder::updateRebuildTime( mbContext, indexCB, FALSE ) ;
         }
      }

   done:
      if ( lockedHere )
      {
         mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__OPTAPHELP_UPDATEIDXREBUILDTIME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP__EVALEST, "_optAccessPlanHelper::_evalEstimation" )
   void _optAccessPlanHelper::_evalEstimation ( optCollectionStat *pCollectionStat )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP__EVALEST ) ;

      double predSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      double scanSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      double tmpSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      UINT32 tmpCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

      if ( getMatchTree() != NULL )
      {
         if ( pCollectionStat )
         {
            predSelectivity = pCollectionStat->evalPredicateSet(
                  _predicateSet, mthEnabledMixCmp(), scanSelectivity ) ;
         }
         getMatchTree()->evalEstimation( pCollectionStat, tmpSelectivity,
                                         tmpCPUCost ) ;
         tmpSelectivity *= predSelectivity ;
      }

      _estSelectivity = OPT_ROUND_SELECTIVITY( tmpSelectivity ) ;
      _predSelectivity = OPT_ROUND_SELECTIVITY( predSelectivity ) ;
      _scanSelectivity = OPT_ROUND_SELECTIVITY( scanSelectivity ) ;
      _estCPUCost = tmpCPUCost ;
      _isPredEstimated = TRUE ;

      PD_TRACE_EXIT( SDB__OPTAPHELP__EVALEST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_GENSIMMTH, "_optAccessPlanHelper::normalizeQuery" )
   INT32 _optAccessPlanHelper::normalizeQuery ( const BSONObj &query,
                                                BSONObjBuilder &normalBuilder,
                                                rtnParamList &parameters,
                                                BOOLEAN &invalidMatcher )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_GENSIMMTH ) ;

      // No need to be copied
      _query = query ;

      // Normalize the query with simple parser
      rc = _normalizer.normalize( query, normalBuilder, parameters ) ;
      invalidMatcher = _normalizer.isInvalidMatcher() ;
      PD_RC_CHECK( rc, invalidMatcher ? PDERROR : PDDEBUG,
                   "Failed to normalize query [%s] with normalizer, rc: %d",
                   query.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTAPHELP_GENSIMMTH, rc ) ;
      return rc ;

   error :
      _normalizer.clear() ;
      parameters.clearParams() ;
      goto done ;
   }

}
