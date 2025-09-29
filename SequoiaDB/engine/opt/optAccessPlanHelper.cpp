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
   _optAccessPlanHelper::_optAccessPlanHelper ( OPT_PLAN_CACHE_LEVEL cacheLevel,
                                                const optAccessPlanConfig &planConfig,
                                                const mthNodeConfig &mthConfig,
                                                const rtnExplainOptions *expOptions )
   : _mthMatchTreeHolder(),
     _optAccessPlanConfigHolder( planConfig ),
     _mthMatchConfigHolder( mthConfig ),
     _cacheLevel( cacheLevel ),
     _normalizer( getMatchConfigPtr() ),
     _isAutoHint( FALSE ),
     _isPredEstimated( FALSE ),
     _estSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _predSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _scanSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _estCPUCost( OPT_MTH_OPTR_DEFAULT_SELECTIVITY ),
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
