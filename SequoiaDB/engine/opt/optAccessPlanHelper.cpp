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
   _optAccessPlanHelper::_optAccessPlanHelper ( IExecutor *eduCB,
                                                OPT_PLAN_CACHE_LEVEL cacheLevel,
                                                const optAccessPlanConfig &planConfig,
                                                const mthNodeConfig &mthConfig,
<<<<<<< HEAD
=======
                                                CONST_CL_META_INFO_PTR clMetaPtr,
                                                CONST_CL_STAT_INFO_PTR clStatPtr,
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
<<<<<<< HEAD
     _estCPUCost( OPT_MTH_OPTR_DEFAULT_SELECTIVITY ),
=======
     _estCPUCost( OPT_MTH_DEFAULT_CPU_COST ),
     _hasNonGTIndex( FALSE ),
     _clMetaPtr(clMetaPtr),
     _clStatPtr(clStatPtr),
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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
      _selectivityCache.clear() ;
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
      _estCPUCost          = OPT_MTH_DEFAULT_CPU_COST ;

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
                                               const CONST_INDEX_META_INFO_PTR &pIndex )
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
           !_eduCB->getTransID().isGlobTrans() ||
           ( _eduCB->getTransIsolation() != 
             TRANS_ISOLATION_RR ) )
      {
         goto done ;
      }

      transID = _eduCB->getTransID() ;
      transBeginTime = transID.getLogicalTime() ;
      ixRebuildTime = pIndex->getStpEffectiveTime() ;

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
         OID indexOID = pIndex->getOID() ;
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
         PD_LOG( PDWARNING,
                 "Failed to check index rebuild time for index "
                 "[%s, %s], it had not been set yet",
                 options.getCLFullName(), pIndex->getIndexName() ) ;
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
                "index [%s %s] for global transaction %s, "
                "transaction time [%llu], index rebuild time [%llu]",
                ixRebuildTime,
                options.getCLFullName(),
                pIndex->getIndexName(),
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
                                               _optAccessPlan *plan )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_CHKGLOBTRANS_PLAN ) ;

      DPS_TRANS_ID transID ;
      UINT64 transBeginTime = DPS_INVALID_TRANS_TIME ;
      UINT64 planIxRebuildTime = DPS_INVALID_TRANS_TIME ;

      // no need to check index rebuild time for global transaction:
      // - write operators
      // - non global transaction
      // - RR is not enabled ( RR requires --transon, --globtranson, --mvccon )
      // NOTE: write operators requires latest values and uncommited values,
      //       while rebuild phase will add latest values from disk to index,
      //       and uncommited values to memory index tree, so any indexes could
      //       be available for write operators from global transactions
      if ( options.isWriteOp() ||
           !_eduCB->getTransID().isGlobTrans() ||
           ( _eduCB->getTransIsolation() != 
             TRANS_ISOLATION_RR ) )
      {
         goto done ;
      }

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
         CONST_INDEX_META_INFO_PTR pIndex = _clMetaPtr->seek( plan->getIndexName() );

         // get rebuild time from index CB
         if ( !pIndex )
         {
            UINT64 ixRebuildTime = DPS_INVALID_TRANS_TIME ;
         

            ixRebuildTime = pIndex->getStpEffectiveTime() ;

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
            PD_LOG( PDWARNING,
                    "Failed to check index rebuild time for index "
                    "[%s, %s], it had not been set yet",
                    options.getCLFullName(), plan->getIndexName() );
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
      if ( SDB_OK != rc )
      {
         _hasNonGTIndex = TRUE ;
      }
      PD_TRACE_EXITRC( SDB__OPTAPHELP_CHKGLOBTRANS_PLAN, rc ) ;
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

      optIndexPathEncoder encoder ;

      if ( getMatchTree() != NULL )
      {
         if ( pCollectionStat )
         {
            predSelectivity = pCollectionStat->evalPredicateSet(
                  _predicateSet, mthEnabledMixCmp(), scanSelectivity, encoder ) ;
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

      saveSelectivityToCache( encoder.getPath(), _predSelectivity, _scanSelectivity ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_SAVESLCTY, "_optAccessPlanHelper::saveSelectivityToCache" )
   INT32 _optAccessPlanHelper::saveSelectivityToCache( const ossPoolString &indexPath,
                                                       double predSelectivity,
                                                       double scanSelectivity )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_SAVESLCTY ) ;

      if ( !indexPath.empty() )
      {
         try
         {
            _selectivityCache[ indexPath ] = optPlanSelectivity( predSelectivity,
                                                                 scanSelectivity ) ;
         }
         catch ( exception &e )
         {
            _selectivityCache.erase( indexPath ) ;
            PD_LOG( PDERROR, "Failed to save selectivity, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__OPTAPHELP_SAVESLCTY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_GETSLCTY, "_optAccessPlanHelper::getSelectivityFromCache" )
   BOOLEAN _optAccessPlanHelper::getSelectivityFromCache( const ossPoolString &indexPath,
                                                          double &predSelectivity,
                                                          double &scanSelectivity )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_GETSLCTY ) ;

      if ( !indexPath.empty() )
      {
         optPlanSelectivityCache::iterator it = _selectivityCache.find( indexPath ) ;
         if ( it != _selectivityCache.end() )
         {
            predSelectivity = it->second._predSelectivity ;
            scanSelectivity = it->second._scanSelectivity ;
            result = TRUE ;
         }
      }

      PD_TRACE_EXIT( SDB__OPTAPHELP_GETSLCTY ) ;

      return result ;
   }

}
