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

   Source File Name = clsRecycleRecordJob.cpp

   Descriptive Name = Recycle Record Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dms.hpp"
#include "dmsCB.hpp"
#include "dpsTransCB.hpp"
#include "dmsStorageUnit.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsRecycleRecordJob.hpp"

namespace engine
{
   class _clsRecycleCLFilter : public _dmsCLFilter
   {
   public:
      _clsRecycleCLFilter( UINT32 delayTimeMs )
      {
         _delayTimeMs = delayTimeMs ;
      }

      virtual ~_clsRecycleCLFilter() {}

      virtual BOOLEAN filter( const dmsMBStatInfo &stat )
      {
         static const UINT64 MIN_REC_TO_BE_RECYCLED = 1000 ;
         if ( 0 == _delayTimeMs ) // feature is disabled
         {
            return FALSE ;
         }
         return ( stat._totalDeletingRecords >= MIN_REC_TO_BE_RECYCLED &&
                  pmdGetTickSpanTime( stat._lastWriteTick ) > _delayTimeMs ) ;
      }

   private:
      UINT64 _delayTimeMs ;
   } ;

   /*
    *  _clsRecycleRecordJob implement
    */
   _clsRecycleRecordJob::_clsRecycleRecordJob ()
   {
   }

   _clsRecycleRecordJob::~_clsRecycleRecordJob ()
   {
   }

   INT32 _clsRecycleRecordJob::doit ()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdEDUMgr *pEduMgr = krcb->getEDUMgr() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      dpsTransCB *pTransCB = krcb->getTransCB() ;
      pmdOptionsCB *pOptionCB = pmdGetOptionCB() ;
      UINT64 delayTimeMs = 0 ;
      pmdEDUCB *cb = eduCB() ;
      UINT64 waitMs = 0 ;
      pmdEDUEvent event ;
      MON_CL_SIM_LIST clList ;

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
         */
         pEduMgr->waitEDU( cb ) ;
         cb->resetDisconnect() ;

         delayTimeMs = pOptionCB->getRecordRecycleDelay() * 60 * OSS_ONE_SEC ;
         cb->waitEvent( event, OSS_ONE_SEC ) ;
         waitMs += OSS_ONE_SEC ;

         // Check stop signal first
         if ( cb->isInterrupted() )
         {
            continue ;
         }

         if ( 0 == delayTimeMs || // This feature was disabled, do nothing
              waitMs < delayTimeMs )
         {
            continue ;
         }

         /// set edu active
         pEduMgr->activateEDU( cb ) ;
         waitMs = 0 ;

         // do it
         {
            _clsRecycleCLFilter filter( delayTimeMs ) ;
            pDmsCB->dumpInfo( clList, TRUE, FALSE, &filter ) ;
         }
         if ( !clList.empty() )
         {
            _doRecycleRecordJobs( cb, pDmsCB, pTransCB, clList ) ;
            cb->incEventCount( 1 ) ;
         }

