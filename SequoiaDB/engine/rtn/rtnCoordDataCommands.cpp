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

   Source File Name = rtnCoordDataCommands.cpp

   Descriptive Name = Runtime Coord Commands for Data Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "ossTypes.h"
#include "ossErr.h"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtnContext.hpp"
#include "netMultiRouteAgent.hpp"
#include "msgCatalog.hpp"
#include "rtnCoordCommands.hpp"
#include "rtnCoordCommon.hpp"
#include "rtnCoordDataCommands.hpp"
#include "msgCatalog.hpp"
#include "catCommon.hpp"
#include "../bson/bson.h"
#include "pmdOptions.hpp"
#include "dms.hpp"
#include "omagentDef.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnCommand.hpp"
#include "coordSession.hpp"
#include "mthModifier.hpp"
#include "rtnCoordDef.hpp"
#include "rtnAlterRunner.hpp"

using namespace bson;

namespace engine
{
   /*
    * rtnCoordDataCMD2Phase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD2PH_GENDATAMSG, "rtnCoordDataCMD2Phase::_generateDataMsg" )
   INT32 rtnCoordDataCMD2Phase::_generateDataMsg ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   const vector<BSONObj> &cataObjs,
                                                   CHAR **ppMsgBuf,
                                                   MsgHeader **ppDataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCODATACMD2PH_GENDATAMSG ) ;

      // Generally, the operator code is MSG_BS_QUERY_REQ
      pMsg->opCode = MSG_BS_QUERY_REQ ;
      (*ppDataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCODATACMD2PH_GENDATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD2PH_GENROLLBACKMSG, "rtnCoordDataCMD2Phase::_generateRollbackDataMsg" )
   INT32 rtnCoordDataCMD2Phase::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                           _rtnCMDArguments *pArgs,
                                                           CHAR **ppMsgBuf,
                                                           MsgHeader **ppRollbackMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCODATACMD2PH_GENROLLBACKMSG ) ;

      (*ppRollbackMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCODATACMD2PH_GENROLLBACKMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD2PH_DOONCATA, "rtnCoordDataCMD2Phase::_doOnCataGroup" )
   INT32 rtnCoordDataCMD2Phase::_doOnCataGroup( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                rtnContextCoord **ppContext,
                                                _rtnCMDArguments *pArgs,
                                                CoordGroupList *pGroupLst,
                                                vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCODATACMD2PH_DOONCATA ) ;

      rtnContextCoord *pContext = NULL ;
      CoordCataInfoPtr cataInfo ;
      UINT32 retryTimes = 0 ;

   retry_before_cata :
      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         rc = rtnCoordGetRemoteCata( cb, pArgs->_targetName.c_str(), cataInfo ) ;
         if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
         {
            // If the rc is SDB_CLS_COORD_NODE_CAT_VER_OLD, it might be caused
            // by the out-of-date mainCL cache, removed the mainCL cache and
            // try again.
            CoordCB *pCoordcb = pmdGetKRCB()->getCoordCB() ;
            pCoordcb->delMainCLCataInfo( pArgs->_targetName.c_str() ) ;
            goto retry_before_cata ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: "
                      "Update collection [%s] catalog info failed, rc: %d",
                      _getCommandName(), pArgs->_targetName.c_str(),
                      pArgs->_targetName.c_str(), rc ) ;
      }

   retry :
      if ( _flagUpdateBeforeCata() && _flagDoOnCollection() )
      {
         MsgOpQuery *pOpMsg = (MsgOpQuery *)pMsg ;
         pOpMsg->version = cataInfo->getVersion() ;
      }

      // Send request to catalog, and get the control of CoordContext
      rc = executeOnCataGroup( pMsg, cb, pGroupLst, pReplyObjs,
                               TRUE, NULL, &pContext, pArgs->_pBuf ) ;
      if ( SDB_OK != rc &&
           _flagUpdateBeforeCata() &&
           _flagDoOnCollection() )
      {
         if ( rtnCoordCataReplyCheck( cb, rc, _canRetry( retryTimes ),
                                      cataInfo, NULL, TRUE ) )
         {
            ++retryTimes ;
            goto retry ;
         }
         goto error ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: "
                      "failed to execute on catalog, rc: %d",
                      _getCommandName(), pArgs->_targetName.c_str(), rc ) ;
      }

   done :
      if ( pContext )
      {
         (*ppContext) = pContext ;
      }

      PD_TRACE_EXITRC ( CMD_RTNCODATACMD2PH_DOONCATA, rc ) ;

      return rc ;

   error :
      if ( pContext )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( pContext->contextID(), cb ) ;
         pContext = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD2PH_DOONDATA, "rtnCoordDataCMD2Phase::_doOnDataGroup" )
   INT32 rtnCoordDataCMD2Phase::_doOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 _rtnCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs,
                                                 CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCODATACMD2PH_DOONDATA ) ;

      // Pre-read is opened for execute on data group
      // For DropCL/DropCS, this will guarantee data updates are
      // started before catalog updates
      if ( _flagDoOnCollection() )
      {
         rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                           _flagUpdateBeforeData(),
                           _flagUseGrpLstInCoord() ? NULL : &groupLst,
                           &(pArgs->_ignoreRCList), &sucGroupLst,
                           ppContext, pArgs->_pBuf ) ;
      }
      else
      {
         rc = executeOnDataGroup( pMsg, cb, groupLst, TRUE,
                                  &(pArgs->_ignoreRCList), NULL,
                                  ppContext, pArgs->_pBuf ) ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "failed to execute on data, rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCODATACMD2PH_DOONDATA, rc ) ;
      return rc ;
   error :
      if ( ppContext && (*ppContext) )
      {
         SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
         pRtnCB->contextDelete( (*ppContext)->contextID(), cb ) ;
         (*ppContext) = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD2PH_DOAUDIT "rtnCoordDataCMD2Phase::_doAudit" )
   INT32 rtnCoordDataCMD2Phase::_doAudit ( _rtnCMDArguments *pArgs,
                                           INT32 rc )
   {
      PD_TRACE_ENTRY ( CMD_RTNCODATACMD2PH_DOAUDIT ) ;

      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, _getCommandName(),
                           _flagDoOnCollection() ? AUDIT_OBJ_CL : AUDIT_OBJ_CS,
                           pArgs->_targetName.c_str(), rc, "Option: %s",
                           pArgs->_boQuery.toString().c_str() ) ;
      }

      PD_TRACE_EXIT ( CMD_RTNCODATACMD2PH_DOAUDIT ) ;

      return SDB_OK ;
   }

   /*
    * rtnCoordDataCMD3Phase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD3PH_DOONCATAP2, "rtnCoordDataCMD3Phase::_doOnCataGroupP2" )
   INT32 rtnCoordDataCMD3Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   rtnContextCoord **ppContext,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &pGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCODATACMD3PH_DOONCATAP2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( CMD_RTNCODATACMD3PH_DOONCATAP2, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCODATACMD3PH_DOONDATAP2, "rtnCoordDataCMD3Phase::_doOnDataGroupP2" )
   INT32 rtnCoordDataCMD3Phase::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   rtnContextCoord **ppContext,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &groupLst,
                                                   const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCODATACMD3PH_DOONDATAP2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( CMD_RTNCODATACMD3PH_DOONDATAP2, rc ) ;

      return rc ;
   }

   /*
    * rtnCoordCMDCreateDomain implement
    */
   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDCREATEDOMAIN_EXE, "rtnCoordCMDCreateDomain::execute" )
   INT32 rtnCoordCMDCreateDomain::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNCOCMDCREATEDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_CREATE_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDCreateDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create domain, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCREATEDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDDropDomain implement
    */
   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDDROPDOMAIN_EXE, "rtnCoordCMDDropDomain::execute" )
   INT32 rtnCoordCMDDropDomain::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNCOCMDDROPDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_DROP_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDDropDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to drop domain, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDDROPDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDAlterDomain implement
    */
   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOCMDALTERDOMAIN_EXE, "rtnCoordCMDAlterDomain::execute" )
   INT32 rtnCoordCMDAlterDomain::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDALTERDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_ALTER_DOMAIN_REQ;

      _printDebug ( (CHAR*)pMsg, "rtnCoordCMDAlterDomain" ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to alter domain, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDALTERDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDCreateCollectionSpace implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCRCS_EXE, "rtnCoordCMDCreateCollectionSpace::execute" )
   INT32 rtnCoordCMDCreateCollectionSpace::execute( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    INT64 &contextID,
                                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNCOCMDCRCS_EXE ) ;

      CHAR *pCollectionName = NULL ;
      CHAR *pQuery = NULL ;
      BSONObj boQuery ;

      // fill default-reply
      contextID = -1 ;
      MsgOpQuery *pCreateReq = (MsgOpQuery *)pMsg;

      try
      {
         _printDebug ( (CHAR*)pMsg, "rtnCoordCMDCreateCollectionSpace" ) ;

         rc = msgExtractQuery( (CHAR*)pMsg, NULL, &pCollectionName,
                               NULL, NULL, &pQuery, NULL, NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to extract message, rc: %d", rc ) ;

         boQuery = BSONObj( pQuery ) ;

         pCreateReq->header.opCode = MSG_CAT_CREATE_COLLECTION_SPACE_REQ ;
         // execute create collection on catalog
         rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, pCollectionName + 1, AUDIT_OBJ_CS,
                           boQuery.getField(FIELD_NAME_NAME).valuestrsafe(),
                           rc, "Option:%s", boQuery.toString().c_str() ) ;
         /// CHECK ERRORS
         if ( rc )
         {
            PD_LOG ( PDERROR,
                     "Failed to create collection space, rc: %d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCRCS_EXE, rc ) ;
      return rc;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDDropCollectionSpace implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCS_PARSEMSG, "rtnCoordCMDDropCollectionSpace::_parseMsg" )
   INT32 rtnCoordCMDDropCollectionSpace::_parseMsg ( MsgHeader *pMsg,
                                                     _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCS_PARSEMSG ) ;

      try
      {
         string csName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_SPACE_NAME,
                                      csName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_SPACE_NAME ) ;
         PD_CHECK( !csName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection space name can't be empty!",
                   _getCommandName() ) ;

         // Update argument
         pArgs->_targetName = csName ;

         // Add ignore return codes
         pArgs->_ignoreRCList.insert( SDB_DMS_CS_NOTEXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDDROPCS_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCS_GENCATAMSG, "rtnCoordCMDDropCollectionSpace::_generateCataMsg" )
   INT32 rtnCoordCMDDropCollectionSpace::_generateCataMsg ( MsgHeader *pMsg,
                                                            pmdEDUCB *cb,
                                                            _rtnCMDArguments *pArgs,
                                                            CHAR **ppMsgBuf,
                                                            MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCS_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_DROP_SPACE_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDDROPCS_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCS_DOCOMPLETE, "rtnCoordCMDDropCollectionSpace::_doComplete" )
   INT32 rtnCoordCMDDropCollectionSpace::_doComplete ( MsgHeader *pMsg,
                                                       pmdEDUCB * cb,
                                                       _rtnCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCS_DOCOMPLETE ) ;

      vector< string > subCLSet ;
      CoordCB *pCoordCB = pmdGetKRCB()->getCoordCB() ;
      pCoordCB->delCataInfoByCS( pArgs->_targetName.c_str(), &subCLSet ) ;

      /// clear relate sub collection's catalog info
      vector< string >::iterator it = subCLSet.begin() ;
      while( it != subCLSet.end() )
      {
         pCoordCB->delCataInfo( *it ) ;
         ++it ;
      }

      PD_TRACE_EXIT ( CMD_RTNCOCMDDROPCS_DOCOMPLETE ) ;

      return SDB_OK ;
   }

   /*
    * rtnCoordCMDCreateCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATECL_PARSEMSG, "rtnCoordCMDCreateCollection::_parseMsg" )
   INT32 rtnCoordCMDCreateCollection::_parseMsg ( MsgHeader *pMsg,
                                                  _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATECL_PARSEMSG ) ;

      try
      {
         string clName ;
         BOOLEAN isMainCL = FALSE ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_NAME ) ;
         PD_CHECK( !clName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection name can't be empty!",
                   _getCommandName() ) ;

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ISMAINCL,
                                    isMainCL ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            isMainCL = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_ISMAINCL ) ;

         if ( isMainCL )
         {
            // Check sharding keys
            BSONObj boShardingKey ;
            rc = rtnGetObjElement( pArgs->_boQuery, FIELD_NAME_SHARDINGKEY,
                                   boShardingKey ) ;
            PD_CHECK( SDB_OK == rc,
                      SDB_NO_SHARDINGKEY, error, PDERROR,
                      "Failed to %s: there is no valid sharding-key field",
                      _getCommandName() ) ;

            // Check sharding type
            string shardingType ;
            rc = rtnGetSTDStringElement( pArgs->_boQuery, FIELD_NAME_SHARDTYPE,
                                         shardingType ) ;
            if ( SDB_OK == rc )
            {
               PD_CHECK( 0 != shardingType.compare( FIELD_NAME_SHARDTYPE_HASH ),
                         SDB_INVALID_MAIN_CL_TYPE, error, PDERROR,
                         "Failed to %s: "
                         "sharding-type of main-collection must be range",
                         _getCommandName() ) ;
            }
            else
            {
               PD_LOG( PDWARNING,
                       "Failed to %s: failed to get the field [%s] from query",
                       _getCommandName(), FIELD_NAME_SHARDTYPE ) ;
               rc = SDB_OK ;
            }
         }

         // Update arguments
         pArgs->_targetName = clName ;

         // Add ignored error return code
         pArgs->_ignoreRCList.insert( SDB_DMS_EXIST ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATECL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATECL_GENCATAMSG, "rtnCoordCMDCreateCollection::_generateCataMsg" )
   INT32 rtnCoordCMDCreateCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         _rtnCMDArguments *pArgs,
                                                         CHAR **ppMsgBuf,
                                                         MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATECL_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_CREATE_COLLECTION_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDCREATECL_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATECL_GENROLLBACKMSG, "rtnCoordCMDCreateCollection::_generateRollbackDataMsg" )
   INT32 rtnCoordCMDCreateCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                                 _rtnCMDArguments *pArgs,
                                                                 CHAR **ppMsgBuf,
                                                                 MsgHeader **ppRollbackMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATECL_GENROLLBACKMSG ) ;

      INT32 bufSize = 0 ;

      rc = msgBuildDropCLMsg( ppMsgBuf, &bufSize, pArgs->_targetName.c_str(), 0 ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to rollback %s on [%s]: "
                    "failed to build dropCL message, rc: %d",
                    _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

      (*ppRollbackMsg) = (MsgHeader *)(*ppMsgBuf) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATECL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATECL_ROLLBACK, "rtnCoordCMDCreateCollection::_rollbackOnDataGroup" )
   INT32 rtnCoordCMDCreateCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                             pmdEDUCB *cb,
                                                             _rtnCMDArguments *pArgs,
                                                             const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATECL_ROLLBACK ) ;

      pmdKRCB *pKrcb = pmdGetKRCB();
      CoordCB *pCoordcb = pKrcb->getCoordCB();
      SET_RC ignoreRC ;
      rtnContextCoord *pCtxForData = NULL ;

      ignoreRC.insert( SDB_DMS_NOTEXIST ) ;
      ignoreRC.insert( SDB_DMS_CS_NOTEXIST ) ;
      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &ignoreRC, NULL,
                        &pCtxForData, pArgs->_pBuf ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback %s on [%s]: "
                   "drop collection phase 1 failed, rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

      if ( pCtxForData )
      {
         rc = _processContext( cb, &pCtxForData, -1 ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to rollback %s on [%s]: "
                      "drop collection phase 2, rc: %d",
                      _getCommandName(), pArgs->_targetName.c_str(), rc ) ;
      }

      // Clear Coord catalog info
      pCoordcb->delCataInfo( pArgs->_targetName ) ;

   done :
      if ( pCtxForData )
      {
         pmdKRCB *pKrcb = pmdGetKRCB();
         _SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
         pRtncb->contextDelete ( pCtxForData->contextID(), cb ) ;
      }

      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATECL_ROLLBACK, rc ) ;

      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDDropCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCL_PARSEMSG, "rtnCoordCMDDropCollection::_parseMsg" )
   INT32 rtnCoordCMDDropCollection::_parseMsg ( MsgHeader *pMsg,
                                                _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCL_PARSEMSG ) ;

      try
      {
         string clName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_NAME ) ;
         PD_CHECK( !clName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection name can't be empty!",
                   _getCommandName() ) ;

         pArgs->_targetName = clName ;

         pArgs->_ignoreRCList.insert( SDB_DMS_NOTEXIST );
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDDROPCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCL_GENCATAMSG, "rtnCoordCMDDropCollection::_generateCataMsg" )
   INT32 rtnCoordCMDDropCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       _rtnCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCL_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_DROP_COLLECTION_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDDROPCL_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCL_GENDATAMSG, "rtnCoordCMDDropCollection::_generateDataMsg" )
   INT32 rtnCoordCMDDropCollection::_generateDataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       _rtnCMDArguments *pArgs,
                                                       const vector<BSONObj> &cataObjs,
                                                       CHAR **ppMsgBuf,
                                                       MsgHeader **ppDataMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCL_GENDATAMSG ) ;

      rc = rtnCoordDataCMD3Phase::_generateDataMsg( pMsg, cb, pArgs,
                                                    cataObjs, ppMsgBuf,
                                                    ppDataMsg) ;

      if ( cataObjs.size() == 1 )
      {
         try
         {
            BSONElement beCollection = cataObjs[0].getField( CAT_COLLECTION ) ;
            if ( !beCollection.eoo() &&
                 beCollection.type() == Object )
            {
               CoordCataInfoPtr cataInfo ;
               BSONObj boCataInfo = beCollection.Obj() ;

               // The catalog info of collection maybe too old
               // The reply from Catalog implies that info need to be updated

               PD_LOG( PDDEBUG,
                       "Updating catalog info of collection [%s]",
                       pArgs->_targetName.c_str() ) ;

               rc = rtnCoordProcessCatInfoObj( boCataInfo, cataInfo ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to parse catalog info of collection [%s], "
                            "rc: %d",
                            pArgs->_targetName.c_str(), rc ) ;
               pmdKRCB *pKrcb = pmdGetKRCB() ;
               CoordCB *pCoordcb = pKrcb->getCoordCB() ;
               pCoordcb->updateCataInfo( pArgs->_targetName, cataInfo ) ;

               MsgOpQuery *pOpMsg = (MsgOpQuery *)pMsg ;
               pOpMsg->version = cataInfo->getVersion() ;
            }
         }
         catch ( std::exception &e )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Failed to parse catalog info of collection,"
                     "received unexpected error: %s", e.what() ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( CMD_RTNCOCMDDROPCL_GENDATAMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPCL_DOCOMPLETE, "rtnCoordCMDDropCollection::_doComplete" )
   INT32 rtnCoordCMDDropCollection::_doComplete ( MsgHeader *pMsg,
                                                  pmdEDUCB * cb,
                                                  _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPCL_DOCOMPLETE) ;

      pmdKRCB *pKrcb = pmdGetKRCB();
      CoordCB *pCoordcb = pKrcb->getCoordCB();

      string strMainCLName ;
      CoordCataInfoPtr cataInfo ;

      rc = rtnCoordGetCataInfo( cb, pArgs->_targetName.c_str(), FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to %s on [%s]: "
                   "failed to get catalog, complete drop-CL failed, rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

      strMainCLName = cataInfo->getCatalogSet()->getMainCLName() ;
      pCoordcb->delCataInfo( pArgs->_targetName ) ;
      if ( !strMainCLName.empty() )
      {
         pCoordcb->delCataInfo( strMainCLName ) ;
      }

   done :
      PD_TRACE_EXIT ( CMD_RTNCOCMDDROPCL_DOCOMPLETE ) ;
      return SDB_OK ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDAlterCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDALTERCL_PARSEMSG, "rtnCoordCMDAlterCollection::_parseMsg" )
   INT32 rtnCoordCMDAlterCollection::_parseMsg ( MsgHeader *pMsg,
                                                 _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDALTERCL_PARSEMSG ) ;

      try
      {
         string clName ;
         BOOLEAN isOldAlterCMD = FALSE ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION_NAME,
                                      clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_NAME ) ;
         PD_CHECK( !clName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection name can't be empty!",
                   _getCommandName() ) ;

         isOldAlterCMD = pArgs->_boQuery.getField( FIELD_NAME_VERSION ).eoo() ;

         PD_LOG ( PDDEBUG,
                  "Alter collection command %s",
                  isOldAlterCMD ? "" : "id index" ) ;

         pArgs->_targetName = clName ;

         if ( isOldAlterCMD )
         {
            /// we only want to update data's catalog version.
            pArgs->_ignoreRCList.insert( SDB_MAIN_CL_OP_ERR ) ;
            pArgs->_ignoreRCList.insert( SDB_CLS_COORD_NODE_CAT_VER_OLD ) ;
         }
         else
         {
            pArgs->_ignoreRCList.insert( SDB_IXM_REDEF ) ;
            pArgs->_ignoreRCList.insert( SDB_IXM_NOTEXIST ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDALTERCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDALTERCL_GENCATAMSG, "rtnCoordCMDAlterCollection::_generateCataMsg" )
   INT32 rtnCoordCMDAlterCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                        pmdEDUCB *cb,
                                                        _rtnCMDArguments *pArgs,
                                                        CHAR **ppMsgBuf,
                                                        MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDALTERCL_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_ALTER_COLLECTION_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDALTERCL_GENCATAMSG ) ;

      return SDB_OK ;
   }

   /*
    * rtnCoordCMDLinkCollection implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDLinkCollection::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDLinkCLArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLINKCL_PARSEMSG, "rtnCoordCMDLinkCollection::_parseMsg" )
   INT32 rtnCoordCMDLinkCollection::_parseMsg ( MsgHeader *pMsg,
                                                _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDLINKCL_PARSEMSG ) ;

      _rtnCMDLinkCLArgs *pSelfArgs = ( _rtnCMDLinkCLArgs * )pArgs ;

      try
      {
         string mainCLName, subCLName ;
         BSONObj lowBound, upBound ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery, CAT_SUBCL_NAME,
                                      subCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_SUBCL_NAME ) ;
         PD_CHECK( !subCLName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: sub-collection name can't be empty!",
                   _getCommandName() ) ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery, CAT_COLLECTION_NAME,
                                      mainCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_NAME ) ;
         PD_CHECK( !mainCLName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: main-collection name can't be empty!",
                   _getCommandName() ) ;

         rc = rtnGetObjElement( pSelfArgs->_boQuery, CAT_LOWBOUND_NAME, lowBound ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_LOWBOUND_NAME ) ;

         rc = rtnGetObjElement( pSelfArgs->_boQuery, CAT_UPBOUND_NAME, upBound ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_UPBOUND_NAME ) ;

         pSelfArgs->_targetName = mainCLName ;
         pSelfArgs->_subCLName = subCLName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDLINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLINKCL_GENCATAMSG, "rtnCoordCMDLinkCollection::_generateCataMsg" )
   INT32 rtnCoordCMDLinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                       pmdEDUCB *cb,
                                                       _rtnCMDArguments *pArgs,
                                                       CHAR **ppMsgBuf,
                                                       MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDLINKCL_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_LINK_CL_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDLINKCL_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLINKCL_GENROLLBACKMSG, "rtnCoordCMDLinkCollection::_generateRollbackDataMsg" )
   INT32 rtnCoordCMDLinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                               _rtnCMDArguments *pArgs,
                                                               CHAR **ppMsgBuf,
                                                               MsgHeader **ppRollbackMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDLINKCL_GENROLLBACKMSG ) ;

      INT32 bufSize = 0 ;

      _rtnCMDLinkCLArgs *pSelfArgs = ( _rtnCMDLinkCLArgs * )pArgs ;

      rc = msgBuildUnlinkCLMsg( ppMsgBuf, &bufSize,
                                pSelfArgs->_targetName.c_str(),
                                pSelfArgs->_subCLName.c_str(), 0 ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to rollback %s on [%s/%s]: "
                    "failed to build unlink message, rc: %d",
                    _getCommandName(),
                    pSelfArgs->_targetName.c_str(),
                    pSelfArgs->_subCLName.c_str(),
                    rc ) ;

      (*ppRollbackMsg) = (MsgHeader *)(*ppMsgBuf) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDLINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDLINKCL_ROLLBACK, "rtnCoordCMDLinkCollection::_rollbackOnDataGroup" )
   INT32 rtnCoordCMDLinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                           pmdEDUCB *cb,
                                                           _rtnCMDArguments *pArgs,
                                                           const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDLINKCL_ROLLBACK ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback %s on [%s], rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDLINKCL_ROLLBACK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDUnlinkCollection implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDUnlinkCollection::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDUnlinkCLArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDUNLINKCL_PARSEMSG, "rtnCoordCMDUnlinkCollection::_parseMsg" )
   INT32 rtnCoordCMDUnlinkCollection::_parseMsg ( MsgHeader *pMsg,
                                                  _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDUNLINKCL_PARSEMSG ) ;

      _rtnCMDUnlinkCLArgs *pSelfArgs = ( _rtnCMDUnlinkCLArgs * )pArgs ;

      try
      {
         string mainCLName, subCLName ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery, CAT_SUBCL_NAME,
                                      subCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_SUBCL_NAME ) ;
         PD_CHECK( !subCLName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: sub-collection name can't be empty!",
                   _getCommandName() ) ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery, CAT_COLLECTION_NAME,
                                      mainCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION_NAME ) ;
         PD_CHECK( !mainCLName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: main-collection name can't be empty!",
                   _getCommandName() ) ;

         pSelfArgs->_targetName = mainCLName ;
         pSelfArgs->_subCLName = subCLName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDUNLINKCL_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDUNLINKCL_GENCATAMSG, "rtnCoordCMDUnlinkCollection::_generateCataMsg" )
   INT32 rtnCoordCMDUnlinkCollection::_generateCataMsg ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         _rtnCMDArguments *pArgs,
                                                         CHAR **ppMsgBuf,
                                                         MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDUNLINKCL_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_UNLINK_CL_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDUNLINKCL_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDUNLINKCL_GENROLLBACKMSG, "rtnCoordCMDUnlinkCollection::_generateRollbackDataMsg" )
   INT32 rtnCoordCMDUnlinkCollection::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                                 _rtnCMDArguments *pArgs,
                                                                 CHAR **ppMsgBuf,
                                                                 MsgHeader **ppRollbackMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDUNLINKCL_GENROLLBACKMSG ) ;

      INT32 bufSize = 0 ;

      _rtnCMDUnlinkCLArgs *pSelfArgs = ( _rtnCMDUnlinkCLArgs * )pArgs ;

      // The Data Group doesn't care about lowBound and upBound
      rc = msgBuildLinkCLMsg( ppMsgBuf, &bufSize,
                              pSelfArgs->_targetName.c_str(),
                              pSelfArgs->_subCLName.c_str(),
                              NULL, NULL, 0 ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to rollback %s on [%s/%s]: "
                    "failed to build link message, rc: %d",
                    _getCommandName(),
                    pSelfArgs->_targetName.c_str(),
                    pSelfArgs->_subCLName.c_str(),
                    rc ) ;

      (*ppRollbackMsg) = (MsgHeader *)(*ppMsgBuf) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDUNLINKCL_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDUNLINKCL_ROLLBACK, "rtnCoordCMDUnlinkCollection::_rollbackOnDataGroup" )
   INT32 rtnCoordCMDUnlinkCollection::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                             pmdEDUCB *cb,
                                                             _rtnCMDArguments *pArgs,
                                                             const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDUNLINKCL_ROLLBACK ) ;

      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        TRUE, &groupLst, &(pArgs->_ignoreRCList), NULL,
                        NULL, pArgs->_pBuf ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback %s on [%s], rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDUNLINKCL_ROLLBACK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDSplit implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP_GETCLCOUNT, "rtnCoordCMDSplit::_getCLCount" )
   INT32 rtnCoordCMDSplit::_getCLCount( const CHAR * clFullName,
                                       CoordGroupList & groupList,
                                       pmdEDUCB *cb,
                                       UINT64 & count,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_RTNCOCMDSP_GETCLCOUNT ) ;

      pmdKRCB *pKRCB = pmdGetKRCB () ;
      SDB_RTNCB *pRtncb = pKRCB->getRTNCB() ;
      rtnContext *pContext = NULL ;
      count = 0 ;
      CoordGroupList tmpGroupList = groupList ;

      BSONObj collectionObj ;
      BSONObj dummy ;
      rtnContextBuf buffObj ;

      collectionObj = BSON( FIELD_NAME_COLLECTION << clFullName ) ;

      // send getcount to node
      rc = rtnCoordNodeQuery( CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                              dummy, dummy, dummy, collectionObj,
                              0, 1, tmpGroupList, cb, &pContext,
                              clFullName, 0, buf ) ;

      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get count from source node, rc: %d",
                    rc ) ;

      rc = pContext->getMore( -1, buffObj, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to get count from source node: get-more failed, rc: %d",
                 rc ) ;
         goto error ;
      }
      else
      {
         // get count data
         BSONObj countObj ( buffObj.data() ) ;
         BSONElement beTotal = countObj.getField( FIELD_NAME_TOTAL );
         PD_CHECK( beTotal.isNumber(), SDB_INVALIDARG, error,
                   PDERROR,
                   "Failed to get count from source node, "
                   "failed to get the field [%s]",
                   FIELD_NAME_TOTAL ) ;
         count = beTotal.numberLong() ;
      }

   done :
      if ( pContext )
      {
         SINT64 contextID = pContext->contextID() ;
         pRtncb->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSP_GETCLCOUNT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP_EXE, "rtnCoordCMDSplit::execute" )
   INT32 rtnCoordCMDSplit::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDSP_EXE ) ;
      pmdKRCB *pKRCB                   = pmdGetKRCB () ;
      SDB_RTNCB *pRtncb                = pKRCB->getRTNCB() ;
      CoordCB *pCoordcb                = pKRCB->getCoordCB () ;
      contextID                        = -1 ;

      CHAR *pCommandName               = NULL ;
      CHAR *pQuery                     = NULL ;

      CHAR szSource [ OSS_MAX_GROUPNAME_SIZE + 1 ] = {0} ;
      CHAR szTarget [ OSS_MAX_GROUPNAME_SIZE + 1 ] = {0} ;
      const CHAR *strName              = NULL ;
      CHAR *splitReadyBuffer           = NULL ;
      INT32 splitReadyBufferSz         = 0 ;
      CHAR *splitQueryBuffer           = NULL ;
      INT32 splitQueryBufferSz         = 0 ;
      MsgOpQuery *pSplitQuery          = NULL ;
      UINT64 taskID                    = CLS_INVALID_TASKID ;
      BOOLEAN async                    = FALSE ;
      BSONObj taskInfoObj ;

      BSONObj boShardingKey ;
      CoordCataInfoPtr cataInfo ;
      BSONObj boKeyStart ;
      BSONObj boKeyEnd ;
      FLOAT64 percent = 0.0 ;

      // first round we perform prepare, so catalog node is able to do sanity
      // check for collection name and nodes
      MsgOpQuery *pSplitReq            = (MsgOpQuery *)pMsg ;
      pSplitReq->header.opCode         = MSG_CAT_SPLIT_PREPARE_REQ ;

      CoordGroupList groupLst ;
      vector<BSONObj> boRecv ;
      CoordGroupList groupDstLst ;

      CoordSession * coordSession = cb->getCoordSession() ;
      rtnInstanceOption instanceOption ;
      BOOLEAN replacedInstanceOption = FALSE ;

      if ( NULL != coordSession && !coordSession->isMasterRequired() )
      {
         instanceOption = coordSession->getInstanceOption() ;
         coordSession->setMasterRequired() ;
         replacedInstanceOption = TRUE ;
      }

      /******************************************************************
       *              PREPARE PHASE                                     *
       ******************************************************************/
      // send request to catalog
      rc = executeOnCataGroup ( pMsg, cb, &groupLst, NULL, TRUE,
                                NULL, NULL, buf ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Split failed on catalog, rc: %d", rc ) ;

      // here, in groupLst there should be one and only one group, for SOURCE
      // send request to data-node to find the partitioning key
      // Extract the SplitQuery Field and build a query request to send to data
      // node
      rc = msgExtractQuery ( (CHAR*)pSplitReq, NULL, &pCommandName,
                             NULL, NULL, &pQuery,
                             NULL, NULL, NULL ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to extract query, rc: %d", rc ) ;
      try
      {
         /***************************************************************
          *             DO SOME VALIDATION HERE                         *
          ***************************************************************/
         BSONObj boQuery ( pQuery ) ;

         // get collection name and query
         BSONElement beName = boQuery.getField ( CAT_COLLECTION_NAME ) ;
         BSONElement beSplitQuery =
               boQuery.getField ( CAT_SPLITQUERY_NAME ) ;
         BSONElement beSplitEndQuery ;
         BSONElement beSource = boQuery.getField ( CAT_SOURCE_NAME ) ;
         BSONElement beTarget = boQuery.getField ( CAT_TARGET_NAME ) ;
         BSONElement beAsync  = boQuery.getField ( FIELD_NAME_ASYNC ) ;
         percent = boQuery.getField( CAT_SPLITPERCENT_NAME ).numberDouble() ;
         // collection name verify
         PD_CHECK ( !beName.eoo() && beName.type () == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Failed to process split prepare, unable to find "
                    "collection name field" ) ;
         // now strName is the name of collection
         strName = beName.valuestr() ;
         // get source group name
         PD_CHECK ( !beSource.eoo() && beSource.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to find source field" ) ;
         rc = catGroupNameValidate ( beSource.valuestr() ) ;
         PD_CHECK ( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                    "Source name is not valid: %s",
                    beSource.valuestr() ) ;
         ossStrncpy ( szSource, beSource.valuestr(), sizeof(szSource) ) ;

         // get target group name
         PD_CHECK ( !beTarget.eoo() && beTarget.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Unable to find target field" ) ;
         rc = catGroupNameValidate ( beTarget.valuestr() ) ;
         PD_CHECK ( SDB_OK == rc, SDB_INVALIDARG, error, PDERROR,
                    "Target name is not valid: %s",
                    beTarget.valuestr() ) ;
         ossStrncpy ( szTarget, beTarget.valuestr(), sizeof(szTarget) ) ;

         // async check
         if ( Bool == beAsync.type() )
         {
            async = beAsync.Bool() ? TRUE : FALSE ;
         }
         else if ( !beAsync.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] error", FIELD_NAME_ASYNC,
                    beAsync.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // make sure we have either split value or split query
         if ( !beSplitQuery.eoo() )
         {
            PD_CHECK ( beSplitQuery.type() == Object,
                       SDB_INVALIDARG, error, PDERROR,
                       "Split is not defined or not valid" ) ;
            beSplitEndQuery = boQuery.getField ( CAT_SPLITENDQUERY_NAME ) ;
            if ( !beSplitEndQuery.eoo() )
            {
               PD_CHECK ( beSplitEndQuery.type() == Object,
                          SDB_INVALIDARG, error, PDERROR,
                          "Split is not defined or not valid" ) ;
            }
         }
         else
         {
            PD_CHECK( percent > 0.0 && percent <= 100.0,
                      SDB_INVALIDARG, error, PDERROR,
                      "Split percent value is error" ) ;
         }

         // get sharding key, always get the newest version from catalog
         rc = rtnCoordGetCataInfo ( cb, strName, TRUE, cataInfo ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to get cata info for collection %s, rc: %d",
                       strName, rc ) ;
         // sharding key must exist, we should NEVER hit this check because the
         // check already done in catalog in PREPARE phase
         cataInfo->getShardingKey ( boShardingKey ) ;
         PD_CHECK ( !boShardingKey.isEmpty(),
                    SDB_COLLECTION_NOTSHARD, error, PDWARNING,
                    "Collection must be sharded: %s",
                    strName ) ;

         /*********************************************************************
          *           GET THE SHARDING KEY VALUE FROM SOURCE                  *
          *********************************************************************/
         if ( cataInfo->getCatalogSet()->isHashSharding() )
         {
            if ( !beSplitQuery.eoo() )
            {
               BSONObj tmpStart = beSplitQuery.embeddedObject() ;
               BSONObjBuilder tmpStartBuilder ;
               tmpStartBuilder.appendElementsWithoutName( tmpStart ) ;
               boKeyStart = tmpStartBuilder.obj() ;
               if ( !beSplitEndQuery.eoo() )
               {
                  BSONObj tmpEnd = beSplitEndQuery.embeddedObject() ;
                  BSONObjBuilder tmpEndBuilder ;
                  tmpEndBuilder.appendElementsWithoutName( tmpEnd ) ;
                  boKeyEnd = tmpEndBuilder.obj() ;
               }
            }
         }
         else
         {
            if ( beSplitQuery.eoo())
            {
               rc = _getBoundByPercent( strName, percent, cataInfo,
                                        groupLst, cb, boKeyStart, boKeyEnd,
                                        buf ) ;
            }
            else
            {
               rc = _getBoundByCondition( strName,
                                          beSplitQuery.embeddedObject(),
                                          beSplitEndQuery.eoo() ?
                                          BSONObj():
                                          beSplitEndQuery.embeddedObject(),
                                          groupLst,
                                          cb,
                                          cataInfo,
                                          boKeyStart, boKeyEnd,
                                          buf ) ;
            }

            PD_RC_CHECK( rc, PDERROR, "Failed to get bound, rc: %d",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when query from remote node: %s",
                       e.what() ) ;
      }

      /************************************************************************
       *         SHARDING READY REQUEST                                       *
       ************************************************************************/
      // now boKeyStart contains the key we want to split, let's construct a new
      // request for split ready
      try
      {
         BSONObj boSend ;

         // construct the record that we are going to send to catalog
         boSend = BSON ( CAT_COLLECTION_NAME << strName <<
                         CAT_SOURCE_NAME << szSource <<
                         CAT_TARGET_NAME << szTarget <<
                         CAT_SPLITPERCENT_NAME << percent <<
                         CAT_SPLITVALUE_NAME << boKeyStart <<
                         CAT_SPLITENDVALUE_NAME << boKeyEnd <<
                         CAT_TASKID_NAME << (long long)taskID ) ;
         taskInfoObj = boSend ;
         rc = msgBuildQueryMsg ( &splitReadyBuffer, &splitReadyBufferSz,
                                 CMD_ADMIN_PREFIX CMD_NAME_SPLIT, 0,
                                 0, 0, -1, &boSend, NULL,
                                 NULL, NULL ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to build query message, rc: %d",
                       rc ) ;
         pSplitReq                        = (MsgOpQuery *)splitReadyBuffer ;
         pSplitReq->header.opCode         = MSG_CAT_SPLIT_READY_REQ ;
         pSplitReq->version               = cataInfo->getVersion() ;

         rc = executeOnCataGroup ( (MsgHeader*)pSplitReq, cb, &groupDstLst,
                                   &boRecv, TRUE, NULL, NULL, buf ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to execute split ready on catalog, rc: %d",
                       rc ) ;

         // Get task ID
         if ( boRecv.empty() )
         {
            PD_LOG( PDERROR, "Failed to get task id from result msg" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         taskID = (UINT64)(boRecv.at(0).getField( CAT_TASKID_NAME ).numberLong()) ;

         // Construct split query request to destination Data group
         boSend = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
         rc = msgBuildQueryMsg( &splitQueryBuffer, &splitQueryBufferSz,
                                CMD_ADMIN_PREFIX CMD_NAME_SPLIT, 0,
                                0, 0, -1, &boSend, NULL,
                                NULL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build query message, rc: %d",
                      rc ) ;
         pSplitQuery                      = (MsgOpQuery *)splitQueryBuffer ;
         pSplitQuery->version             = cataInfo->getVersion() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when building split ready message: %s",
                       e.what() ) ;
      }
      /************************************************************************
       *           SHARDING START REQUEST                                     *
       ************************************************************************/
      // before sending to data node, we have to convert the request to QUERY
      pSplitReq->header.opCode = MSG_BS_QUERY_REQ ;
      rc = executeOnCL( (MsgHeader *)splitReadyBuffer, cb, strName,
                        FALSE, &groupDstLst, NULL, NULL, NULL, buf ) ;
      // If we get error here, something big happend. We have marked ready to
      // split on catalog but data node refused to do so.
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute split on data node, rc: %d",
                   rc ) ;

      // if sync, need to wait task finished
      if ( !async )
      {
         rtnCoordProcesserFactory *pFactory = pCoordcb->getProcesserFactory() ;
         rtnCoordCommand *pCmd = pFactory->getCommandProcesser(
                                 COORD_CMD_WAITTASK ) ;
         SDB_ASSERT( pCmd, "wait task command not found" ) ;
         rc = pCmd->execute( (MsgHeader*)splitQueryBuffer, cb,
                             contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING,
                    "Wait task [%lld] failed, rc: %d",
                    taskID, rc ) ;
            rc = SDB_OK ;
            /// can not report error, because split already created
         }
      }
      else // return taskid to client
      {
         rtnContextDump *pContext = NULL ;
         rc = pRtncb->contextNew( RTN_CONTEXT_DUMP, (rtnContext**)&pContext,
                                  contextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create context, rc: %d", rc ) ;
         rc = pContext->open( BSONObj(), BSONObj(), 1, 0 ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;
         pContext->append( BSON( CAT_TASKID_NAME << (long long)taskID ) ) ;
      }

   done :
      if ( NULL != coordSession && replacedInstanceOption )
      {
         coordSession->setInstanceOption( instanceOption ) ;
      }
      if ( pCommandName && strName )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, pCommandName + 1, AUDIT_OBJ_CL,
                           strName, rc, "Option:%s, TaskID:%llu",
                           taskInfoObj.toString().c_str(), taskID ) ;
      }
      SAFE_OSS_FREE( splitReadyBuffer ) ;
      SAFE_OSS_FREE( splitQueryBuffer ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOCMDSP_EXE, rc ) ;
      return rc ;
   error :
      if ( CLS_INVALID_TASKID != taskID )
      {
         // Need to delete catContext allocated with taskID, send the cancel
         // request anyway
         INT32 tmpRC = SDB_OK ;
         if ( !pSplitQuery )
         {
            // Construct the cancel request with task ID
            BSONObj boCancel = BSON( CAT_TASKID_NAME << (long long)taskID ) ;
            tmpRC = msgBuildQueryMsg( &splitQueryBuffer, &splitQueryBufferSz,
                                      CMD_ADMIN_PREFIX CMD_NAME_SPLIT, 0,
                                      0, 0, -1, &boCancel, NULL,
                                      NULL, NULL ) ;
            if ( SDB_OK == tmpRC )
            {
               pSplitQuery = (MsgOpQuery *)splitQueryBuffer ;
            }
            else
            {
               PD_LOG( PDWARNING,
                       "Failed to execute split cancel on catalog, rc: %d",
                       tmpRC ) ;
            }
         }
         if ( pSplitQuery )
         {
            // Send the request
            pSplitQuery->header.opCode = MSG_CAT_SPLIT_CANCEL_REQ ;
            tmpRC = executeOnCataGroup ( (MsgHeader*)pSplitQuery,
                                         cb, TRUE, NULL, NULL, buf ) ;
            if ( tmpRC )
            {
               PD_LOG( PDWARNING,
                       "Failed to execute split cancel on catalog, rc: %d",
                       tmpRC ) ;
            }
         }
      }
      if ( SDB_RTN_INVALID_HINT == rc )
      {
         rc = SDB_COORD_SPLIT_NO_SHDIDX ;
      }
      if ( -1 != contextID )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP_GETBOUNDRONDATA, "rtnCoordCMDSplit::_getBoundRecordOnData" )
   INT32 rtnCoordCMDSplit::_getBoundRecordOnData( const CHAR *cl,
                                                  const BSONObj &condition,
                                                  const BSONObj &hint,
                                                  const BSONObj &sort,
                                                  INT32 flag,
                                                  INT64 skip,
                                                  CoordGroupList &groupList,
                                                  pmdEDUCB *cb,
                                                  BSONObj &shardingKey,
                                                  BSONObj &record,
                                                  rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP_GETBOUNDRONDATA ) ;
      INT32 rc = SDB_OK ;
      BSONObj empty ;
      rtnContext *context = NULL ;
      rtnContextBuf buffObj ;
      BSONObj obj ;

      // check condition has invalid fileds
      if ( !condition.okForStorage() )
      {
         PD_LOG( PDERROR,
                 "Condition [%s] has invalid field name",
                 condition.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( condition.isEmpty() )
      {
         rc = rtnCoordNodeQuery( cl, condition, empty, sort,
                                 hint, skip, 1, groupList,
                                 cb, &context, NULL, flag, buf ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to query from data group, rc: %d",
                       rc ) ;
         rc = context->getMore( -1, buffObj, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            obj = BSONObj( buffObj.data() ) ;
         }
      }
      else
      {
         obj = condition ;
      }

      // product split key
      {
         PD_LOG ( PDINFO,
                  "Split found record %s",
                  obj.toString().c_str() ) ;
         // we need to compare with boShardingKey and extract the partition key
         ixmIndexKeyGen keyGen ( shardingKey ) ;
         BSONObjSet keys ;
         BSONObjSet::iterator keyIter ;
         rc = keyGen.getKeys ( obj, keys, NULL, TRUE ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to extract keys\nkeyDef: %s\n"
                       "record: %s\nrc: %d", shardingKey.toString().c_str(),
                       obj.toString().c_str(), rc ) ;
         // make sure there is one and only one element in the keys
         PD_CHECK ( keys.size() == 1,
                    SDB_INVALID_SHARDINGKEY, error, PDWARNING,
                    "There must be a single key generate for "
                    "sharding\nkeyDef = %s\nrecord = %s\n",
                    shardingKey.toString().c_str(),
                    obj.toString().c_str() ) ;

         keyIter = keys.begin () ;
         record = (*keyIter).copy() ;

         // validate key does not contains Undefined
         /*{
            BSONObjIterator iter ( record ) ;
            while ( iter.more () )
            {
               BSONElement e = iter.next () ;
               PD_CHECK ( e.type() != Undefined, SDB_CLS_BAD_SPLIT_KEY,
                          error, PDERROR, "The split record does not contains "
                          "a valid key\nRecord: %s\nShardingKey: %s\n"
                          "SplitKey: %s", obj.toString().c_str(),
                          shardingKey.toString().c_str(),
                          record.toString().c_str() ) ;
            }
         }*/

        PD_LOG ( PDINFO, "Split found key %s", record.toString().c_str() ) ;
     }

   done:
      if ( NULL != context )
      {
         SINT64 contextID = context->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP_GETBOUNDRONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP__GETBOUNDBYC, "rtnCoordCMDSplit::_getBoundByCondition" )
   INT32 rtnCoordCMDSplit::_getBoundByCondition( const CHAR *cl,
                                                 const BSONObj &begin,
                                                 const BSONObj &end,
                                                 CoordGroupList &groupList,
                                                 pmdEDUCB *cb,
                                                 CoordCataInfoPtr &cataInfo,
                                                 BSONObj &lowBound,
                                                 BSONObj &upBound,
                                                 rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP__GETBOUNDBYC ) ;
      INT32 rc = SDB_OK ;
      /// coord send will clear group list.
      CoordGroupList grpTmp = groupList ;
      BSONObj shardingKey ;
      cataInfo->getShardingKey( shardingKey ) ;
      PD_CHECK ( !shardingKey.isEmpty(),
                 SDB_COLLECTION_NOTSHARD, error, PDWARNING,
                 "Collection must be sharded: %s",
                 cl ) ;

      rc = _getBoundRecordOnData( cl, begin, BSONObj(),BSONObj(),
                                  0, 0, grpTmp, cb,
                                  shardingKey, lowBound, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to get begin bound, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( !end.isEmpty() )
      {
         grpTmp = groupList ;
         rc = _getBoundRecordOnData( cl, end, BSONObj(),BSONObj(),
                                     0, 0, grpTmp, cb,
                                     shardingKey, upBound, buf ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR,
                    "Failed to get end bound, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP__GETBOUNDBYC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDSP__GETBOUNDBYP, "rtnCoordCMDSplit::_getBoundByPercent" )
   INT32 rtnCoordCMDSplit::_getBoundByPercent( const CHAR *cl,
                                               FLOAT64 percent,
                                               CoordCataInfoPtr &cataInfo,
                                               CoordGroupList &groupList,
                                               pmdEDUCB *cb,
                                               BSONObj &lowBound,
                                               BSONObj &upBound,
                                               rtnContextBuf *buf )
   {
      PD_TRACE_ENTRY( SDB_RTNCOCMDSP__GETBOUNDBYP ) ;
      INT32 rc = SDB_OK ;
      BSONObj shardingKey ;
      cataInfo->getShardingKey ( shardingKey ) ;
      CoordGroupList grpTmp = groupList ;
      PD_CHECK ( !shardingKey.isEmpty(),
                 SDB_COLLECTION_NOTSHARD, error, PDWARNING,
                 "Collection must be sharded: %s",
                 cl ) ;

      // if split percent is 100.0%, get the group low bound
      if ( 100.0 - percent < OSS_EPSILON )
      {
         rc = cataInfo->getGroupLowBound( grpTmp.begin()->second,
                                          lowBound ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get group [%d] low bound, rc: %d",
                      grpTmp.begin()->second, rc ) ;
      }
      else
      {
         UINT64 totalCount = 0 ;
         INT64 skipCount = 0 ;
         INT32 flag = 0 ;
         BSONObj hint ;
         while ( TRUE )
         {
            rc = _getCLCount( cl, grpTmp, cb, totalCount, buf ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get collection count, rc: %d",
                         rc ) ;
            if ( 0 == totalCount )
            {
               rc = SDB_DMS_EMPTY_COLLECTION ;
               PD_LOG( PDDEBUG, "collection [%s] is empty", cl ) ;
               break ;
            }

            skipCount = (INT64)(totalCount * ( ( 100 - percent ) / 100 ) ) ;
            hint = BSON( "" << "" ) ;
            flag = FLG_QUERY_FORCE_HINT ;

            /// sort by shardingKey that if $shard index does not exist
            /// can still match index.
            rc = _getBoundRecordOnData( cl, BSONObj(), hint, shardingKey,
                                        flag, skipCount, grpTmp,
                                        cb, shardingKey, lowBound, buf ) ;
            if ( SDB_DMS_EOC == rc )
            {
               continue ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR,
                       "Failed to get bound from data, rc: %d",
                       rc ) ;
               goto error ;
            }
            else
            {
               break ;
            }
         }
      }

      /// upbound always be empty.
      upBound = BSONObj() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNCOCMDSP__GETBOUNDBYP, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   /*
    * rtnCoordCMDCreateIndex implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDCreateIndex::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDCreateIndexArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATEIDX_PARSEMSG, "rtnCoordCMDCreateIndex::_parseMsg" )
   INT32 rtnCoordCMDCreateIndex::_parseMsg ( MsgHeader *pMsg,
                                             _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATEIDX_PARSEMSG ) ;

      _rtnCMDCreateIndexArgs *pSelfArgs = ( _rtnCMDCreateIndexArgs * )pArgs ;

      try
      {
         string clName, idxName ;
         BSONObj boIndex ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery, CAT_COLLECTION,
                                      clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION ) ;
         PD_CHECK( !clName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection name can't be empty!",
                   _getCommandName() ) ;

         rc = rtnGetObjElement( pSelfArgs->_boQuery, FIELD_NAME_INDEX, boIndex ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_INDEX ) ;

         // get embedded index name
         rc = rtnGetSTDStringElement( boIndex, IXM_FIELD_NAME_NAME, idxName );
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to %s: failed to get the field [%s] from query",
                       _getCommandName(), IXM_FIELD_NAME_NAME ) ;
         PD_CHECK( !idxName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: index name can't be empty!",
                   _getCommandName() ) ;

         pSelfArgs->_targetName = clName ;
         pSelfArgs->_indexName = idxName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATEIDX_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATEIDX_GENCATAMSG, "rtnCoordCMDCreateIndex::_generateCataMsg" )
   INT32 rtnCoordCMDCreateIndex::_generateCataMsg( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   CHAR **ppMsgBuf,
                                                   MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATEIDX_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_CREATE_IDX_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDCREATEIDX_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATEIDX_GENROLLBACKMSG, "rtnCoordCMDCreateIndex::_generateRollbackDataMsg" )
   INT32 rtnCoordCMDCreateIndex::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                            _rtnCMDArguments *pArgs,
                                                            CHAR **ppMsgBuf,
                                                            MsgHeader **ppRollbackMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATEIDX_GENROLLBACKMSG ) ;

      INT32 bufSize = 0 ;

      _rtnCMDCreateIndexArgs *pSelfArgs = ( _rtnCMDCreateIndexArgs * ) pArgs ;

      rc = msgBuildDropIndexMsg( ppMsgBuf, &bufSize,
                                 pSelfArgs->_targetName.c_str(),
                                 pSelfArgs->_indexName.c_str(), 0 ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to rollback %s on [%s]: "
                    "failed to build drop index message, rc: %d",
                    _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;

      (*ppRollbackMsg) = (MsgHeader *)(*ppMsgBuf) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATEIDX_GENROLLBACKMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATEIDX_ROLLBACK, "rtnCoordCMDCreateIndex::_rollbackOnDataGroup" )
   INT32 rtnCoordCMDCreateIndex::_rollbackOnDataGroup ( MsgHeader *pMsg,
                                                        pmdEDUCB *cb,
                                                        _rtnCMDArguments *pArgs,
                                                        const CoordGroupList &groupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATEIDX_ROLLBACK ) ;

      CoordCataInfoPtr cataInfo ;
      SET_RC ignoreRC ;

      // let's get most current version again
      rc = rtnCoordGetCataInfo( cb, pArgs->_targetName.c_str(), FALSE, cataInfo ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback %s on [%s]: "
                   "catalog info for collection[%s] failed, rc: %d)",
                   _getCommandName(), pArgs->_targetName.c_str(),
                   pArgs->_targetName.c_str(), rc ) ;
      PD_CHECK( !cataInfo->isMainCL(),
                SDB_OK, error, PDWARNING,
                "Failed to rollback %s on [%s]:"
                "main-collection create index failed and will not rollback",
                _getCommandName(), pArgs->_targetName.c_str() ) ;

      ignoreRC.insert( SDB_IXM_NOTEXIST ) ;
      rc = executeOnCL( pMsg, cb, pArgs->_targetName.c_str(),
                        FALSE, &groupLst, &ignoreRC, NULL,
                        NULL, pArgs->_pBuf ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback %s on [%s], rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATEIDX_ROLLBACK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDDropIndex define
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPIDX_PARSEMSG, "rtnCoordCMDDropIndex::_parseMsg" )
   INT32 rtnCoordCMDDropIndex::_parseMsg ( MsgHeader *pMsg,
                                           _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPIDX_PARSEMSG ) ;

      try
      {
         string clName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_COLLECTION,
                                      clName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_COLLECTION ) ;
         PD_CHECK( !clName.empty(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to %s: collection name can't be empty!",
                   _getCommandName() ) ;

         pArgs->_targetName = clName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDDROPIDX_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDDROPIDX_GENCATAMSG, "rtnCoordCMDDropIndex::_generateCataMsg" )
   INT32 rtnCoordCMDDropIndex::_generateCataMsg( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 _rtnCMDArguments *pArgs,
                                                 CHAR **ppMsgBuf,
                                                 MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDDROPIDX_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_DROP_IDX_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDDROPIDX_GENCATAMSG ) ;

      return SDB_OK ;
   }
}
