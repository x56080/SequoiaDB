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

   Source File Name = dmsIndexOnlineBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexOnlineBuilder.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsScanner.hpp"
#include "ixmKey.hpp"
#include "ixm.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "dmsTrace.hpp"
#include "rtnDiskTBScanner.hpp"

namespace engine
{

   _dmsIndexOnlineBuilder::_dmsIndexOnlineBuilder( _dmsStorageUnit* su,
                                                   _dmsMBContext* mbContext,
                                                   _pmdEDUCB* eduCB,
                                                   dmsExtentID indexExtentID,
                                                   dmsExtentID indexLogicID,
                                                   dmsIndexBuildGuardPtr &guardPtr,
                                                   dmsDupKeyProcessor *dkProcessor,
                                                   dmsIdxTaskStatus* pIdxStatus )
   : _dmsIndexBuilder( su, mbContext, eduCB, indexExtentID, indexLogicID,
                       guardPtr, dkProcessor, pIdxStatus )
   {
   }

   _dmsIndexOnlineBuilder::~_dmsIndexOnlineBuilder()
   {
   }

   INT32 _dmsIndexOnlineBuilder::_build()
   {
      INT32 rc = SDB_OK ;

      dmsRecordID moveRID ;
      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;
      rtnDiskTBScanner scanner( _su, _mbContext, _scanRID, TRUE, 1, FALSE, _eduCB ) ;

      if ( _pIdxStatus )
      {
         _pIdxStatus->setOpInfo( OPINFO_SCAN_THEN_INSERT ) ;
      }

      // loop through each extent
      while ( !scanner.isEOF() )
      {
         rc = _mbLockAndCheck( SHARED ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
            goto error ;
         }

         if ( moveRID.isValid() )
         {
            {
               ossScopedRWLock lock( &( _buildGuardPtr->getProcessMutex() ), EXCLUSIVE ) ;
               rc = _buildGuardPtr->buildMove( moveRID ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to move index build record ID, rc: %d", rc ) ;
            }
            rc = scanner.relocateRID( moveRID ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG ( PDERROR, "Failed to get record: %d", rc ) ;
                  goto error ;
               }
               rc = SDB_OK ;
               goto done ;
            }
            moveRID.reset() ;
         }

         {
            dmsDataScanner extScanner( _suData, _mbContext, &scanner, NULL ) ;
            _mthRecordGenerator generator ;
            UINT64 scannedNum = 0 ;
            dmsRecordID processedRID ;
            dmsRecordID recordID ;
            ossValuePtr recordDataPtr ;
            OSS_LATCH_MODE mode = scanner.isInit() ? SHARED : EXCLUSIVE ;
            ossScopedRWLock lock( &( _buildGuardPtr->getProcessMutex() ), mode ) ;

            rc = _beforeExtent() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check before scanner, rc: %d", rc ) ;

            while ( SDB_OK == ( rc = extScanner.advance( recordID, generator, _eduCB ) ) )
            {
               BOOLEAN needProcess = FALSE, needWait = FALSE, needMove = FALSE ;
               rc = _buildGuardPtr->buildCheck( recordID, FALSE, needProcess, needWait, needMove) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check index build, rc: %d", rc ) ;

               if ( needMove )
               {
                  moveRID = recordID ;
                  break ;
               }
               else if ( needWait )
               {
                  lock.lock( EXCLUSIVE ) ;
                  rc = _buildGuardPtr->buildCheckAfterWait( recordID, needProcess ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to check index build, rc: %d", rc ) ;
               }

               if ( needProcess )
               {
                  generator.getDataPtr( recordDataPtr ) ;
                  rc = _insertKey( recordDataPtr, recordID, ordering ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
               }

               processedRID = recordID ;
               _buildGuardPtr->buildDone( FALSE, recordID ) ;
               ++ scannedNum ;
            }

            if ( SDB_DMS_EOC != rc && SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "Failed to get record: %d", rc ) ;
               goto error ;
            }
            if ( ( !moveRID.isValid() ) &&
                 ( scanner.isEOF() ) )
            {
               lock.lock( EXCLUSIVE ) ;
               dmsRecordID nextRID ;
               if ( processedRID.isValid() )
               {
                  nextRID.fromUINT64( processedRID.toUINT64() + 1 ) ;
               }
               else
               {
                  dmsRecordID lastScanRID = _indexCB->getScanRID() ;
                  if ( lastScanRID.isValid() )
                  {
                     nextRID.fromUINT64( lastScanRID.toUINT64() + 1 ) ;
                  }
                  else
                  {
                     nextRID.resetMin() ;
                  }
               }

               rc = scanner.relocateRID( nextRID ) ;
               if ( SDB_OK != rc )
               {
                  if ( SDB_DMS_EOC != rc )
                  {
                     PD_LOG ( PDERROR, "Failed to relocate record: %d", rc ) ;
                     goto error ;
                  }
                  rc = SDB_OK ;
               }
            }

            rc = _afterExtent( processedRID, scannedNum, scanner.isEOF() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = scanner.pauseScan() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pause scanner, rc: %d", rc ) ;
         }

         _mbContext->mbUnlock() ;
      }

   done:
      _buildGuardPtr->buildExit() ;
      if ( _pIdxStatus )
      {
         _pIdxStatus->resetOpInfo() ;
      }
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

}