         /// release mem
         cb->shrink() ;
      }
      return SDB_OK ;
   }

   void _clsRecycleRecordJob::_addCL2List( MON_CL_SIM_LIST &clList,
                                           const monCLSimple &clInfo )
   {
      try
      {
         clList.insert( clInfo ) ;
      }
      catch ( const std::exception &e )
      {
         PD_LOG( PDWARNING, "Failed to add collection info into list, "
                 "exception: %s", e.what() ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYCLERECJOB__DORECYCLERECJOBS, "_clsRecycleRecordJob::_doRecycleRecordJobs" )
   void _clsRecycleRecordJob::_doRecycleRecordJobs( pmdEDUCB *pEduCB,
                                                    SDB_DMSCB *pDmsCB,
                                                    dpsTransCB *pTransCB,
                                                    MON_CL_SIM_LIST& clList )
   {
      static const INT32 MAX_RETRY_TIMES = 10 ;
      static const INT32 RETRY_INTERVAL = 30 * OSS_ONE_SEC ;

      PD_TRACE_ENTRY( SDB__CLSRECYCLERECJOB__DORECYCLERECJOBS ) ;

      INT32 rc = SDB_OK ;
      INT32 retryTimes = 0 ;
      MON_CL_SIM_LIST retryList ;
      MON_CL_SIM_LIST::iterator it ;
      pmdEDUEvent event ;
      BOOLEAN hasUserWrite = FALSE ;
      INT32 lockFailTimes = 0 ;
      INT32 userWriteTimes = 0 ;

      while ( !clList.empty() && retryTimes++ < MAX_RETRY_TIMES )
      {
         for ( it = clList.begin() ; it != clList.end() ; ++it )
         {
            rc = _doRecycleRecordJob( pEduCB, pDmsCB, pTransCB, *it,
                                      hasUserWrite ) ;
            if ( SDB_LOCK_FAILED == rc || hasUserWrite )
            {
               if ( SDB_LOCK_FAILED == rc )
               {
                  ++lockFailTimes ;
               }
               else if ( hasUserWrite )
               {
                  ++userWriteTimes ;
               }
               _addCL2List( retryList, *it ) ;
            }
         }
         clList.clear() ;
         for ( it = retryList.begin() ; it != retryList.end() ; ++it )
         {
            _addCL2List( clList, *it ) ;
         }
         retryList.clear() ;

         if ( !clList.empty() )
         {
            pEduCB->waitEvent( event, RETRY_INTERVAL ) ;
         }
      }

      if ( !clList.empty() )
      {
         // Quit these collections. Wait for the next dump
         clList.clear() ;
      }

      if ( lockFailTimes > 0 || userWriteTimes > 0 )
      {
         PD_LOG( PDINFO, "Retry recycling records %d times for lock failure "
                 "and %d times for user write operations.", lockFailTimes,
                 userWriteTimes ) ;
      }

      PD_TRACE_EXIT( SDB__CLSRECYCLERECJOB__DORECYCLERECJOBS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSRECYCLERECJOB__DORECYCLERECJOB, "_clsRecycleRecordJob::_doRecycleRecordJob" )
   INT32 _clsRecycleRecordJob::_doRecycleRecordJob( pmdEDUCB *pEduCB,
                                                    SDB_DMSCB *pDmsCB,
                                                    dpsTransCB *pTransCB,
                                                    const monCLSimple& clInfo,
                                                    BOOLEAN &hasUserWrite )
   {
      static const UINT64 CHECK_WRITE_INTERVAL = 1000 ; // ms

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSRECYCLERECJOB__DORECYCLERECJOB ) ;

      dmsStorageUnit *pSu = NULL ;
      dmsMBContext *pContext = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      INT32 deletedCount = 0 ;
      UINT64 startTick = pmdGetDBTick() ;
      UINT64 lastCheckTick = 0 ;
      INT32 lastWriteCount = 0 ;

      hasUserWrite = FALSE ;

      if ( SDB_OK != pDmsCB->nameToSUAndLock( clInfo._csname, suID, &pSu ) || !pSu )
      {
         /// cs not exist
         goto done ;
      }

      if ( SDB_OK != pSu->data()->getMBContext( &pContext, clInfo._blockID,
                                                clInfo._logicalID, DMS_INVALID_CLID ) )
      {
         /// cl not exist
         goto done ;
      }

      lastCheckTick = startTick ;
      lastWriteCount = pContext->mbStat()->_crudCB._totalDataWrite ;

      while ( SDB_OK == rc )
      {
         rc = _deleteFirstDeletingRecord( pEduCB, pTransCB, pSu,
                                          pContext, clInfo ) ;
         if ( SDB_DMS_RECORD_NOTEXIST == rc || // no more record to be deleted
              SDB_DMS_NOTEXIST == rc || // cl was truncated or dropped
              SDB_LOCK_FAILED == rc ) // wait for the next time
         {
            PD_LOG( PDDEBUG, "Stop recycle cl[%s] records for rc: %d",
                    clInfo._name, rc ) ;
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                    "cl: %s, rc: %d", clInfo._name, rc ) ;
            goto error ;
         }

         ++ deletedCount ;

         // In each Interval, sleep to release the lock and pause this job
         // if user has write operations to yield IO resource.
         if ( pmdGetTickSpanTime( lastCheckTick ) > CHECK_WRITE_INTERVAL )
         {
            INT32 curWriteCount = pContext->mbStat()->_crudCB._totalDataWrite ;
            if ( curWriteCount - lastWriteCount > 0 )
            {
               hasUserWrite = TRUE ;
               rc = SDB_OK ;
               goto done ;
            }
            ossSleep( 1 ) ;
            lastCheckTick = pmdGetDBTick() ;
         }
      }

   done:
      if ( deletedCount > 0 )
      {
         PD_LOG( PDEVENT, "Recycled %d deleting records in collection[ %s ], "
                 "total cost: %d ms", deletedCount, clInfo._name,
                 pmdGetTickSpanTime( startTick ) ) ;
      }
      if ( pContext )
      {
         pSu->data()->releaseMBContext( pContext ) ;
      }
      if ( pSu )
      {
         pDmsCB->suUnlock( pSu->CSID(), SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSRECYCLERECJOB__DORECYCLERECJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsRecycleRecordJob::_deleteFirstDeletingRecord( pmdEDUCB *pEduCB,
                                                           dpsTransCB *pTransCB,
                                                           dmsStorageUnit *pSu,
                                                           dmsMBContext *pContext,
                                                           const monCLSimple &clInfo )
   {
      /// Test Lock
      INT32 rc = SDB_OK ;
      dmsRecordRW recordRW ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordID rid ;

      rc = pContext->mbTryLock( EXCLUSIVE ) ;
      if ( SDB_TIMEOUT == rc )
      {
         /// lock failed, wait next time
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      else if ( rc )
      {
         /// cl not exist
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }

      rid = pContext->mb()->_firstDeletingRID ;
      if ( rid.isNull() )
      {
         rc = SDB_DMS_RECORD_NOTEXIST ;
         goto done ;
      }

      rc = pTransCB->transLockTestX( pEduCB, pSu->LogicalCSID(),
                                     clInfo._blockID, &rid ) ;
      if ( rc != SDB_OK )
      {
         /// lock failed, wait next time
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }

      recordRW = pSu->data()->record2RW( rid, clInfo._blockID ) ;
      recordRW.setNothrow( TRUE ) ;
      pRecord = recordRW.readPtr<dmsRecord>() ;
      if ( !pRecord || pRecord->getMyOffset() != rid._offset )
      {
         /// record not exist
         rc = SDB_DMS_RECORD_NOTEXIST ;
         goto done ;
      }

      if ( !pRecord->isDeleting() || !pRecord->isInDeletingList() )
      {
         /// record is not deleting, or not in deleting list.
         /// this branch should never be arrived
         pContext->mb()->_firstDeletingRID.reset() ;
         pContext->mb()->_lastDeletingRID.reset() ;
         pContext->mbStat()->_totalDeletingRecords = 0 ;
         PD_LOG( PDWARNING, "Non-deleting record [%d, %d] was found in "
                 "deleting list", rid._extent, rid._offset ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// delete record
      rc = pSu->data()->deleteRecord( pContext, rid, (ossValuePtr)pRecord,
                                      pEduCB, NULL, NULL, NULL ) ;
      if ( rc != SDB_OK )
      {
         goto error ;
      }

   done:
      pContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 startRecycleRecordJob ( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      clsRecycleRecordJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsRecycleRecordJob() ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
