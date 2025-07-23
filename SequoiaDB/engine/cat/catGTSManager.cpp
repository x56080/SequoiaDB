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

   Source File Name = catGTSManager.cpp

   Descriptive Name = GTS(Global Transaction Service) manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/13/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catGTSManager.hpp"
#include "catGTSDef.hpp"
#include "catalogueCB.hpp"
#include "pmdEDU.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

namespace engine
{
   _catGTSManager::_catGTSManager()
   {
      _dmsCB = NULL ;
      _eduCB = NULL ;
      _catCB = NULL ;
   }

   _catGTSManager::~_catGTSManager()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR_INIT, "_catGTSManager::init" )
   INT32 _catGTSManager::init()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;
      _dmsCB = pKrcb->getDMSCB() ;
      _catCB = pKrcb->getCATLOGUECB() ;

      PD_TRACE_ENTRY ( SDB_GTS_MGR_INIT ) ;

      rc = _ensureMetadata () ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to create GTS metadata, rc=%d", rc ) ;
         goto error ;
      }

      rc = _msgHandler.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _catGTSManager::fini()
   {
      INT32 rc = SDB_OK ;

      rc = _msgHandler.fini();
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to finalize GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _catGTSManager::attachCB( _pmdEDUCB* cb )
   {
      _eduCB = cb ;
   }

   void _catGTSManager::detachCB( _pmdEDUCB* cb )
   {
      _eduCB = NULL ;
   }

   INT32 _catGTSManager::active()
   {
      INT32 rc = SDB_OK ;

      rc = _seqMgr.active() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to active GTS sequence manager, rc=%d", rc ) ;
         goto error ;
      }

      rc = _msgHandler.active() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to active GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catGTSManager::deactive()
   {
      INT32 rc = SDB_OK ;

      rc = _msgHandler.deactive() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to deactive GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

      rc = _seqMgr.deactive() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to deactive GTS sequence manager, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catGTSManager::handleMsg( const NET_HANDLE& handle, const MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;

      PD_LOG( PDINFO, "Receive GTS msg: %d, length:%d",
              msg->opCode, msg->messageLength ) ;

      rc = _msgHandler.postMsg( handle, msg ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__ENSURE_METADATA, "_catGTSManager::_ensureMetadata" )
   INT32 _catGTSManager::_ensureMetadata()
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      PD_TRACE_ENTRY( SDB_GTS_MGR__ENSURE_METADATA ) ;

      // create SYSGTS.SEQUENCES
      rc = _createSysCollection( GTS_SEQUENCE_COLLECTION_NAME, cb ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _createSysIndex( GTS_SEQUENCE_COLLECTION_NAME,
                            GTS_SEQUENCE_NAME_INDEX, cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_GTS_MGR__ENSURE_METADATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__CREATE_SYS_INDEX, "_catGTSManager::_createSysIndex" )
   INT32 _catGTSManager::_createSysIndex( const CHAR* clFullName,
                                          const CHAR* indexJson,
                                          _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      PD_TRACE_ENTRY ( SDB_GTS_MGR__CREATE_SYS_INDEX ) ;
      PD_TRACE2 ( SDB_GTS_MGR__CREATE_SYS_INDEX,
                  PD_PACK_STRING ( clFullName ),
                  PD_PACK_STRING ( indexJson ) ) ;

      rc = fromjson ( indexJson, indexDef ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to build index object, rc = %d",
                    rc ) ;

      rc = rtnTestAndCreateIndex( clFullName, indexDef, cb, _dmsCB,
                                  NULL, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR__CREATE_SYS_INDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__CREATE_SYS_CL, "_catGTSManager::_createSysCollection" )
   INT32 _catGTSManager::_createSysCollection( const CHAR* clFullName,
                                               _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_MGR__CREATE_SYS_CL ) ;
      PD_TRACE1 ( SDB_GTS_MGR__CREATE_SYS_CL,
                  PD_PACK_STRING ( clFullName ) ) ;

      rc = rtnTestAndCreateCL( clFullName, cb, _dmsCB, NULL, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR__CREATE_SYS_CL, rc ) ;
      return rc ;
   error :
      goto done ;
   }
}

