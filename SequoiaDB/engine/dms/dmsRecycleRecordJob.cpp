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

   Source File Name = dmsRecycleRecordJob.cpp

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
#include "dmsTrace.hpp"
#include "dmsRecycleRecordJob.hpp"

namespace engine
{
   class _dmsRecycleCLFilter : public _dmsCLFilter
   {
   public:
      _dmsRecycleCLFilter( UINT64 delayTimeMs, UINT32 recordRecycleRatio )
      {
         _delayTimeMs = delayTimeMs ;
         _recordRecycleRatio = recordRecycleRatio ;
      }

      virtual ~_dmsRecycleCLFilter() {}

      virtual BOOLEAN filter( const dmsMBStatInfo &stat )
      {
         static const UINT64 MIN_REC_TO_BE_RECYCLED = 1000 ;
         if ( 0 == _delayTimeMs )
         {
            // feature is disabled
            return FALSE ;
         }
         if ( stat._totalDeletingRecords < MIN_REC_TO_BE_RECYCLED )
         {
            // too few records to be recycled
            return FALSE ;
         }
         UINT32 deletingPercent = (stat._totalDeletingRecords * 100) /
               ( stat._totalRecords + stat._totalDeletingRecords ) ;
         if ( deletingPercent < _recordRecycleRatio && // (1)
              pmdGetTickSpanTime( stat._lastWriteTick ) <= _delayTimeMs ) // (2)
         {
            // (1) no need to do force recycle
            // (2) cl has write operations recently
            return FALSE ;
         }
         return TRUE ;
      }

   private:
      UINT64 _delayTimeMs ;
      UINT32 _recordRecycleRatio ;
   } ;

   /*
    *  _dmsRecycleRecordJob implement
    */
   _dmsRecycleRecordJob::_dmsRecycleRecordJob ()
   {
   }

   _dmsRecycleRecordJob::~_dmsRecycleRecordJob ()
   {
   }

   INT32 _dmsRecycleRecordJob::doit ()
   {
      static const UINT64 DO_JOB_INTERVAL = 10 * OSS_ONE_SEC ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdEDUMgr *pEduMgr = krcb->getEDUMgr() ;
      SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
      dpsTransCB *pTransCB = krcb->getTransCB() ;
      pmdOptionsCB *pOptionCB = pmdGetOptionCB() ;
      UINT64 delayTimeMs = 0 ;
      pmdEDUCB *cb = eduCB() ;
      UINT64 lastDumpTick = pmdGetDBTick() ;
      UINT64 lastDoJobTick = pmdGetDBTick() ;
      UINT64 curTick = 0 ;
      pmdEDUEvent event ;
      CLS_JOB_SET jobSet ;

      while ( !PMD_IS_DB_DOWN() && !cb->isForced() )
      {
         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
         */
         pEduMgr->waitEDU( cb ) ;
         cb->resetDisconnect() ;

         if ( pOptionCB->getRecordRecycleDelay() > 0 )
         {
            delayTimeMs = (UINT64)(pOptionCB->getRecordRecycleDelay()) * 60 * OSS_ONE_SEC ;
         }
         else
         {
            delayTimeMs = 0 ;
         }
         cb->waitEvent( event, OSS_ONE_SEC ) ;

         // Check stop signal first
         if ( cb->isInterrupted() )
         {
            continue ;
         }

         if ( 0 == delayTimeMs )
         {
            // This feature was disabled, do nothing
            continue ;
         }

         /// set edu active
         pEduMgr->activateEDU( cb ) ;

         curTick = pmdGetDBTick() ;
         if ( pmdDBTickSpan2Time( curTick - lastDumpTick ) >= delayTimeMs )
         {
            MON_CL_SIM_LIST clList ;
            MON_CL_SIM_LIST::iterator it ;
            UINT32 ratio = pOptionCB->getRecordRecycleRatio() ;
            _dmsRecycleCLFilter filter( delayTimeMs, ratio ) ;

            pDmsCB->dumpInfo( clList, TRUE, FALSE, &filter ) ;

            for ( it = clList.begin() ; it != clList.end() ; ++it )
            {
               _addJob2Set( jobSet, _dmsRecycleJobInfo( *it ) ) ;
            }
            lastDumpTick = pmdGetDBTick() ;
         }

         if ( !jobSet.empty() &&
              pmdDBTickSpan2Time( curTick - lastDoJobTick ) >= DO_JOB_INTERVAL )
         {
            _doRecycleRecordJobs( cb, pDmsCB, pTransCB, jobSet ) ;
            cb->incEventCount( 1 ) ;
            lastDoJobTick = pmdGetDBTick() ;
            /// release mem
            cb->shrink() ;
         }
      }
      return SDB_OK ;
   }

