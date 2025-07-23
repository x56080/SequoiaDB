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

   Source File Name = clsUniqueIDCheckJob.cpp

   Descriptive Name = CS/CL UniqueID Checking Job

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          06/08/2018  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsUniqueIDCheckJob.hpp"
#include "pmd.hpp"
#include "dmsStorageUnit.hpp"
#include "rtn.hpp"
#include "clsTrace.hpp"

namespace engine
{

   // outpu: BSONObj
   // [
   //    { "Name": "bar1", "UniqueID": 0 } ,
   //    { "Name": "bar2", "UniqueID": 0 }
   // ]
   static BSONObj clsSetUniqueID( const MON_CL_SIM_VEC& clVector, utilCLUniqueID setValue )
   {
      BSONArrayBuilder arrBuilder ;

      for ( MON_CL_SIM_VEC::const_iterator iter = clVector.begin() ;
            iter != clVector.end() ;
            ++ iter )
      {
         arrBuilder << BSON( FIELD_NAME_NAME << iter->_clname
                    << FIELD_NAME_UNIQUEID << (INT64)setValue ) ;
      }

      return arrBuilder.arr() ;
   }

   #define CLS_UNIQUEID_CHECK_INTERVAL ( OSS_ONE_SEC * 3 )

   /*
    *  _clsUniqueIDCheckJob implement
    */
   _clsUniqueIDCheckJob::_clsUniqueIDCheckJob()
   {
   }

   _clsUniqueIDCheckJob::~_clsUniqueIDCheckJob()
   {
   }

