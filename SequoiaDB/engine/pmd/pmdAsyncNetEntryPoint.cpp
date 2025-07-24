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

   Source File Name = pmdAsyncNetEntryPoint.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDUMgr.hpp"
#include "netRouteAgent.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDNETCALLBACKFUNC, "pmdNetCallbackFunc" )
   INT32 pmdNetCallbackFunc( _netEventSuit *pSuit )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDNETCALLBACKFUNC ) ;

      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;

      rc = pEDUMgr->startEDU( EDU_TYPE_SUB_NET_AGENT,
                              (void*)pSuit, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Start sub-net agent failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_PMDNETCALLBACKFUNC, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDASYNCNETEP, "pmdAsyncNetEntryPoint" )
   INT32 pmdAsyncNetEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_PMDASYNCNETEP ) ;
      pmdEDUMgr *pEDUMgr = cb->getEDUMgr () ;
      _netRouteAgent *pRouteAgent = (_netRouteAgent *)pData;
      BOOLEAN hasReg = FALSE ;

      rc = pEDUMgr->activateEDU( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU[type:%d,ID:%lld]",
                  cb->getType(), cb->getID() ) ;
         goto error ;
      }

      //start run
      PD_LOG ( PDEVENT, "Run %s[Type: %d] ...", getEDUName( cb->getType() ),
               cb->getType() ) ;

      pEDUMgr->addIOService( pRouteAgent->ioservice() ) ;
      hasReg = TRUE ;
      try
      {
         rc = pRouteAgent->run( pmdNetCallbackFunc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception during start %s[Type: %d]: %s",
                  getEDUName( cb->getType() ), cb->getType(),
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Stop %s[Type: %d]", getEDUName( cb->getType() ),
               cb->getType() ) ;

   done:
      if ( hasReg )
      {
         pEDUMgr->delIOSerivce( pRouteAgent->ioservice() ) ;
      }
      PD_TRACE_EXITRC ( SDB_PMDASYNCNETEP, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDASYNCNETSUBEP, "pmdAsyncNetSubEntryPoint" )
   INT32 pmdAsyncNetSubEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_PMDASYNCNETSUBEP ) ;
      pmdEDUMgr *pEDUMgr = cb->getEDUMgr () ;
      _netEventSuit *pSuit = (_netEventSuit*)pData;
      netEvSuitPtr suitPtr = netEvSuitPtr::makeRaw( pSuit ) ;
      SDB_UNUSED( suitPtr ) ;

      rc = pEDUMgr->activateEDU( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU[type:%d,ID:%lld]",
                  cb->getType(), cb->getID() ) ;
         goto error ;
      }

      //start run
      PD_LOG ( PDEVENT, "Run %s[Type: %d] ...", getEDUName( cb->getType() ),
               cb->getType() ) ;

      rc = pSuit->run() ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Run %s[Type: %d] failed, rc: %d",
                  getEDUName( cb->getType() ), cb->getType(), rc ) ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Stop %s[Type: %d]", getEDUName( cb->getType() ),
               cb->getType() ) ;

   done:
      PD_TRACE_EXITRC ( SDB_PMDASYNCNETSUBEP, rc );
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_REPR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "ReplReader" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SHARDR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "ShardReader" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CATNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "CatalogNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_COORDNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "CoordNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_COORD_DS_NETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "CoordDSNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_OMNET, TRUE,
                          pmdAsyncNetEntryPoint,
                          "OMNet" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_RTNNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "RtnNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_SERVICE, TRUE,
                          pmdAsyncNetEntryPoint,
                          "SeService" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_INDEXR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "SeIndexerReader" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SUB_NET_AGENT, FALSE,
                          pmdAsyncNetSubEntryPoint,
                          "SubNetAgent" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_FS_MCS_NET_SERVICE, TRUE,
                          pmdAsyncNetEntryPoint,
                          "FsMcsNetService" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_FS_MCS_NET_AGENT, TRUE,
                          pmdAsyncNetEntryPoint,
                          "FsMcsNetAgent" ) ;

}

