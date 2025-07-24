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

   Source File Name = rtnAPM.cpp

   Descriptive Name = Runtime Access Plan Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Access Plan
   Manager, which is used to pool access plans that previously generated.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnAPM.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "pmd.hpp"
namespace engine
{
   static INT32 createNewPlan( _dmsStorageUnit *su,
                               const CHAR *name,
                               const BSONObj &query,
                               const BSONObj &orderBy,
                               const BSONObj &hint,
                               optAccessPlan **out )
   {
      INT32 rc = SDB_OK ;
      *out = SDB_OSS_NEW optAccessPlan ( su, name, query,
                                         orderBy, hint ) ;
      if ( !(*out) )
      {
         PD_LOG ( PDERROR, "Not able to allocate memory for new plan" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      (*out)->setAPM ( NULL ) ;
      rc = (*out)->optimize() ;
      PD_RC_CHECK ( rc, (SDB_RTN_INVALID_PREDICATES==rc)?PDINFO:PDERROR,
                    "Failed to optimize plan, query: %s\norder %s\nhint %s",
                    query.toString().c_str(),
                    orderBy.toString().c_str(),
                    hint.toString().c_str() ) ;
   done:
      return rc ;
   error:
      if ( NULL != *out )
      {
         SDB_OSS_DEL (*out) ;
         (*out) = NULL ;
      }
      goto done ;
   }

   /*
      _rtnAccessPlanList implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPL_INVALIDATE, "_rtnAccessPlanList::invalidate" )
   void _rtnAccessPlanList::invalidate ( UINT32 &cleanNum )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPL_INVALIDATE );
      cleanNum = 0 ;
      vector<optAccessPlan *>::iterator it ;

      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

      // check if the plan is in the list
      for ( it = _plans.begin(); it != _plans.end(); )
      {
         // if so let's decrease the count
         if ( (*it)->getCount() == 0 )
         {
            ++cleanNum ;

            SDB_OSS_DEL (*it) ;
            // in vector, erase returns the next valid iterator, or
            // vector::end()
            it = _plans.erase(it) ;
         }
         else
         {
            (*it)->setValid( FALSE ) ;
            ++it ;
         }
      }

      _emptyAPNum.sub( cleanNum ) ;
      _pSetEmptyAPNum->sub( cleanNum ) ;
      _pTotalEmptyAPNum->sub( cleanNum ) ;

      PD_TRACE_EXIT ( SDB__RTNACCESSPL_INVALIDATE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPL_ADDPLAN, "_rtnAccessPlanList::addPlan" )
   BOOLEAN _rtnAccessPlanList::addPlan( optAccessPlan *plan,
                                        const rtnAPContext &context,
                                        SINT32 &incSize )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPL_ADDPLAN ) ;
      SINT32 inc = 0 ;
      BOOLEAN hasAdd = FALSE ;
      UINT32 clearNum = 0 ;

      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

      /// when lastID changed, need to judge equal
      if ( context._lastListID != _lastID )
      {
         vector<optAccessPlan *>::iterator it ;
         for ( it = _plans.begin(); it != _plans.end(); ++it )
         {
            if ( (*it)->equal( *plan ) )
            {
               /// already exist
               plan->setAPM( NULL ) ;
               goto done ;
            }
         }
      }

      // now let's see how many plans we have, if so let's attempt to remove
      // any plan that not been used
      if ( _plans.size() >= RTN_APL_SIZE &&
           ( clearNum = _clearFast( 1 ) ) == 0 )
      {
         inc = 0 ;
         plan->setAPM ( NULL ) ;
         PD_LOG ( PDINFO, "AccessPlanList is full" ) ;
      }
      else
      {
         plan->setAPM( _apm ) ;
         inc = 1 - clearNum ;
         hasAdd = TRUE ;
         _plans.insert ( _plans.begin(), plan ) ;
         plan->incCount() ;
         ++_lastID ;
      }

   done:
      incSize = inc ;
      PD_TRACE_EXIT( SDB__RTNACCESSPL_ADDPLAN ) ;
      return hasAdd ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPL_GETPLAN, "_rtnAccessPlanList::getPlan" )
   optAccessPlan* _rtnAccessPlanList::getPlan ( const BSONObj &query,
                                                const BSONObj &orderBy,
                                                const BSONObj &hint,
                                                rtnAPContext &context )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPL_GETPLAN ) ;

      optAccessPlan *pOut = NULL ;
      vector<optAccessPlan *>::iterator it ;

      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

      context._lastListID = _lastID ;

      // check if the plan is in the list
      for ( it = _plans.begin() ; it != _plans.end() ; ++it )
      {
         if ( (*it)->Reusable( query, orderBy, hint )  )
         {
            // we found one plan match, then let's delete the plan we just
            // created and return the existing one
            pOut = *it ;
            // if it's not the first one, let's remove it and re-add to begin
            // of the list
            if ( it != _plans.begin() )
            {
               _plans.erase( it ) ;
               _plans.insert ( _plans.begin(), pOut ) ;
            }
            // increase usage count
            if ( 0 == pOut->incCount() )
            {
               _emptyAPNum.dec() ;
               _pSetEmptyAPNum->dec() ;
               _pTotalEmptyAPNum->dec() ;
            }
            break ;
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPL_GETPLAN ) ;
      return pOut ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNACCESSPL_RELPL, "rtnAccessPlanList::releasePlan" )
   void _rtnAccessPlanList::releasePlan ( optAccessPlan *plan )
   {
      PD_TRACE_ENTRY ( SDB_RTNACCESSPL_RELPL );
      vector<optAccessPlan *>::iterator it ;

      ossScopedLock _lock ( &_mutex, SHARED ) ;

      // check if the plan is in the list
      for ( it = _plans.begin() ; it != _plans.end() ; ++it )
      {
         // if the plan is already in cache, let's return without any change
         if ( *it == plan )
         {
            // decrease plan count
            if ( 1 == plan->decCount() )
            {
               _emptyAPNum.inc() ;
               _pSetEmptyAPNum->inc() ;
               _pTotalEmptyAPNum->inc() ;
            }
            goto done ;
         }
      }
      PD_LOG( PDERROR, "Access plan[%s] is not found in vector",
              plan->toString().c_str() ) ;
      // otherwise it's not in the list, let's simply delete the plan
      SDB_OSS_DEL plan ;

   done :
      PD_TRACE_EXIT ( SDB_RTNACCESSPL_RELPL ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPL_CLEARFAST, "_rtnAccessPlanList::_clearFast" )
   UINT32 _rtnAccessPlanList::_clearFast( INT32 expectNum )
   {
      UINT32 cleanNum = 0 ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPL_CLEARFAST ) ;

      if ( hasEmptyAP() )
      {
         vector<optAccessPlan *>::reverse_iterator rit ;
         for ( rit = _plans.rbegin() ; rit != _plans.rend(); )
         {
            if ( (*rit)->getCount() == 0 )
            {
               ++cleanNum ;
               --expectNum ;
               _emptyAPNum.dec() ;
               _pSetEmptyAPNum->dec() ;
               _pTotalEmptyAPNum->dec() ;

               SDB_OSS_DEL (*rit) ;
               _plans.erase( (++rit).base() ) ;

               if ( expectNum <= 0 )
               {
                  break ;
               }
            }
            else
            {
               ++rit ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPL_CLEARFAST ) ;
      return cleanNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPL_CLEAR, "_rtnAccessPlanList::clear" )
   void _rtnAccessPlanList::clear ( UINT32 &cleanNum )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPL_CLEAR );
      cleanNum = 0 ;

      if ( hasEmptyAP() )
      {
         vector<optAccessPlan *>::iterator it ;

         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

         // check if the plan is in the list
         for ( it = _plans.begin() ; it != _plans.end() ; )
         {
            // if so let's decrease the count
            if ( (*it)->getCount() == 0 )
            {
               ++cleanNum ;
               _emptyAPNum.dec() ;
               _pSetEmptyAPNum->dec() ;
               _pTotalEmptyAPNum->dec() ;

               SDB_OSS_DEL (*it) ;
               // in vector, erase returns the next valid iterator, or
               // vector::end()
               it = _plans.erase( it ) ;
            }
            else
            {
               ++it ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPL_CLEAR );
   }

   /*
      _rtnAccessPlanSet implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_INVALIDATE, "_rtnAccessPlanSet::invalidate" )
   void _rtnAccessPlanSet::invalidate( UINT32 &cleanNum )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_INVALIDATE ) ;
      cleanNum = 0 ;
      MAP_CODE_2_LIST_IT it ;

      ossScopedLock _lock ( &_mutex, SHARED ) ;

      for ( it = _planLists.begin() ; it != _planLists.end() ; )
      {
         UINT32 tempClean = 0 ;
         rtnAccessPlanList *list = (*it).second ;
         list->invalidate( tempClean ) ;
         ++it ;
         cleanNum += tempClean ;
         _totalNum.sub( tempClean ) ;
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPS_INVALIDATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_ADDPLAN, "_rtnAccessPlanSet::addPlan" )
   BOOLEAN _rtnAccessPlanSet::addPlan( optAccessPlan *plan,
                                       const rtnAPContext &context,
                                       SINT32 &incSize )
   {
      BOOLEAN hasAdd = FALSE ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_ADDPLAN ) ;

      MAP_CODE_2_LIST_IT itr ;
      rtnAccessPlanList *pList = NULL ;
      INT32 inc = 0 ;

      incSize = 0 ;
      hasAdd = FALSE ;

      /// first check set exist
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;

         itr = _planLists.find( plan->hash() ) ;
         if ( itr != _planLists.end() )
         {
            pList = itr->second ;
            hasAdd = pList->addPlan( plan, context, inc ) ;
            _accessList.reHeader( pList, TRUE ) ;
            _totalNum.add( inc ) ;
            incSize += inc ;
            goto done ;
         }
      }

      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

         itr = _planLists.find( plan->hash() ) ;
         if ( itr == _planLists.end() )
         {
            pList = SDB_OSS_NEW _rtnAccessPlanList( _apm,
                                                    &_emptyAPNum,
                                                    _pTotalEmptyAPNum,
                                                    plan->hash() ) ;
            if ( !pList )
            {
               PD_LOG ( PDWARNING, "Failed to allocate memory for list" ) ;
               plan->setAPM( NULL ) ;
               goto done ;
            }
            _planLists[ plan->hash() ] = pList ;
            _accessList.insert( pList, FALSE ) ;
         }
         else
         {
            pList = itr->second ;
            _accessList.reHeader( pList, FALSE ) ;
         }

         hasAdd = pList->addPlan( plan, context, inc ) ;
         _totalNum.add( inc ) ;
         incSize += inc ;
         goto done ;
      }

   done:
      PD_TRACE_EXIT( SDB__RTNACCESSPS_ADDPLAN ) ;
      return hasAdd ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_GETPLAN, "_rtnAccessPlanSet::getPlan" )
   optAccessPlan* _rtnAccessPlanSet::getPlan ( const BSONObj &query,
                                               const BSONObj &orderBy,
                                               const BSONObj &hint,
                                               rtnAPContext &context )
   {
      optAccessPlan *plan = NULL ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_GETPLAN ) ;
      MAP_CODE_2_LIST_IT it ;

      UINT32 hash = optAccessPlan::hash( query, orderBy, hint ) ;

      ossScopedLock _lock ( &_mutex, SHARED ) ;

      it = _planLists.find ( hash ) ;

      if ( _planLists.end() != it )
      {
         rtnAccessPlanList *pList = it->second ;
         plan = pList->getPlan( query, orderBy, hint, context ) ;
         _accessList.reHeader( pList, TRUE ) ;
      }

      PD_TRACE_EXIT( SDB__RTNACCESSPS_GETPLAN ) ;
      return plan ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_RELPL, "_rtnAccessPlanSet::releasePlan" )
   void _rtnAccessPlanSet::releasePlan ( optAccessPlan *plan )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_RELPL ) ;
      SDB_ASSERT ( plan, "plan can't be NULL" ) ;
      UINT32 hash = plan->hash () ;
      MAP_CODE_2_LIST_IT it ;

      ossScopedLock _lock ( &_mutex, SHARED ) ;

      it = _planLists.find( hash ) ;

      if ( it == _planLists.end() )
      {
         // if the plan is not in the plan list
         PD_LOG( PDERROR, "Access plan[%s] is not found is lists",
                 plan->toString().c_str() ) ;
         SDB_OSS_DEL plan ;
      }
      else
      {
         it->second->releasePlan ( plan ) ;
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPS_RELPL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_CLEAR_FAST, "_rtnAccessPlanSet::_clearFast" )
   UINT32 _rtnAccessPlanSet::_clearFast( INT32 expectNum )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_CLEAR_FAST ) ;

      rtnAccessPlanList *pList = NULL ;
      UINT32 totalClearNum = 0 ;
      UINT32 clearNum = 0 ;

      if ( hasEmptyAP() )
      {
         pList = _accessList.tail() ;
         while( pList )
         {
            rtnAccessPlanList *pTmp = pList ;
            pList = _accessList.prev( pTmp ) ;

            if ( ( clearNum = pTmp->_clearFast( expectNum ) ) > 0 )
            {
               _totalNum.sub( clearNum ) ;
               totalClearNum += clearNum ;
               expectNum -= clearNum ;
            }

            if ( 0 == pTmp->size() )
            {
               _planLists.erase( pTmp->hash() ) ;
               _accessList.remove( pTmp, FALSE ) ;
               SDB_OSS_DEL pTmp ;
            }

            if ( expectNum <= 0 )
            {
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__RTNACCESSPS_CLEAR_FAST ) ;
      return totalClearNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPS_CLEAR, "_rtnAccessPlanSet::clear" )
   void _rtnAccessPlanSet::clear( UINT32 &cleanNum, BOOLEAN full )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPS_CLEAR ) ;
      cleanNum = 0 ;
      rtnAccessPlanList *list = NULL ;
      MAP_CODE_2_LIST_IT it ;

      if ( hasEmptyAP() )
      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
         UINT32 threshold = _totalNum.peek() * RTN_APS_DFT_OCCUPY_PCT ;

         for ( it = _planLists.begin() ; it != _planLists.end() ; )
         {
            UINT32 tempClean = 0 ;
            list = it->second ;
            list->clear( tempClean ) ;
            if ( tempClean > 0 )
            {
               _totalNum.sub( tempClean ) ;
               cleanNum += tempClean ;
            }

            // clear empty list
            if ( list->size() == 0 )
            {
               _accessList.remove( list, FALSE ) ;
               SDB_OSS_DEL list ;
               _planLists.erase( it++ ) ;
            }
            else
            {
               ++it ;
            }

            // don't run too aggresive, try to clear things to 75%
            if ( !full && _totalNum.peek() < threshold )
            {
               break ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPS_CLEAR ) ;
   }

   /*
      _rtnAccessPlanManager implement
   */
   _rtnAccessPlanManager::_rtnAccessPlanManager( _dmsStorageUnit *su )
   :_totalNum( 0 ),
    _emptyAPNum( 0 ),
    _su( su ),
    _bucketsNum( 0 )
   {
      _bucketsNum = pmdGetOptionCB()->getPlanBuckets() ;
   }

   _rtnAccessPlanManager::~_rtnAccessPlanManager()
   {
      SDB_ASSERT( _emptyAPNum.peek() >= 0 &&
                  _emptyAPNum.peek() <= _totalNum.peek(),
                  "0 <= _emptyAPNum <= _totalNum" ) ;

      PLAN_SETS_IT it ;
      for ( it = _planSets.begin() ; it != _planSets.end() ; ++it )
      {
         SDB_OSS_DEL it->second ;
      }
      _planSets.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_INVALIDATEPL, "_rtnAccessPlanManager::invalidatePlans" )
   void _rtnAccessPlanManager::invalidatePlans ( const CHAR *collectionName )
   {
      UINT32 cleanNum = 0 ;
      PLAN_SETS_IT it ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPLMAN_INVALIDATEPL ) ;

      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

      it = _planSets.find( collectionName ) ;
      if ( it != _planSets.end() )
      {
         rtnAccessPlanSet *planset = (*it).second ;
         _accessList.remove( planset, FALSE ) ;
         planset->invalidate( cleanNum ) ;
         _totalNum.sub( cleanNum ) ;
         if ( planset->size() == 0 )
         {
            _planSets.erase( it++ ) ;
            SDB_OSS_DEL planset ;
         }
         else
         {
            ++it ;
         }
      }
      PD_TRACE_EXIT ( SDB__RTNACCESSPLMAN_INVALIDATEPL );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_ADDPLAN, "_rtnAccessPlanManager::addPlan" )
   BOOLEAN _rtnAccessPlanManager::addPlan( optAccessPlan *plan,
                                           const rtnAPContext &context,
                                           SINT32 &incSize )
   {
      BOOLEAN hasAdd = FALSE ;
      PD_TRACE_ENTRY( SDB__RTNACCESSPLMAN_ADDPLAN ) ;

      PLAN_SETS_IT itr ;
      _rtnAccessPlanSet *pSet = NULL ;
      INT32 inc = 0 ;

      incSize = 0 ;
      hasAdd = FALSE ;

      /// first check set exist
      if ( _totalNum.fetch() < _bucketsNum )
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;

         itr = _planSets.find( plan->getName() ) ;
         if ( itr != _planSets.end() )
         {
            pSet = itr->second ;
            hasAdd = pSet->addPlan( plan, context, inc ) ;
            _totalNum.add( inc ) ;
            incSize += inc ;
            _accessList.reHeader( pSet, TRUE ) ;
            goto done ;
         }
      }

      /// then add set
      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

         /// when ap is full in the set, need to clean
         if ( _totalNum.peek() >= _bucketsNum )
         {
            INT32 expectNum = _totalNum.peek() - _bucketsNum + 1 ;
            UINT32 cleanNum = _clearFast( expectNum ) ;
            if ( cleanNum < (UINT32)expectNum )
            {
               /// set is full
               plan->setAPM( NULL ) ;
               goto done ;
            }
            incSize -= cleanNum ;
         }

         itr = _planSets.find( plan->getName() ) ;
         if ( itr == _planSets.end() )
         {
            pSet = SDB_OSS_NEW _rtnAccessPlanSet( _su,
                                                  plan->getName(),
                                                  this,
                                                  &_emptyAPNum ) ;
            if ( !pSet )
            {
               PD_LOG ( PDWARNING, "Failed to allocate memory for set" ) ;
               plan->setAPM( NULL ) ;
               goto done ;
            }
            _planSets[ pSet->getName() ] = pSet ;
            _accessList.insert( pSet, FALSE ) ;
         }
         else
         {
            pSet = itr->second ;
            _accessList.reHeader( pSet, FALSE ) ;
         }

         hasAdd = pSet->addPlan( plan, context, inc ) ;
         _totalNum.add( inc ) ;
         incSize += inc ;
         goto done ;
      }

