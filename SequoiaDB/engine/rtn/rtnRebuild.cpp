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

   Source File Name = rtnRebuild.cpp

   Descriptive Name = Runtime Database Rebuild

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for database
   rebuild.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsReorgUnit.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   // rebuild entire database, first we need to register rebuild, so that no
   // body are able to access the database
   // After that we have to iterate all storage units, and all collections, and
   // perform offline reorg
   // note the offline reorg request does NOT go through _rtnReorg::doit, so we
   // won't check dmsCB->writable(). Thus we still able to perform
   // rtnReorgOffline while db is in REBUILD state

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREBUILDDB, "rtnRebuildDB" )
   INT32 rtnRebuildDB ( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNREBUILDDB ) ;

      pmdKRCB *krcb = pmdGetKRCB () ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB () ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB () ;
      BSONObj dummyObj ;
      std::set<monCollectionSpace> csList ;
      std::set<monCollectionSpace>::iterator it ;
      BOOLEAN registeredRebuild = FALSE ;
      UINT64 beginTime = 0 ;
      UINT64 endTime = 0 ;

      PD_LOG ( PDEVENT, "Start rebuilding database" ) ;

      // 1) register rebuild, make sure no one else is able to change anything
      rc = dmsCB->registerRebuild ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to register rebuild" ) ;
         goto error ;
      }
      registeredRebuild = TRUE ;
      beginTime = (UINT64)time( NULL ) ;

      // 2) get all collectionspaces
      dmsCB->dumpInfo ( csList, TRUE ) ;

      for ( it = csList.begin(); it != csList.end(); ++it )
      {
         // 3) for each collection space, let's do reorg for all collections
         const CHAR *pCSName = (*it)._name ;
         std::set<monCollection> clList ;
         std::set<monCollection>::iterator itCollection ;
         dmsStorageUnitID suID ;
         // make sure collectio spacea name is valid
         if ( ossStrlen ( pCSName ) > DMS_COLLECTION_SPACE_NAME_SZ )
         {
            PD_LOG ( PDERROR, "collection space name is not valid: %s",
                     pCSName ) ;
            continue ;
         }
         // skip system collectionspace
         if ( ossStrncmp ( pCSName, SDB_DMSTEMP_NAME,
                           DMS_COLLECTION_SPACE_NAME_SZ ) == 0 )
         {
            // skip SYSTEMP collectionspace
            continue ;
         }
         PD_LOG ( PDEVENT, "Start rebuilding collection space %s", pCSName ) ;
         // lock collection space
         // Note there could be nested S lock in the following rtnReorgOffline,
         // thus we have to make sure no one should attempt to drop the
         // collection space at the moment. Otherwise deadlock may happen.
         // This is guaranteed by dmsCB->registerRebuild, so that no
         // dropCollectionSpace command able to come into engine
         dmsStorageUnit *su = NULL;
         rc = dmsCB->nameToSUAndLock ( pCSName, suID, &su ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to lock collection space %s", pCSName ) ;
            continue ;
         }

         do
         {
            UINT64 tCLBegin = 0 ;
            UINT64 tCLEnd = 0 ;
            // get collection list for the given collectionspace, include
            // catalog system collections
            su->dumpInfo ( clList, TRUE ) ;
            // loop for each collection
            for ( itCollection = clList.begin();
                  itCollection != clList.end();
                  ++itCollection )
            {
               dmsMBContext *mbContext = NULL ;
               UINT16 collectionFlag ;
               const CHAR *pCLNameTemp = NULL ;
               const CHAR *pCLName = (*itCollection)._name ;

               // make sure collection name is valid
               if ( ( ossStrlen ( pCLName ) > DMS_COLLECTION_FULL_NAME_SZ ) ||
                    ( NULL == ( pCLNameTemp = ossStrrchr ( pCLName, '.' ))) )
               {
                  PD_LOG ( PDERROR, "collection name is not valid: %s",
                           pCLName ) ;
                  continue ;
               }
               // lock collection
               // +1 to skip "."
               rc = su->data()->getMBContext( &mbContext, pCLNameTemp+1,
                                              EXCLUSIVE ) ;
               if ( rc )
               {
                  PD_LOG ( PDWARNING, "Failed to lock collection %s, rc = %d",
                           pCLName, rc ) ;
                  continue ;
               }
               // get collection status
               collectionFlag = mbContext->mb()->_flag ;
               // unlock collection
               su->data()->releaseMBContext( mbContext ) ;

               tCLBegin = (UINT64)time( NULL ) ;
               PD_LOG ( PDEVENT, "Start rebuilding collection %s", pCLName ) ;

               // test collection status
               if ( DMS_IS_MB_OFFLINE_REORG( collectionFlag ) ||
                    DMS_IS_MB_ONLINE_REORG ( collectionFlag ) )
               {
                  // if the collection is already in reorg status, that means
                  // either system crash during reorg, or last rebuild didn't
                  // success, so let's perform reorg recover
                  rc = rtnReorgRecover ( pCLName, cb, dmsCB, rtnCB ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to perform reorg recover: %s, "
                              "rc = %d", pCLName, rc ) ;
                  }
               }
               else
               {
                  // perform offline reorg for the collection
                  rc = rtnReorgOffline ( pCLName, dummyObj, cb,
                                         dmsCB, rtnCB, TRUE ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to perform offline reorg: %s, "
                              "rc = %d", pCLName, rc ) ;
                  }
               }

               tCLEnd = (UINT64)time(NULL) ;
               PD_LOG ( PDEVENT, "Complete rebuilding collection %s, "
                        "result: %d, cost %s", pCLName, rc,
                        utilTimeSpanStr( tCLEnd-tCLBegin ).c_str() ) ;

               if ( SDB_APP_INTERRUPT == rc )
               {
                  break ;
               }
            } // for
         } while ( 0 ) ;

         // unlock the collection space
         dmsCB->suUnlock ( suID ) ;
         PD_LOG ( PDEVENT, "Complete rebuilding collection space %s, "
                  "result: %d", pCSName, rc ) ;

         if ( SDB_APP_INTERRUPT == rc )
         {
            if ( PMD_IS_DB_UP() )
            {
               PMD_SHUTDOWN_DB( rc ) ;
            }
            break ;
         }
      } // end for

      endTime = (UINT64)time(NULL) ;
      PD_LOG ( PDEVENT, "Complete rebuild database, result: %d, cost %s",
               rc, utilTimeSpanStr( endTime-beginTime ).c_str() ) ;

   done :
      if ( registeredRebuild )
      {
         dmsCB->rebuildDown ( cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNREBUILDDB, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}

