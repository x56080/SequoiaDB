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

   Source File Name = optAccessPlan.cpp

   Descriptive Name = Optimizer Access Plan

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for optimizer
   access plan creation. It will calculate based on rules and try to estimate
   a lowest cost plan to access data.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "optAccessPlan.hpp"
#include "dmsStorageUnit.hpp"
#include "../bson/ordering.h"
#include "rtnAPM.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"

using namespace bson;
namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__OPTHINT, "_optAccessPlan::_optimizeHint" )
   INT32 _optAccessPlan::_optimizeHint ( dmsMBContext *mbContext,
                                         const CHAR *pIndexName,
                                         const rtnPredicateSet &predSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__OPTHINT );

      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;
      INT32 dir = 1 ;
      _idxEstimateDetail detail ;

      rc = _su->index()->getIndexCBExtent( mbContext, pIndexName,
                                           indexCBExtent ) ;
      if ( rc )
      {
         goto error ;
      }

      // call estimate index to get estimation and most importantly the scan
      // direction
      rc = _estimateIndex ( indexCBExtent, dir, detail ) ;
      if ( rc )
      {
         if ( SDB_IXM_UNEXPECTED_STATUS == rc )
         {
            PD_LOG ( PDINFO, "Unable to use the specified index: %s, index is "
                     "not normal status.", pIndexName ) ;
         }
         goto error ;
      }

      rc = _useIndex ( indexCBExtent, dir, predSet, detail ) ;

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__OPTHINT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__OPTHINT2, "_optAccessPlan::_optimizeHint" )
   INT32 _optAccessPlan::_optimizeHint ( dmsMBContext *mbContext,
                                         const OID &indexOID,
                                         const rtnPredicateSet &predSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__OPTHINT2 ) ;

      dmsExtentID indexCBExtent = DMS_INVALID_EXTENT ;
      INT32 dir = 1 ;
      _idxEstimateDetail detail ;

      rc = _su->index()->getIndexCBExtent( mbContext, indexOID,
                                           indexCBExtent ) ;
      if ( rc )
      {
         goto error ;
      }
      // call estimate index to get estimation and most importantly the scan
      // direction
      rc = _estimateIndex ( indexCBExtent, dir, detail ) ;
      if ( rc )
      {
         if ( SDB_IXM_UNEXPECTED_STATUS == rc )
         {
            PD_LOG ( PDINFO, "Unable to use the specified index, index is "
                     "not normal status." ) ;
         }
         goto error ;
      }
      rc = _useIndex ( indexCBExtent, dir, predSet, detail ) ;

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__OPTHINT2, rc );
      return rc ;
   error :
      goto done ;
   }

   // caller must hold S latch on the obj
   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__OPTHINT3, "_optAccessPlan::_optimizeHint" )
   INT32 _optAccessPlan::_optimizeHint( dmsMBContext *mbContext,
                                        const rtnPredicateSet &predSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__OPTHINT3 ) ;
      SDB_ASSERT ( !_hint.isEmpty(), "hint can't be empty" ) ;

      BOOLEAN hasError = FALSE ;
      BSONObjIterator it ( _hint ) ;
      PD_LOG ( PDDEBUG, "Hint is provided: %s", _hint.toString().c_str() ) ;
      rc = SDB_RTN_INVALID_HINT ;
      // user can define more than one index name/oid in hint, it will pickup
      // the first valid one
      while ( it.more() )
      {
         BSONElement hint = it.next() ;
         if ( hint.type() == String )
         {
            if ( '\0' != *( hint.valuestr() ) )
            {
               PD_LOG ( PDDEBUG, "Try to use index: %s", hint.valuestr() ) ;
               // search based on index name
               rc = _optimizeHint ( mbContext, hint.valuestr(), predSet ) ;
            }
            else
            {
               /// return error that go to auto-estimate
               _autoHint = TRUE ;
               _hintFailed = TRUE ;
               rc = SDB_RTN_INVALID_HINT ;
               goto error ;
            }
         }
         else if ( hint.type() == jstOID )
         {
            PD_LOG ( PDDEBUG, "Try to use index: %s",
                     _hint.toString().c_str() ) ;
            // search based on OID
            rc = _optimizeHint ( mbContext, hint.__oid(), predSet ) ;
         }
         else if ( hint.type() == jstNULL )
         {
            PD_LOG ( PDDEBUG, "Use Collection Scan by Hint" ) ;
            // if we use null in the hint, we use tbscan
            _scanType = TBSCAN ;
            rc = SDB_OK ;
            // if hint shows tbscan, let's check whether manual sort is required
            if ( !_orderBy.isEmpty() )
            {
               _sortRequired = TRUE ;
            }
         }
         else if ( hint.isABSONObj() )
         {
            continue ;
         }

         if ( SDB_OK == rc )
         {
            break ;
         }
         else
         {
            hasError = TRUE ;
         }
      }

      // let's check return value
      if ( rc )
      {
         if ( hasError )
         {
            PD_LOG ( PDWARNING, "Hint is not valid: %s",
                     _hint.toString().c_str() ) ;
            _hintFailed = TRUE ;
         }
         goto error ;
      }
      PD_LOG ( PDDEBUG, "Hint is successfully applied" ) ;

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__OPTHINT3, rc );
      return rc ;
   error :
      goto done ;
   }

   // we are not using real CBO since we don't have time to implement statistics
   // yet, so let's hardcode base line cost for now and we'll improve it further
   #define TEMP_COST_BASELINE 10000

   // output cost estimation, dir, and indexCBExtent
   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__ESTINX, "_optAccessPlan::_estimateIndex" )
   INT32 _optAccessPlan::_estimateIndex ( dmsExtentID indexCBExtent,
                                          INT32 &dir,
                                          _idxEstimateDetail &detail )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__ESTINX );
      ixmIndexCB indexCB ( indexCBExtent, _su->index(), NULL ) ;
      if ( !indexCB.isInitialized() )
      {
         PD_LOG ( PDWARNING, "Failed to use index at extent %d",
                  indexCBExtent ) ;
         rc = SDB_DMS_INIT_INDEX ;
         goto error ;
      }
      if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL )
      {
         PD_LOG ( PDDEBUG, "Index is not normal status, skip" ) ;
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto error ;
      }
      try
      {
         // compare with pure tablescan, each index scan need to advance +
         // compare key + fetch, which could be significantely more expensive
         // than tbscan. So let's increase baseline cost for full index
         // scan+fetch
         // note this is a very bad hack
         detail.reset() ;

         BSONObj idxPattern = indexCB.keyPattern() ;
         Ordering keyorder = Ordering::make(idxPattern) ;
         Ordering orderByorder = Ordering::make ( _orderBy ) ;
         INT32 nFields = _orderBy.nFields() ;
         INT32 nQueryFields = 0 ;
         INT32 orderMatchFields = 0 ;
         INT32 matchedFields = 0 ;
         BSONObjIterator keyItr (idxPattern) ;
         BSONObjIterator orderItr ( _orderBy ) ;
         // first check order
         FLOAT32 orderFactor = 1.0f ;
         dir = 1 ;
         BOOLEAN start = TRUE ;
         rtnStartStopKey startStopKey;
         BSONElement startKey;
         BSONElement stopKey;
         BOOLEAN matchAll = FALSE ;

         if ( nFields > 0 )
         {
            detail.setFlag( OPT_QUERY_FLAG_WITH_SORT ) ;
         }

         while ( keyItr.more() && orderItr.more() )
         {
            BSONElement keyEle = keyItr.next() ;
            BSONElement orderEle = orderItr.next() ;
            if ( ossStrcmp ( keyEle.fieldName(), orderEle.fieldName() ) == 0 )
            {
               // if the first field match name, let's compare the order
               if ( start )
               {
                  // if key is forward and order by is forward, dir = forward
                  // if key is backward and order by is forward, dir = backward
                  // if key is backward and order by is backward, dir = forward
                  // if key is forward and order by is backward, dir = backward
                  dir = (keyorder.get(orderMatchFields ) ==
                         orderByorder.get(orderMatchFields ))?1:-1 ;
                  start = FALSE ;
               }
               // break if the order is different
               if ( keyorder.get(orderMatchFields )*dir !=
                    orderByorder.get(orderMatchFields ) )
                  break ;
               ++orderMatchFields ;
            }
            else
               break ;
         }
         orderFactor = 1.0f - ((nFields == 0) ?
               (0) : (((FLOAT32)orderMatchFields)/((FLOAT32)nFields)));
         orderFactor = OSS_MIN(1.0f, orderFactor) ;
         orderFactor = OSS_MAX(0.0f, orderFactor) ;

         // then we check query compare with index pattern
         FLOAT32 queryFactor = 1.0f ;
         keyItr = BSONObjIterator ( idxPattern ) ;
         nFields = idxPattern.nFields() ;
         const map<string, rtnPredicate> &predicates
                     = _matcher.getPredicateSet().predicates();
         map<string, rtnPredicate>::const_iterator it;
         nQueryFields = predicates.size() ;
         if ( nQueryFields > 0 )
         {
            detail.setFlag( OPT_QUERY_FLAG_WITH_COND ) ;
         }

         while ( keyItr.more() )
         {
            BSONElement keyEle = keyItr.next() ;
            // for each element in the key, let's see if we used it in the
            // query. More keys used by query, we esimtate the index may more
            // satisfy our requirement
            if (( it = predicates.find( keyEle.fieldName() ))
                  != predicates.end() )
            {
               // some cases have predicates, though it's not
               // that proper to use index, such as the case
               // with max and min boundanry,
               // so we need to lead such cases to table scan here
               startStopKey = it->second._startStopKeys[0] ;
               startKey = startStopKey._startKey._bound ;
               stopKey = startStopKey._stopKey._bound ;

               if(0 == startKey.woCompare( bson::minKey.firstElement() ) &&
                  0 == stopKey.woCompare( bson::maxKey.firstElement() ))
               {
                  break;
               }

               ++matchedFields ;
            }
            else
            {
               break;
            }
         }
         if ( nFields == 0 || nQueryFields == 0 )
         {
            queryFactor = 1.0f ;
         }
         else
         {
            queryFactor =
                  1.0f - ((FLOAT32)matchedFields)/((FLOAT32)nQueryFields) ;
            FLOAT32 factor = queryFactor ;
            for ( INT32 i = 1; i < matchedFields; ++i )
            {
               queryFactor *= factor ;
            }
         }
         queryFactor = OSS_MIN(1.0f, queryFactor) ;
         queryFactor = OSS_MAX(0.0f, queryFactor) ;

         /// we try to set matchall only when all fields converted into predicates
         if ( _matcher.totallyConverted() )
         {
            matchAll = ( ( 0 != matchedFields ) &&
                         ( matchedFields == nQueryFields ) &&
                         matchedFields <= idxPattern.nFields() ) ||
                       ( 0 == nQueryFields );
         }

         detail.setData( 2 * TEMP_COST_BASELINE, queryFactor,
                         1.0f - ((FLOAT32)matchedFields) / ((FLOAT32)nFields),
                         orderFactor, matchAll ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "Index Scan Estimation: %s : %d",
               indexCB.getName (), detail.getCost() ) ;

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__ESTINX, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__ESTINX2, "_optAccessPlan::_estimateIndex" )
   INT32 _optAccessPlan::_estimateIndex ( dmsMBContext *mbContext,
                                          INT32 indexID,
                                          INT32 &dir,
                                          dmsExtentID &indexCBExtent,
                                          _idxEstimateDetail &detail )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__ESTINX2 ) ;

      rc = _su->index()->getIndexCBExtent( mbContext, indexID,
                                           indexCBExtent ) ;
      if ( rc )
      {
         goto done ;
      }
      rc = _estimateIndex ( indexCBExtent, dir, detail ) ;

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__ESTINX2, rc );
      return rc ;
   }

   void _optAccessPlan::_estimateTBScan ( INT64 &costEstimation )
   {
      costEstimation = TEMP_COST_BASELINE ;
      PD_LOG ( PDDEBUG, "Collection Scan estimation cost is %d",
               costEstimation ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN__USEINX, "_optAccessPlan::_useIndex" )
   INT32 _optAccessPlan::_useIndex ( dmsExtentID indexCBExtent,
                                     INT32 dir,
                                     const rtnPredicateSet &predSet,
                                     const _idxEstimateDetail &detail )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OPTACCPLAN__USEINX );
      ixmIndexCB indexCB ( indexCBExtent, _su->index(), NULL ) ;
      if ( !indexCB.isInitialized() )
      {
         PD_LOG ( PDWARNING, "Failed to use index at extent %d",
                  indexCBExtent ) ;
         rc = SDB_DMS_INIT_INDEX ;
         goto error ;
      }
      {
         _direction = dir ;
         // if there's order by statement, let's see if the index we are using
         // is able to bypass sort phase
         if ( !_orderBy.isEmpty() )
         {
            BSONObj idxPattern = indexCB.keyPattern() ;
            Ordering keyorder = Ordering::make(idxPattern) ;
            Ordering orderByorder = Ordering::make ( _orderBy ) ;
            INT32 matchedFields = 0 ;
            BSONObjIterator keyItr (idxPattern) ;
            BSONObjIterator orderItr ( _orderBy ) ;
            while ( keyItr.more() && orderItr.more() )
            {
               BSONElement keyEle = keyItr.next() ;
               BSONElement orderEle = orderItr.next() ;
               if (ossStrcmp( keyEle.fieldName(), orderEle.fieldName() ) == 0)
               {
                  if ( keyorder.get(matchedFields)*dir !=
                       orderByorder.get(matchedFields) )
                     break ;
                  ++matchedFields ;
               }
               else
                  break ;
            }
            if ( matchedFields == _orderBy.nFields() )
               _sortRequired = FALSE ;
            else
               _sortRequired = TRUE ;
         }
         if ( _predList )
            SDB_OSS_DEL _predList ;
         // memory is freed in destructor
         _predList = SDB_OSS_NEW rtnPredicateList ( predSet, &indexCB,
                                                          _direction ) ;
         if ( !_predList )
         {
            PD_LOG ( PDERROR, "Out of memory" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _scanType = IXSCAN ;
         _indexCBExtent = indexCBExtent ;
         _indexLID = indexCB.getLogicalID() ;
         indexCB.getIndexID(_indexOID) ;
         {
         const CHAR *idxName = indexCB.getName() ;
         ossMemcpy( _idxName, idxName, ossStrlen( idxName ) ) ;
         }

         if ( !_matcher.isMatchesAll() && detail.matchAll() )
         {
            _matcher.setMatchesAll( TRUE ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN__USEINX, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _optAccessPlan::_checkOrderBy()
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator iter( _orderBy ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         INT32 value ;
         if ( ossStrcasecmp( ele.fieldName(), "" ) == 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's fieldName can't be empty:rc=%d", rc ) ;
            goto error ;
         }

         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's value must be numberic:rc=%d", rc ) ;
            goto error ;
         }

         value = ele.numberInt() ;
         if ( value != 1 && value != -1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "orderBy's value must be 1 or -1:rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTACCPLAN_OPT, "_optAccessPlan::optimize" )
   INT32 _optAccessPlan::optimize()
   {
      INT32 rc = SDB_OK ;
      dmsMBContext *mbContext = NULL ;

      PD_TRACE_ENTRY ( SDB__OPTACCPLAN_OPT ) ;

      rc = _checkOrderBy() ;
      PD_RC_CHECK( rc, PDERROR, "failed to check orderby", rc ) ;

      // first let's build matcher
      rc = _matcher.loadPattern ( _query ) ;
      PD_RC_CHECK ( rc, (SDB_RTN_INVALID_PREDICATES==rc) ? PDINFO : PDERROR,
                    "Failed to load query, rc = %d", rc ) ;
      // currently the plan is set to tablescan with a valid matcher and Null
      // predList

      // then let's see if there's better index plan exist
      {
         // get the predicate sets from matcher
         const rtnPredicateSet &predSet = _matcher.getPredicateSet () ;
         // lock the collection before picking up the right index
         // this function will also get collection id
         rc = _su->data()->getMBContext( &mbContext, _collectionName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get dms mb context failed, rc: %d", rc ) ;

         if ( _hint.isEmpty() || SDB_OK != _optimizeHint( mbContext, predSet ) )
         {
            // if hint not defined, or if failed to optimize using hint, let's
            // set the plan is automatically picked
            _isAutoPlan = TRUE ;
            // if hint is not defined, or if failed to optimize using
            // hint, let's check each index
            dmsExtentID bestMatchedIndexCBExtent = DMS_INVALID_EXTENT ;
            INT64 bestCostEstimation = 0 ;
            INT32 bestMatchedIndexDirection = 1 ;
            _idxEstimateDetail detail ;

            // use tbscan as baseline
            _estimateTBScan ( bestCostEstimation ) ;

            // If no query condition nor sort clause, use table scan directly.
            if ( !_query.isEmpty() || !_orderBy.isEmpty() )
            {
               // Estimate and find the best index.
               for ( INT32 i = 0 ; i<DMS_COLLECTION_MAX_INDEX; i++ )
               {
                  _idxEstimateDetail tmpDetail ;
                  dmsExtentID extID ;
                  INT32 dir ;
                  rc = _estimateIndex( mbContext, i, dir, extID, tmpDetail ) ;
                  if ( SDB_IXM_NOTEXIST == rc )
                  {
                     break ;
                  }
                  if ( SDB_OK == rc &&
                       ( !detail.valid() || tmpDetail.betterThan( detail ) ) )
                  {
                     SDB_ASSERT( tmpDetail.valid(), "New detail is invalid" )  ;
                     detail = tmpDetail ;
                     bestMatchedIndexCBExtent = extID ;
                     bestMatchedIndexDirection = dir ;
                     if ( detail.hitLowBound() )
                     {
                        break ;
                     }
                  }
                  // otherwise we don't do anything, just skip
               }
               // if best matched index shows any index is better than tbscan, then
               // let's use the index
               if ( detail.valid() && detail.getCost() < bestCostEstimation )
               {
                  PD_LOG ( PDDEBUG, "Use Index Scan" ) ;
                  rc = _useIndex ( bestMatchedIndexCBExtent,
                                   bestMatchedIndexDirection,
                                   predSet,
                                   detail ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDWARNING, "Failed to use index %d",
                              bestMatchedIndexCBExtent ) ;
                  }
               }
               else
               {
                  PD_LOG ( PDDEBUG, "Use Collection Scan" ) ;
                  // otherwise if there's no index is better than tbscan, let's
                  // check if we need manually sort
                  if ( !_orderBy.isEmpty() )
                  {
                     _sortRequired = TRUE ;
                  }
               }
            }
         }

         // unlock collection and reset rc
         rc = SDB_OK ;
         mbContext->mbUnlock() ;
         _isInitialized = TRUE ;
         _isValid = TRUE ;
      }

      if ( _autoHint && _hintFailed &&
           IXSCAN == getScanType() )
      {
         _hintFailed = FALSE ;
      }
   done :
      if ( mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      PD_TRACE_EXITRC ( SDB__OPTACCPLAN_OPT, rc );
      return rc ;
   error :
      goto done ;
   }

   UINT32 _optAccessPlan::hash ( const BSONObj &query, const BSONObj &orderBy,
                                 const BSONObj &hint )
   {
      UINT32 hashValue = ossHash ( query.objdata(), query.objsize() ) ^
                         ossHash ( orderBy.objdata(), orderBy.objsize() ) ;
      /// hint obj
      BSONObjIterator itr( hint ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( e.isABSONObj() )
         {
            continue ;
         }
         hashValue ^= ossHash( e.value(), e.valuesize() ) ;
      }

      return hashValue ;
   }

   BOOLEAN _optAccessPlan::equal( const _optAccessPlan &right ) const
   {
      if ( right.getValid() )
      {
         return Reusable( right._query, right._orderBy, right._hint ) ;
      }
      return FALSE ;
   }

   BOOLEAN _optAccessPlan::Reusable ( const BSONObj &query,
                                      const BSONObj &orderBy,
                                      const BSONObj &hint ) const
   {
      if ( !_isValid )
         return FALSE ;
      // user query must be identical
      if ( !_query.shallowEqual ( query ) )
         return FALSE ;

      // order by must be identical
      if ( !_orderBy.shallowEqual ( orderBy ) )
         return FALSE ;

      /// hint must compare field by field, and need ignore object field and
      /// field name
      BSONObjIterator itr( hint ) ;
      BSONObjIterator itrSelf( _hint ) ;
      while( itr.more() )
      {
         BSONElement e2 ;
         BSONElement e1 = itr.next() ;
         if ( e1.isABSONObj() )
         {
            continue ;
         }

         while( itrSelf.more() )
         {
            e2 = itrSelf.next() ;
            if ( e2.isABSONObj() )
            {
               continue ;
            }
            break ;
         }

         if ( 0 != e1.woCompare( e2, false ) )
         {
            return FALSE ;
         }
      }

      /// if _hint has other hint field, not the same
      while( itrSelf.more() )
      {
         BSONElement e = itrSelf.next() ;
         if ( !e.isABSONObj() )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   void _optAccessPlan::release()
   {
      if ( _apm )
      {
         _apm->releasePlan(this) ;
      }
      else
      {
         SDB_OSS_DEL this ;
      }
   }

   std::string _optAccessPlan::toString() const
   {
      stringstream ss ;
      ss << "CollectionName:" << _collectionName
         << ",IndexName:" << _idxName
         << ",OrderBy:" << _orderBy.toString().c_str()
         << ",Query:" << _query.toString().c_str()
         << ",Hint:" << _hint.toString().c_str()
         << ",HintFailed:" << _hintFailed
         << ",Direction:" << _direction
         << ",ScanType:" << ( TBSCAN == _scanType ? "TBSCAN" : "IXSCAN" )
         << ",Valid:" << _isValid
         << ",AutoPlan:" << _isAutoPlan
         << ",HashValue:" << _hashValue
         << ",Count:" << _useCount.peek()
         << ",SortRequired:" << _sortRequired
         << ",AutoHint:" << _autoHint ;
      return ss.str() ;
   }

}