   done:
      PD_TRACE_EXIT( SDB__RTNACCESSPLMAN_ADDPLAN ) ;
      return hasAdd ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_GETPLAN, "_rtnAccessPlanManager::getPlan" )
   INT32 _rtnAccessPlanManager::getPlan ( const BSONObj &query,
                                          const BSONObj &orderBy,
                                          const BSONObj &hint,
                                          const CHAR *collectionName,
                                          optAccessPlan **out )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPLMAN_GETPLAN ) ;

      SDB_ASSERT ( collectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( out, "out can't be NULL" ) ;
      SINT32 incCount = 0 ;
      rtnAPContext context ;

      if ( 0 == _bucketsNum )
      {
         rc = createNewPlan( _su, collectionName,
                             query, orderBy,
                             hint, out ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Create new plan[%s, query:%s] failed, rc: %d",
                    collectionName, query.toString().c_str(), rc ) ;
            goto error ;
         }
         goto done ;
      }
      else
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;
         PLAN_SETS_IT itr = _planSets.find ( collectionName ) ;

         if ( _planSets.end() != itr )
         {
            _rtnAccessPlanSet *pSet = itr->second ;
            *out = pSet->getPlan( query, orderBy, hint, context ) ;
            if ( *out )
            {
               _accessList.reHeader( pSet, TRUE ) ;
               goto done ;
            }
         }
      }

      /// Need to add plan
      rc = createNewPlan( _su, collectionName, query, orderBy,
                          hint, out ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create new plan[%s, query:%s] failed, rc: %d",
                 collectionName, query.toString().c_str(), rc ) ;
         goto error ;
      }
      addPlan( *out, context, incCount ) ;

   done :
      PD_TRACE_EXITRC ( SDB__RTNACCESSPLMAN_GETPLAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_RELPL, "_rtnAccessPlanManager::releasePlan" )
   void _rtnAccessPlanManager::releasePlan( optAccessPlan *plan )
   {
      PD_TRACE_ENTRY ( SDB__RTNACCESSPLMAN_RELPL );
      SDB_ASSERT ( plan, "plan can't be NULL" ) ;
      SDB_ASSERT ( plan->getAPM() == this,
                   "the owner of plan is not this APM" ) ;
      PLAN_SETS_IT it ;

      ossScopedLock _lock ( &_mutex, SHARED ) ;

      it = _planSets.find( plan->getName() ) ;

      if ( it == _planSets.end() )
      {
         PD_LOG( PDWARNING, "Access plan[%s] is not found in plan sets",
                 plan->toString().c_str() ) ;
         SDB_OSS_DEL plan ;
      }
      else
      {
         it->second->releasePlan( plan ) ;
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPLMAN_RELPL );
   }
   
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_CLEAR, "_rtnAccessPlanManager::clear" )
   void _rtnAccessPlanManager::clear ( BOOLEAN full )
   {
      UINT32 cleanNum = 0 ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPLMAN_CLEAR );
      PLAN_SETS_IT it ;

      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;

      UINT32 threshold = RTN_APM_DFT_OCCUPY_PCT * _totalNum.peek() ;

      for ( it = _planSets.begin() ; it != _planSets.end() ; )
      {
         rtnAccessPlanSet *planset = (*it).second ;
         planset->clear( cleanNum ) ;
         if ( cleanNum > 0 )
         {
            _totalNum.sub( cleanNum ) ;
         }

         // clear empty list
         if ( planset->size() == 0 )
         {
            _accessList.remove( planset, FALSE ) ;
            _planSets.erase( it++ ) ;
            SDB_OSS_DEL planset ;
         }
         else
         {
            ++it ;
         }

         if ( !full && _totalNum.peek() < threshold )
         {
            break ;
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPLMAN_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNACCESSPLMAN_CLEARFAST, "_rtnAccessPlanManager::_clearFast" )
   UINT32 _rtnAccessPlanManager::_clearFast( INT32 expectNum )
   {
      UINT32 totalCleanNum = 0 ;
      UINT32 cleanNum = 0 ;
      PD_TRACE_ENTRY ( SDB__RTNACCESSPLMAN_CLEARFAST );

      if ( hasEmptyAP() )
      {
         rtnAccessPlanSet *pSet = _accessList.tail() ;
         while( pSet )
         {
            rtnAccessPlanSet *pTmp = pSet ;
            pSet = _accessList.prev( pSet ) ;

            if ( ( cleanNum = pTmp->_clearFast( expectNum ) ) > 0 )
            {
               expectNum -= cleanNum ;
               totalCleanNum += cleanNum ;
               _totalNum.sub( cleanNum ) ;
            }

            // clear empty list
            if ( pTmp->size() == 0 )
            {
               _planSets.erase( pTmp->getName() ) ;
               _accessList.remove( pTmp, FALSE ) ;
               SDB_OSS_DEL pTmp ;
            }

            if ( expectNum <= 0 )
            {
               break ;
            }
         }
      }

      PD_TRACE_EXIT ( SDB__RTNACCESSPLMAN_CLEARFAST ) ;
      return totalCleanNum ;
   }

}