   void _dmsRecycleRecordJob::_addJob2Set( CLS_JOB_SET &set,
                                           const _dmsRecycleJobInfo &jobInfo )
   {
      try
      {
         // If the job exists, keep the old.
         set.insert( jobInfo ) ;
      }
      catch ( const std::exception &e )
      {
         PD_LOG( PDWARNING, "Failed to add collection info into list, "
                 "exception: %s", e.what() ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRECYCLERECJOB__DORECYCLERECJOBS, "_dmsRecycleRecordJob::_doRecycleRecordJobs" )
   void _dmsRecycleRecordJob::_doRecycleRecordJobs( pmdEDUCB *pEduCB,
                                                    SDB_DMSCB *pDmsCB,
                                                    dpsTransCB *pTransCB,
                                                    CLS_JOB_SET &jobSet )
   {
      PD_TRACE_ENTRY( SDB__DMSRECYCLERECJOB__DORECYCLERECJOBS ) ;

      INT32 rc = SDB_OK ;
      CLS_JOB_SET retrySet ;
      CLS_JOB_SET::iterator it ;
      BOOLEAN hasUserWrite = FALSE ;
      UINT64 lastWriteCount = 0 ;

      for ( it = jobSet.begin() ; it != jobSet.end() ; ++it )
      {
         rc = _doRecycleRecordJob( pEduCB, pDmsCB, pTransCB, *it,
                                   hasUserWrite, lastWriteCount ) ;
         if ( SDB_LOCK_FAILED == rc || hasUserWrite )
         {
            _dmsRecycleJobInfo job( it->_cl ) ;
            if ( hasUserWrite )
            {
               job._lastWriteCount = lastWriteCount ;
            }
            _addJob2Set( retrySet, job ) ;
         }
      }
      jobSet.clear() ;
      for ( it = retrySet.begin() ; it != retrySet.end() ; ++it )
      {
         _addJob2Set( jobSet, *it ) ;
      }

      PD_TRACE_EXIT( SDB__DMSRECYCLERECJOB__DORECYCLERECJOBS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRECYCLERECJOB__DORECYCLERECJOB, "_dmsRecycleRecordJob::_doRecycleRecordJob" )
   INT32 _dmsRecycleRecordJob::_doRecycleRecordJob( pmdEDUCB *pEduCB,
                                                    SDB_DMSCB *pDmsCB,
                                                    dpsTransCB *pTransCB,
                                                    const _dmsRecycleJobInfo &jobInfo,
                                                    BOOLEAN &hasUserWrite,
                                                    UINT64 &lastWriteCount )
   {
      static const UINT64 CHECK_WRITE_INTERVAL = 1000 ; // ms

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSRECYCLERECJOB__DORECYCLERECJOB ) ;

      const monCLSimple &clInfo = jobInfo._cl ;
      dmsStorageUnit *pSu = NULL ;
      dmsMBContext *pContext = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      IDmsScannerChecker *checker = NULL ;
      INT32 deletedCount = 0 ;
      UINT64 startTick = pmdGetDBTick() ;
      UINT64 lastCheckTick = 0 ;
      UINT64 curWriteCount = 0 ;

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

      rc = pDmsCB->createScannerChecker( pSu->data()->logicalID(),
                                         pContext->clLID(),
                                         clInfo._csname,
                                         clInfo._clname,
                                         "recycle record",
                                         pEduCB,
                                         &checker ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to open storage unit checker for "
                   "collection [%s], rc: %d", clInfo._name, rc ) ;

      lastCheckTick = startTick ;
      curWriteCount = pContext->mbStat()->_crudCB._totalDataWrite ;

      if ( jobInfo._lastWriteCount != OSS_UINT64_MAX )
      {
         // Don't recycle, until no user write operations
         lastWriteCount = jobInfo._lastWriteCount ;
         if ( curWriteCount != lastWriteCount )
         {
            hasUserWrite = TRUE ;
            lastWriteCount = curWriteCount ;
            rc = SDB_OK ;
            goto done ;
         }
      }
      else
      {
         lastWriteCount = curWriteCount ;
      }

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

         // check if interrupt
         if ( NULL != checker && checker->needInterrupt() )
         {
            PD_LOG( PDWARNING, "Scanner for collection [%s] need "
                    "interrupt", clInfo._name ) ;
            rc = SDB_DMS_SCANNER_INTERRUPT ;
            goto error ;
         }

         // In each Interval, sleep to release the lock and pause this job
         // if user has write operations to yield IO resource.
         if ( pmdGetTickSpanTime( lastCheckTick ) > CHECK_WRITE_INTERVAL )
         {
            curWriteCount = pContext->mbStat()->_crudCB._totalDataWrite ;
            if ( curWriteCount != lastWriteCount )
            {
               hasUserWrite = TRUE ;
               lastWriteCount = curWriteCount ;
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
      if ( NULL != checker )
      {
         pDmsCB->releaseScannerChecker( checker ) ;
      }
      if ( pContext )
      {
         pSu->data()->releaseMBContext( pContext ) ;
      }
      if ( pSu )
      {
         pDmsCB->suUnlock( pSu->CSID(), SHARED ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSRECYCLERECJOB__DORECYCLERECJOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsRecycleRecordJob::_deleteFirstDeletingRecord( pmdEDUCB *pEduCB,
                                                           dpsTransCB *pTransCB,
                                                           dmsStorageUnit *pSu,
                                                           dmsMBContext *pContext,
                                                           const _dmsRecycleJobInfo &jobInfo )
   {
      /// Test Lock
      INT32 rc = SDB_OK ;
      const monCLSimple &clInfo = jobInfo._cl ;
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

   INT32 dmsStartRecycleRecordJob ( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      dmsRecycleRecordJob *pJob = NULL ;

      pJob = SDB_OSS_NEW dmsRecycleRecordJob() ;
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
