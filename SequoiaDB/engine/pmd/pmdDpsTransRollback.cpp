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

   Source File Name = pmdDpsTransRollback.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"
#include "dmsRBSMgr.hpp"
#include "ossUtil.h"
#include "dpsTransCB.hpp"

namespace engine
{
   #define PMD_CLEAR_HISTRANS_INTERVAL          ( 60 )

   INT32 _pmdPrepareMVCCRBS( pmdEDUCB *cb,
                             dpsTransCB *transCB )
   {
      INT32 rc = SDB_OK ;

      UINT32 rbsNum = pmdGetOptionCB()->mvccRBSNum() ;
      dmsRBSMgr *rbsMgr = pmdGetKRCB()->getDMSCB()->getRBSSUMgr() ;

      if ( !pmdGetOptionCB()->mvccOn() )
      {
         goto done ;
      }

      SDB_ASSERT( NULL != rbsMgr, "RBS manager is invalid" ) ;
      SDB_ASSERT( 0 != rbsNum, "Number of RBS storage units is invalid" ) ;

      // initialize must be succeed for a primary node
      // so wait until it is done
      while ( !cb->isDisconnected() )
      {
         // re-initialize RBS before node start service
         rc = rbsMgr->init( rbsNum ) ;
         if ( SDB_OK == rc )
         {
            // initialization is finished
            PD_LOG( PDEVENT, "Finished initialize RBS manager with [%u] "
                    "storage units", rbsNum ) ;
            break ;
         }
         // failed to initialize
         PD_LOG( PDERROR, "Failed to initialize RBS manager, rc: %d", rc ) ;
         if ( !transCB->isDoRollback() )
         {
            // transCB stopped rollback processing means the node switch
            // back to secondary
            PD_LOG( PDWARNING, "Node switched back to secondary, skip "
                    "initialize RBS manager" ) ;
            break ;
         }
         // sleep for 1 second
         ossSleep( OSS_ONE_SEC ) ;
      }

   done:
      return rc ;
   }

   INT32 pmdDpsTransRollbackEntryPoint( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      UINT64 timeCount = 0 ;
      dpsTransCB * pTransCB = pmdGetKRCB()->getTransCB() ;
      SDB_DPSCB *pDpsCB = pmdGetKRCB()->getDPSCB() ;

      while( !cb->isDisconnected() )
      {
         pmdEDUEvent event;
         if ( cb->waitEvent( event, OSS_ONE_SEC, TRUE ) )
         {
            if ( PMD_EDU_EVENT_TERM == event._eventType )
            {
               PD_LOG ( PDEVENT, "EDU[%lld] is terminated",
                        cb->getID() );
               rc = SDB_APP_DISCONNECT;
               break;
            }
            else if ( PMD_EDU_EVENT_ACTIVE == event._eventType )
            {
               UINT64 doRollbackID = event._userData ;
               rc = SDB_OK ;

               while ( TRUE )
               {
<<<<<<< HEAD
                  rc = rtnTransRollbackAll( cb, doRollbackID ) ;
=======
                  // re-initialize RBS before node start service
                  rc = _pmdPrepareMVCCRBS( cb, pTransCB ) ;
                  if ( SDB_OK != rc )
                  {
                     break ;
                  }

                  // check in-doubt transactions first
                  if ( pTransCB->getEventHandler() )
                  {
                     rc = pTransCB->getEventHandler()->onRollbackAll() ;
                     if ( SDB_OK != rc )
                     {
                        break ;
                     }
                  }

                  // rollback remaining transactions
                  rc = rtnTransRollbackAll( cb, doRollbackID ) ;
                  break ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
               }
            }
            pmdEduEventRelease( event, cb ) ;
         }
         else
         {
            cb->shrink() ;
         }

         timeCount++ ;

         if ( 0 == timeCount % PMD_CLEAR_HISTRANS_INTERVAL )
         {
            if ( pTransCB && pDpsCB )
            {
               pTransCB->clearOutDateHisTrans( pDpsCB->getStartLsn().offset ) ;
            }
         }

         // check global transaction active time
         pTransCB->checkPrimaryActiveTime() ;
      }
      rc = SDB_OK ;
      return rc ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_DPSROLLBACK, TRUE,
                          pmdDpsTransRollbackEntryPoint,
                          "DpsRollback" ) ;

}