   BOOLEAN _clsUniqueIDCheckJob::muteXOn( const _rtnBaseJob *pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSUIDCHKJOB_DOIT, "_clsUniqueIDCheckJob::doit" )
   INT32 _clsUniqueIDCheckJob::doit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSUIDCHKJOB_DOIT ) ;

      pmdEDUCB *cb = eduCB() ;
      pmdKRCB* pKrcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = pKrcb->getDMSCB() ;
      SDB_DPSCB *pDpsCB = pKrcb->getDPSCB() ;
      shardCB* pShdMgr = sdbGetShardCB() ;
      clsDCMgr* pDcMgr = pShdMgr->getDCMgr() ;
      UINT64 loopCnt = 0 ;
      BOOLEAN isCataReady = FALSE ;

      PD_LOG( PDEVENT,
              "Start job[%s]: check cs/cl unique id by cs/cl name", name() ) ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() &&
              pmdIsPrimary() &&
              pDmsCB->nullCSUniqueIDCnt() > 0 )
      {
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
          */
         if ( loopCnt++ != 0 )
         {
            pmdEDUEvent event ;
            cb->waitEvent( event, CLS_UNIQUEID_CHECK_INTERVAL, TRUE ) ;
         }

         // 1. check if the cs/cl unique id on catalog have been generated.
         if ( !isCataReady )
         {
            rc = pShdMgr->updateDCBaseInfo() ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Job[%s]: "
                       "Update data center base info failed, rc: %d",
                       name(), rc ) ;
               continue ;
            }

            clsDCBaseInfo* pDcInfo = pDcMgr->getDCBaseInfo() ;
            if ( UTIL_UNIQUEID_NULL == pDcInfo->getCSUniqueHWM() )
            {
               continue ;
            }
         }
         isCataReady = TRUE ;

         // 2. loop each cs
         MON_CS_LIST csList ;
         MON_CS_LIST::const_iterator iterCS ;

         pDmsCB->dumpInfo( csList, FALSE ) ;

         for ( iterCS = csList.begin() ; iterCS != csList.end(); ++iterCS )
         {
            const _monCollectionSpace &cs = *iterCS ;
            utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
            BSONObj clInfoObj ;

            PD_LOG( PDDEBUG,
                    "Job[%s]: Check collection space[%s]",
                    name(), cs._name ) ;

            if ( PMD_IS_DB_DOWN() ||
                 cb->isForced() ||
                 !pmdIsPrimary() )
            {
               break ;
            }

            // we only need to operate cs which unique id = 0
            if ( cs._csUniqueID != UTIL_UNIQUEID_NULL )
            {
               continue ;
            }

            // update catalog info
            rc = pShdMgr->rGetCSInfo( cs._name, csUniqueID,
                                      NULL, NULL, NULL, &clInfoObj ) ;
            if ( rc != SDB_OK )
            {
               PD_LOG( PDWARNING,
                       "Job[%s]: Update cs[%s] catalog info, rc: %d. "
                       "CS doesn't exist in catalog",
                       name(), cs._name, rc ) ;
               if ( rc != SDB_DMS_CS_NOTEXIST )
               {
                  continue ;
               }
            }

            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               rc = rtnChangeUniqueID( cs._name, UTIL_CSUNIQUEID_LOCAL,
                                       clsSetUniqueID( cs._collections,
                                                       UTIL_CLUNIQUEID_LOCAL ),
                                       cb, pDmsCB, pDpsCB ) ;
            }
            else
            {
               rc = rtnChangeUniqueID( cs._name, csUniqueID,
                                       clInfoObj,
                                       cb, pDmsCB, pDpsCB ) ;
            }

            if ( rc )
            {
               PD_LOG( PDWARNING,
                       "Job[%s]: Change cs[%s] unique id failed, rc: %d",
                       name(), cs._name, rc ) ;
               continue ;
            }

         }// end for

      }// end while

      PD_LOG( PDEVENT, "Stop job[%s]", name() ) ;

      PD_TRACE_EXITRC( SDB__CLSUIDCHKJOB_DOIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTUIDCHKJOB, "startUniqueIDCheckJob" )
   INT32 startUniqueIDCheckJob ( EDUID* pEDUID )
   {
      PD_TRACE_ENTRY( SDB_STARTUIDCHKJOB ) ;

      INT32 rc = SDB_OK ;
      clsUniqueIDCheckJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsUniqueIDCheckJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_STOP_CONT, pEDUID ) ;

   done:
      PD_TRACE_EXITRC( SDB_STARTUIDCHKJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   #define CLS_NAME_CHECK_PRIMARY_INTERAL    ( 100 )           /// ms
   #define CLS_NAME_CHECK_DC_INTERAL         ( 100 )           /// ms
   #define CLS_NAME_CHECK_INTERVAL           ( OSS_ONE_SEC )   /// ms

   /*
    *  _clsNameCheckJob implement
    */
   _clsNameCheckJob::_clsNameCheckJob( UINT64 opID )
   : _opID( opID )
   {
      _pShdMgr = sdbGetShardCB() ;
      _pFreezeWindow = _pShdMgr->getFreezingWindow() ;
      _hasBlockGlobal = FALSE ;
   }

   _clsNameCheckJob::~_clsNameCheckJob()
   {
   }

   void _clsNameCheckJob::_onAttach()
   {
      _hasBlockGlobal = TRUE ;
   }

   void _clsNameCheckJob::_onDetach()
   {
      /// unregister collections and whole
      if ( _hasBlockGlobal )
      {
         _pFreezeWindow->unregisterCL( NULL, NULL, _opID ) ;
      }

      map<string, string>::iterator it = _mapRegisterCL.begin() ;
      while( it != _mapRegisterCL.end() )
      {
         _pFreezeWindow->unregisterCL( it->first.c_str(),
                                       it->second.c_str(),
                                       _opID ) ;
         ++it ;
      }
      _mapRegisterCL.clear() ;
   }

   BOOLEAN _clsNameCheckJob::muteXOn( const _rtnBaseJob *pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNAMECHKJOB_DOIT, "_clsNameCheckJob::doit" )
   INT32 _clsNameCheckJob::doit()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSNAMECHKJOB_DOIT ) ;

      pmdEDUCB *cb = eduCB() ;
      pmdKRCB* pKrcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = pKrcb->getDMSCB() ;
      clsDCMgr* pDcMgr = _pShdMgr->getDCMgr() ;
      UINT64 loopCnt = 0 ;
      MON_CS_SIM_LIST csSet ;
      vector<monCSSimple> csList ;

      PD_LOG( PDEVENT, "Start job[%s]: check cs/cl name by unique id",
              name() ) ;

      // wait _clsVSPrimary::active() set primary
      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() )
      {
         if ( loopCnt++ != 0 )
         {
            pmdEDUEvent event ;
            cb->waitEvent( event, CLS_NAME_CHECK_PRIMARY_INTERAL, TRUE ) ;
         }

         if ( pmdIsPrimary() )
         {
            break ;
         }
      }

      /// 1. check if the cs/cl unique id on catalog have been generated.
      loopCnt = 0 ;
      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() &&
              pmdIsPrimary() )
      {
         if ( loopCnt++ != 0 )
         {
            pmdEDUEvent event ;
            cb->waitEvent( event, CLS_NAME_CHECK_INTERVAL, TRUE ) ;
         }

         rc = _pShdMgr->updateDCBaseInfo() ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Job[%s]: "
                    "Update data center base info failed, rc: %d",
                    name(), rc ) ;
            continue ;
         }

         clsDCBaseInfo* pDcInfo = pDcMgr->getDCBaseInfo() ;
         if ( UTIL_UNIQUEID_NULL != pDcInfo->getCSUniqueHWM() )
         {
            break ;
         }
      }

      if ( !pmdIsPrimary() )
      {
         goto done ;
      }

      /// 2. get all cs
      pDmsCB->dumpInfo( csSet, FALSE, TRUE ) ;
      for ( MON_CS_SIM_LIST::iterator it = csSet.begin() ;
            it!= csSet.end() ;
            it++ )
      {
         csList.push_back( *it ) ;
      }

      /// 3. loop all cs, util all cs done
      _renameCSCL( csList, FALSE ) ;

      // after first traversal, block all failed cl,
      // instead of blocking the global
      _registerCLs( csList ) ;
      _pFreezeWindow->unregisterCL( NULL, NULL, _opID ) ;
      _hasBlockGlobal = FALSE ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() &&
              pmdIsPrimary() &&
              !csList.empty() )
      {
         _renameCSCL( csList, TRUE ) ;

         // wait for a litter time
         pmdEDUEvent event ;
         cb->waitEvent( event, CLS_NAME_CHECK_INTERVAL, TRUE ) ;
      }

   done:
      if ( _hasBlockGlobal )
      {
         _pFreezeWindow->unregisterCL( NULL, NULL, _opID ) ;
         _hasBlockGlobal = FALSE ;
      }
      PD_LOG( PDEVENT, "Stop job[%s]", name() ) ;
      PD_TRACE_EXITRC( SDB__CLSNAMECHKJOB_DOIT, rc ) ;
      return rc ;
   }

   // csList is both input parameter and output parameter
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNAMECHKJOB__RENAMECSCL, "_clsNameCheckJob::_renameCSCL" )
   INT32 _clsNameCheckJob::_renameCSCL( vector<monCSSimple>& csList,
                                        BOOLEAN unregCL )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSNAMECHKJOB__RENAMECSCL ) ;

      pmdKRCB* pKrcb = pmdGetKRCB() ;
      SDB_DMSCB *pDmsCB = pKrcb->getDMSCB() ;
      SDB_DPSCB *pDpsCB = pKrcb->getDPSCB() ;
      pmdEDUCB *cb = eduCB() ;

      vector<monCSSimple>::iterator iterCS = csList.begin() ;
      while ( iterCS != csList.end() )
      {
         monCSSimple &cs = *iterCS ;
         utilCSUniqueID csUniqueID = cs._csUniqueID ;
         BSONObj clInfoObj ;
         string newCSName ;
         MAP_CLID_NAME clInfoInCata ;

         PD_LOG( PDDEBUG,
                 "Job[%s]: Check cs[name: %s, id: %u]",
                 name(), cs._name, csUniqueID ) ;

         if ( PMD_IS_DB_DOWN() ||
              cb->isForced() ||
              !pmdIsPrimary() )
         {
            break ;
         }

         /// we only need to operate cs which has valid unique id
         if ( ! UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
         {
            iterCS = csList.erase( iterCS ) ;
            continue ;
         }

         /// update catalog info
         rc = _pShdMgr->rGetCSInfo( cs._name, csUniqueID, NULL, NULL,
                                    NULL, &clInfoObj, &newCSName ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            PD_LOG( PDWARNING,
                    "Job[%s]: update cs[name: %s, id: %u] catalog info, "
                    "rc: %d. CS doesn't exist in catalog",
                    name(), cs._name, csUniqueID, rc ) ;
            iterCS = csList.erase( iterCS ) ;
            continue ;
         }
         else if ( rc )
         {
            iterCS++ ;
            PD_LOG( PDWARNING,
                    "Job[%s]: update cs[name: %s, id: %u] catalog info, "
                    "rc: %d", name(), cs._name, csUniqueID, rc ) ;
            continue ;
         }

         /// rename collections
         clInfoInCata = utilBson2ClIdName( clInfoObj ) ;

         MON_CL_SIM_VEC::iterator itrCL = cs._clList.begin() ;
         while ( itrCL!= cs._clList.end() )
         {
            utilCLUniqueID clUniqueID = itrCL->_clUniqueID ;
            string clShortName = itrCL->_clname ;
            string clFullName = itrCL->_name ;

            PD_LOG( PDDEBUG,
                    "Job[%s]: Check cl[name: %s, id: %llu]",
                    name(), clFullName.c_str(), clUniqueID ) ;

            if ( ! UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
            {
               itrCL = cs._clList.erase( itrCL ) ;
               if ( unregCL )
               {
                  _unregisterCL( clFullName.c_str() ) ;
               }
               continue ;
            }

            MAP_CLID_NAME::iterator it = clInfoInCata.find( clUniqueID ) ;
            if ( it == clInfoInCata.end() )
            {
               // the cl is not exist in catalog
               itrCL = cs._clList.erase( itrCL ) ;
               if ( unregCL )
               {
                  _unregisterCL( clFullName.c_str() ) ;
               }
               continue ;
            }

            // find out the cl in catalog
            string clShortNameInCata = it->second ;
            if ( clShortNameInCata == clShortName )
            {
               // data clname is the same with cata clname
               itrCL = cs._clList.erase( itrCL ) ;
               if ( unregCL )
               {
                  _unregisterCL( clFullName.c_str() ) ;
               }
               continue ;
            }

            // need to rename
            rc = rtnRenameCollectionCommand( newCSName.c_str(),
                                             clShortName.c_str(),
                                             clShortNameInCata.c_str(),
                                             cb, pDmsCB, pDpsCB ) ;
            if ( SDB_OK == rc )
            {
               // rename succeed
               itrCL = cs._clList.erase( itrCL ) ;
               if ( unregCL )
               {
                  _unregisterCL( clFullName.c_str() ) ;
               }
               continue ;
            }
            else
            {
               // ignore error, continue next cl
               itrCL++ ;
               PD_LOG( PDWARNING,"Job[%s]: Rename cl"
                       "[name: %s, id: %llu] failed, rc: %d",
                       name(), clShortName.c_str(), clUniqueID, rc ) ;
               continue ;
            }
         }// end for all cl

         /// rename collection space
         if ( !cs._clList.empty() )
         {
            iterCS++ ;
            continue ;
         }

         // rename all collections succeed, then begin to rename cs
         if ( 0 == ossStrcmp( cs._name, newCSName.c_str() ) )
         {
            // data csname is the same with cata csname
            iterCS = csList.erase( iterCS ) ;
            continue ;
         }

         // need to rename cs
         rc = rtnRenameCollectionSpaceCommand( cs._name,
                                               newCSName.c_str(),
                                               cb, pDmsCB, pDpsCB ) ;
         if ( SDB_OK == rc )
         {
            iterCS = csList.erase( iterCS ) ;
            continue ;
         }
         else
         {
            // ignore error, continue next cs
            PD_LOG( PDWARNING,
                    "Job[%s]: Rename cs[name: %s, id: %u] failed, "
                    "rc: %d", name(), cs._name, csUniqueID, rc ) ;
            iterCS++ ;
            continue ;
         }

      }

      PD_TRACE_EXITRC( SDB__CLSNAMECHKJOB__RENAMECSCL, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNAMECHKJOB__REGCLS, "_clsNameCheckJob::_registerCLs" )
   void _clsNameCheckJob::_registerCLs( const vector<monCSSimple>& csList )
   {
      PD_TRACE_ENTRY( SDB__CLSNAMECHKJOB__REGCLS ) ;

      catAgent* pCatAgent = _pShdMgr->getCataAgent() ;

      vector<monCSSimple>::const_iterator itCS = csList.begin() ;
      for ( ; itCS != csList.end() ; itCS++ )
      {
         MON_CL_SIM_VEC::const_iterator itCL = itCS->_clList.begin() ;
         for ( ; itCL != itCS->_clList.end() ; itCL++ )
         {
            string clName = itCL->_name ;
            string mainCLName ;
            clsCatalogSet *pSet = NULL ;

            // get or update
            if ( SDB_OK == _pShdMgr->getAndLockCataSet( clName.c_str(),
                                                        &pSet, TRUE ) )
            {
               // get main cl name
               mainCLName = pSet->getMainCLName() ;
               pCatAgent->release_r() ;
            }
            /// when can not get the collection info, ignore the main cl
            /// Block collection and main-collection ( if have ) together
            _pFreezeWindow->registerCL( clName.c_str(),
                                        mainCLName.c_str(),
                                        _opID ) ;
            _mapRegisterCL[ clName ] = mainCLName ;
         }
      }

      PD_TRACE_EXIT( SDB__CLSNAMECHKJOB__REGCLS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSNAMECHKJOB__UNREGCL, "_clsNameCheckJob::_unregisterCL" )
   void _clsNameCheckJob::_unregisterCL( const string& clName )
   {
      PD_TRACE_ENTRY( SDB__CLSNAMECHKJOB__UNREGCL ) ;

      string mainCLName ;
      map<string, string>::iterator it ;

      it = _mapRegisterCL.find( clName ) ;
      if( it != _mapRegisterCL.end() )
      {
         mainCLName = it->second ;
      }

      _pFreezeWindow->unregisterCL( clName.c_str(),
                                    mainCLName.c_str(),
                                    _opID ) ;

      _mapRegisterCL.erase( clName ) ;

      PD_TRACE_EXIT( SDB__CLSNAMECHKJOB__UNREGCL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STARTNAMECHKJOB, "startNameCheckJob" )
   INT32 startNameCheckJob ( EDUID* pEDUID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_STARTNAMECHKJOB ) ;

      clsNameCheckJob *pJob = NULL ;
      shardCB* pShdMgr = sdbGetShardCB() ;
      clsFreezingWindow* pFreezeWindow = pShdMgr->getFreezingWindow() ;
      UINT64 opID = 0 ;

      pFreezeWindow->registerCL( NULL, NULL, opID ) ;

      pJob = SDB_OSS_NEW clsNameCheckJob( opID ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_STOP_CONT, pEDUID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_STARTNAMECHKJOB, rc ) ;
      return rc ;
   error:
      pFreezeWindow->unregisterCL( NULL, NULL, opID ) ;
      goto done ;
   }

}
