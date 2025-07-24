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

   Source File Name = rtnCoordNodeCommands.cpp

   Descriptive Name = Runtime Coord Commands for Node Management

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
#include "rtnCoordNodeCommands.hpp"
#include "msgCatalog.hpp"
#include "catCommon.hpp"
#include "../bson/bson.h"
#include "pmdOptions.hpp"
#include "rtnRemoteExec.hpp"
#include "dms.hpp"
#include "omagentDef.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnCommand.hpp"
#include "coordSession.hpp"
#include "mthModifier.hpp"
#include "rtnCoordDef.hpp"
#include "utilCommon.hpp"

using namespace bson;

namespace engine
{
   /*
    * rtnCoordNodeCMD2Phase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMD2PH_GENDATAMSG, "rtnCoordNodeCMD2Phase::_generateDataMsg" )
   INT32 rtnCoordNodeCMD2Phase::_generateDataMsg ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   const vector<BSONObj> &cataObjs,
                                                   CHAR **ppMsgBuf,
                                                   MsgHeader **ppDataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCONODECMD2PH_GENDATAMSG ) ;

      (*ppDataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCONODECMD2PH_GENDATAMSG  ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMD2PH_GENROLLBACKMSG, "rtnCoordNodeCMD2Phase::_generateRollbackDataMsg" )
   INT32 rtnCoordNodeCMD2Phase::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                           _rtnCMDArguments *pArgs,
                                                           CHAR **ppMsgBuf,
                                                           MsgHeader **ppRollbackMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCONODECMD2PH_GENROLLBACKMSG ) ;

      (*ppRollbackMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCONODECMD2PH_GENROLLBACKMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMD2PH_DOAUDIT, "rtnCoordNodeCMD2Phase::_doAudit" )
   INT32 rtnCoordNodeCMD2Phase::_doAudit ( _rtnCMDArguments *pArgs,
                                           INT32 rc )
   {
      PD_TRACE_ENTRY ( CMD_RTNCONODECMD2PH_DOAUDIT ) ;

      // Do nothing

      PD_TRACE_EXIT ( CMD_RTNCONODECMD2PH_DOAUDIT ) ;

      return SDB_OK ;
   }

   /*
    * rtnCoordNodeCMD3Phase implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMD3PH_DOONCATAP2, "rtnCoordNodeCMD3Phase::_doOnCataGroupP2" )
   INT32 rtnCoordNodeCMD3Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   rtnContextCoord **ppContext,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &pGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( CMD_RTNCONODECMD3PH_DOONCATAP2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( CMD_RTNCONODECMD3PH_DOONCATAP2, rc ) ;

      return rc ;
   }

   /*
      rtnCoordCMDCreateCataGroup implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_EXE, "rtnCoordCMDCreateCataGroup::execute" )
   INT32 rtnCoordCMDCreateCataGroup::execute( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              INT64 &contextID,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_EXE ) ;
      contextID = -1 ;

      INT32 flag = 0 ;
      CHAR *pCMDName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;
      BSONObj boNodeConfig;
      BSONObj boNodeInfo;
      const CHAR *pHostName = NULL;
      SINT32 retCode = 0 ;
      BSONObj boLocalSvc ;
      BSONObj boBackup = BSON( "Backup" << true ) ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse create catalog-group "
                   "request(rc=%d)", rc ) ;

      rc = getNodeConf( pQuery, boNodeConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get configure info(rc=%d)", rc ) ;

      rc = getNodeInfo( pQuery, boNodeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node info(rc=%d)", rc ) ;

      try
      {
         BSONElement beHostName = boNodeInfo.getField( FIELD_NAME_HOST );
         PD_CHECK( beHostName.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", FIELD_NAME_HOST );
         pHostName = beHostName.valuestr() ;

         BSONElement beLocalSvc = boNodeInfo.getField( PMD_OPTION_SVCNAME );
         PD_CHECK( beLocalSvc.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_SVCNAME );
         BSONObjBuilder bobLocalSvc ;
         bobLocalSvc.append( beLocalSvc ) ;
         boLocalSvc = bobLocalSvc.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to create catalog group, occured unexpected "
                   "error:%s", e.what() );
      }

      // Create
      rc = rtnRemoteExec( SDBADD, pHostName, &retCode, &boNodeConfig,
                          &boNodeInfo ) ;
      rc = rc ? rc : retCode ;
      PD_RC_CHECK( rc, PDERROR, "remote node execute(configure) "
                   "failed(rc=%d)", rc ) ;

      // Start
      rc = rtnRemoteExec( SDBSTART, pHostName, &retCode, &boLocalSvc ) ;
      rc = rc ? rc : retCode ;
      // if start catalog failed, need to remove config
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Remote node execute(start) failed(rc=%d)", rc ) ;
         // boBackup for remove node to backup node info
         rtnRemoteExec( SDBRM, pHostName, &retCode, &boNodeConfig, &boBackup ) ;
         goto error ;
      }

      /// fillback catalog addr.
      {
         CoordVecNodeInfo cataList ;
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->getCatNodeAddrList( cataList ) ;
         pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;

         for ( CoordVecNodeInfo::const_iterator itr = cataList.begin() ;
               itr != cataList.end() ;
               itr++ )
         {
            optCB->setCatAddr( itr->_host, itr->_service[
               MSG_ROUTE_CAT_SERVICE].c_str() ) ;
         }
         optCB->reflush2File() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_EXE, rc ) ;
      return rc;
   error:
      // clear the catalog-group info
      if ( rc != SDB_COORD_RECREATE_CATALOG )
      {
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->clearCatNodeAddrList() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

         CoordGroupInfo *pEmptyGroupInfo = NULL ;
         pEmptyGroupInfo = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
         if ( NULL != pEmptyGroupInfo )
         {
            CoordGroupInfoPtr groupInfo( pEmptyGroupInfo ) ;
            sdbGetCoordCB()->updateCatGroupInfo( groupInfo ) ;
            sdbGetCoordCB()->removeGroupInfo( CAT_CATALOG_GROUPID ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_GETNDCF, "rtnCoordCMDCreateCataGroup::getNodeConf" )
   INT32 rtnCoordCMDCreateCataGroup::getNodeConf( CHAR *pQuery,
                                                  BSONObj &boNodeConfig )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_GETNDCF ) ;

      pmdOptionsCB *option = pmdGetOptionCB() ;
      std::string clusterName ;
      std::string businessName ;
      option->getFieldStr( PMD_OPTION_CLUSTER_NAME, clusterName, "" ) ;
      option->getFieldStr( PMD_OPTION_BUSINESS_NAME, businessName, "" ) ;

      try
      {
         std::string strCataHostName ;
         std::string strSvcName ;
         std::string strCataSvc ;
         CoordVecNodeInfo cataNodeLst ;

         BSONObj boInput( pQuery ) ;
         BSONObjBuilder bobNodeConf ;
         BSONObjIterator iter( boInput ) ;

         // loop through each input parameter
         while ( iter.more() )
         {
            BSONElement beField = iter.next() ;
            std::string strFieldName( beField.fieldName() ) ;

            // make sure to skip hostname, group name, and role
            if ( strFieldName == FIELD_NAME_HOST )
            {
               strCataHostName = beField.str();
               continue;
            }
            else if ( strFieldName == FIELD_NAME_GROUPNAME ||
                      strFieldName == PMD_OPTION_ROLE ||
                      strFieldName == PMD_OPTION_CATALOG_ADDR ||
                      strFieldName == PMD_OPTION_CLUSTER_NAME ||
                      strFieldName == PMD_OPTION_BUSINESS_NAME )
            {
               continue;
            }
            else if ( strFieldName == PMD_OPTION_CATANAME )
            {
               strCataSvc = beField.str() ;
            }
            else if ( strFieldName == PMD_OPTION_SVCNAME )
            {
               strSvcName = beField.str() ;
            }

            // append into beField
            bobNodeConf.append( beField ) ;
         }

         if ( strSvcName.empty() )
         {
            PD_LOG( PDERROR, "Service name can't be empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( strCataSvc.empty() )
         {
            UINT16 svcPort = 0 ;

            rc = ossGetPort( strSvcName.c_str(), svcPort ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Invalid svcname: %s", strSvcName.c_str() ) ;
               goto error ;
            }

            CHAR szPort[ 10 ] = { 0 } ;
            ossItoa( svcPort + MSG_ROUTE_CAT_SERVICE , szPort, 10 ) ;
            strCataSvc = szPort ;
         }

         // assign role
         bobNodeConf.append ( PMD_OPTION_ROLE, SDB_ROLE_CATALOG_STR ) ;
         if ( !clusterName.empty() )
         {
            bobNodeConf.append ( PMD_OPTION_CLUSTER_NAME, clusterName ) ;
         }
         if ( !businessName.empty() )
         {
            bobNodeConf.append ( PMD_OPTION_BUSINESS_NAME, businessName ) ;
         }

         // assign catalog address, make sure to include all catalog nodes
         // that configured in the system ( for HA ), each system should be
         // separated by "," and sit in a single key: PMD_OPTION_CATALOG_ADDR
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->getCatNodeAddrList( cataNodeLst ) ;

         // already exist catalog group
         if ( cataNodeLst.size() > 0 )
         {
            rc = SDB_COORD_RECREATE_CATALOG ;
            PD_LOG( PDERROR, "Repeat to create catalog-group" ) ;
            sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;
            goto error ;
         }
         else
         {
            std::string strCataNodeLst = strCataHostName + ":" + strCataSvc ;
            MsgRouteID routeID ;
            routeID.columns.groupID = CATALOG_GROUPID ;
            routeID.columns.nodeID = SYS_NODE_ID_BEGIN ;
            routeID.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
            sdbGetCoordCB()->addCatNodeAddr( routeID, strCataHostName.c_str(),
                                             strCataSvc.c_str() ) ;
            sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

            bobNodeConf.append( PMD_OPTION_CATALOG_ADDR, strCataNodeLst ) ;
         }

         boNodeConfig = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_GETNDCF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTCAGP_GETNDINFO, "rtnCoordCMDCreateCataGroup::getNodeInfo" )
   INT32 rtnCoordCMDCreateCataGroup::getNodeInfo( CHAR *pQuery, BSONObj &boNodeInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTCAGP_GETNDINFO ) ;

      try
      {
         BSONObj boConf( pQuery ) ;
         BSONObjBuilder bobNodeInfo ;
         BSONElement beHostName = boConf.getField( FIELD_NAME_HOST ) ;
         PD_CHECK( beHostName.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", FIELD_NAME_HOST ) ;
         bobNodeInfo.append( beHostName ) ;

         BSONElement beDBPath = boConf.getField( PMD_OPTION_DBPATH ) ;
         PD_CHECK( beDBPath.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_DBPATH );
         bobNodeInfo.append( beDBPath ) ;

         BSONElement beLocalSvc = boConf.getField( PMD_OPTION_SVCNAME ) ;
         PD_CHECK( beLocalSvc.type()==String, SDB_INVALIDARG, error, PDERROR,
                   "Failed to get the field(%s)", PMD_OPTION_SVCNAME ) ;
         bobNodeInfo.append( beLocalSvc ) ;

         BSONElement beReplSvc = boConf.getField( PMD_OPTION_REPLNAME );
         if ( beReplSvc.type() == String )
         {
            bobNodeInfo.append( beReplSvc ) ;
         }

         BSONElement beShardSvc = boConf.getField( PMD_OPTION_SHARDNAME ) ;
         if ( beShardSvc.type() == String )
         {
            bobNodeInfo.append( beShardSvc ) ;
         }

         BSONElement beCataSvc = boConf.getField( PMD_OPTION_CATANAME ) ;
         if ( beCataSvc.type() == String )
         {
            bobNodeInfo.append( beCataSvc ) ;
         }
         boNodeInfo = bobNodeInfo.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Occured unexpected error:%s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTCAGP_GETNDINFO, rc ) ;
      return rc;
   error:
      goto done;
   }

   /*
    * rtnCoordCMDCreateGroup implement
    */

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOCMDCTGR, "rtnCoordCMDCreateGroup::execute" )
   INT32 rtnCoordCMDCreateGroup::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;

      PD_TRACE_ENTRY ( SDB_RTNCOCMDCTGR ) ;

      // fill default-reply(create group success)
      contextID                        = -1 ;

      MsgOpQuery *pCreateReq = (MsgOpQuery *)pMsg ;
      pCreateReq->header.opCode = MSG_CAT_CREATE_GROUP_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute on catalog, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_RTNCOCMDCTGR, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDOpOnNodes implement
    */

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPON1NODE, "_coordCMDOpOnGroup::_opOnNodes" )
   INT32 rtnCoordCMDOpOnNodes::_opOnOneNode ( const vector<INT32> &opList,
                                            string hostName,
                                            string svcName,
                                            vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDOPONGROUP_OPON1NODE ) ;

      try
      {
         BSONObj boExecArg = BSON( FIELD_NAME_HOST << hostName <<
                                   PMD_OPTION_SVCNAME << svcName ) ;

         for ( vector<INT32>::const_iterator iter = opList.begin() ;
               iter != opList.end() ;
               ++iter )
         {
            INT32 retCode = SDB_OK ;
            CM_REMOTE_OP_CODE opCode = (CM_REMOTE_OP_CODE)(*iter) ;

            rc = rtnRemoteExec( opCode, hostName.c_str(),
                                &retCode, &boExecArg ) ;
            if ( SDB_OK == rc && SDB_OK == retCode )
            {
               continue ;
            }
            else
            {
               PD_LOG( PDERROR, "Do remote execute[code:%d] on the node[%s:%s] "
                       "failed, rc: %d, remoteRC: %d",opCode, hostName.c_str(),
                       svcName.c_str(), rc, retCode ) ;
               dataObjs.push_back( BSON( FIELD_NAME_HOST << hostName <<
                                         PMD_OPTION_SVCNAME << svcName <<
                                         OP_ERRNOFIELD << retCode ) ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occurred unexpected error:%s", e.what() ) ;
      }

      PD_TRACE_EXITRC ( COORD_CMDOPONGROUP_OPON1NODE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMDOP_OPONNODES, "rtnCoordCMDOpOnNodes::_opOnNodes" )
   INT32 rtnCoordCMDOpOnNodes::_opOnNodes ( const vector<INT32> &opList,
                                            const BSONObj &boGroupInfo,
                                            vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( CMD_RTNCONODECMDOP_OPONNODES ) ;

      BOOLEAN skipSelf = FALSE ;
      const CHAR* selfHostName = pmdGetKRCB()->getHostName() ;
      const CHAR* selfSvcName  = pmdGetKRCB()->getSvcname() ;

      try
      {
         BSONObj boNodeList ;
         rc = rtnGetArrayElement( boGroupInfo, FIELD_NAME_GROUP, boNodeList ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s], rc: %d",
                      FIELD_NAME_GROUP, rc ) ;

         BSONObjIterator i( boNodeList ) ;
         while ( i.more() )
         {
            BSONElement beNode = i.next() ;
            BSONObj boNode = beNode.embeddedObject() ;
            string hostName, svcName ;

            /// get hostname and svcname of the node
            rc = rtnGetSTDStringElement( boNode, FIELD_NAME_HOST, hostName ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get the field [%s], rc: %d",
                         FIELD_NAME_HOST, rc ) ;

            BSONElement beService = boNode.getField( FIELD_NAME_SERVICE ) ;
            PD_CHECK( !beService.eoo() && Array == beService.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Failed to get the field(%s)",
                      FIELD_NAME_SERVICE );

            rc = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE, svcName );
            PD_CHECK( rc == SDB_OK,
                      SDB_INVALIDARG, error, PDWARNING,
                      "Failed to get local-service-name" ) ;

            /// skip stopping myself
            /// if want to stop itself, we must stop all other first.
            vector<INT32>::const_iterator it = std::find( opList.begin(),
                                                          opList.end(),
                                                          SDBSTOP ) ;
            if( it != opList.end() )
            {
               if (  0 == ossStrcmp( hostName.c_str(), selfHostName ) &&
                     0 == ossStrcmp( svcName.c_str(),  selfSvcName ) )
               {
                  skipSelf = TRUE ;
                  continue ;
               }
            }

            /// do operation
            rc = _opOnOneNode( opList, hostName, svcName, dataObjs ) ;
          }

         /// stop itself after all other node stoped
         if ( skipSelf )
         {
            rc = _opOnOneNode( opList, selfHostName, selfSvcName, dataObjs ) ;
         }
       }
       catch ( std::exception &e )
       {
          rc = SDB_INVALIDARG ;
          PD_LOG ( PDERROR, "Occurred unexpected error:%s", e.what() ) ;
          goto error ;
       }

       if ( dataObjs.size() != 0 )
       {
          if ( _flagIsStartNodes() )
          {
             rc = SDB_CM_RUN_NODE_FAILED ;
          }
          else
          {
             rc = SDB_CM_OP_NODE_FAILED ;
          }
          goto error ;
       }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCONODECMDOP_OPONNODES, rc ) ;
       return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCONODECMDOP_OPONCATANODES, "rtnCoordCMDOpOnNodes::_opOnCataNodes" )
   INT32 rtnCoordCMDOpOnNodes::_opOnCataNodes ( const vector<INT32> &opList,
                                                clsGroupItem *pItem,
                                                vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCONODECMDOP_OPONCATANODES ) ;

      MsgRouteID id ;
      string hostName ;
      string svcName ;
      UINT32 pos = 0 ;

      while ( SDB_OK == pItem->getNodeInfo( pos, id, hostName, svcName,
                                            MSG_ROUTE_LOCAL_SERVICE ) )
      {
         ++pos ;
         rc = _opOnOneNode( opList, hostName, svcName, dataObjs ) ;
      }

      if ( dataObjs.size() != 0 )
      {
         if ( _flagIsStartNodes() )
         {
            rc = SDB_CM_RUN_NODE_FAILED ;
         }
         else
         {
            rc = SDB_CM_OP_NODE_FAILED ;
         }
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCONODECMDOP_OPONCATANODES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void rtnCoordCMDOpOnNodes::_logErrorForNodes ( const string &commandName,
                                                  const string &targetName,
                                                  const vector<BSONObj> &dataObjs )
   {
      UINT32 i = 0;
      string strNodeList ;

      for ( ; i < dataObjs.size(); i++ )
      {
         strNodeList += dataObjs[i].toString( false, false ) ;
      }

      PD_LOG_MSG( PDERROR, "Failed to %s on %s: %s",
                  commandName.c_str(), targetName.c_str(), strNodeList.c_str() ) ;
   }

   /*
    * rtnCoordCMDActiveGroup implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDActiveGroup::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDActiveGroupArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDACTIVEGRP_PARSEMSG, "rtnCoordCMDActiveGroup::_parseMsg" )
   INT32 rtnCoordCMDActiveGroup::_parseMsg ( MsgHeader *pMsg,
                                             _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDACTIVEGRP_PARSEMSG ) ;

      try
      {
         string groupName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      CAT_GROUPNAME_NAME, groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_GROUPNAME_NAME );

         pArgs->_targetName = groupName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDACTIVEGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDACTIVEGRP_GENCATAMSG, "rtnCoordCMDActiveGroup::_generateCataMsg" )
   INT32 rtnCoordCMDActiveGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    _rtnCMDArguments *pArgs,
                                                    CHAR **ppMsgBuf,
                                                    MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDACTIVEGRP_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_ACTIVE_GROUP_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDACTIVEGRP_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDACTIVEGRP_DOONCATA, "rtnCoordCMDActiveGroup::_doOnCataGroup" )
   INT32 rtnCoordCMDActiveGroup::_doOnCataGroup( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 _rtnCMDArguments *pArgs,
                                                 CoordGroupList *pGroupLst,
                                                 vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDACTIVEGRP_DOONCATA ) ;

      _rtnCMDActiveGroupArgs *pSelfArgs = (_rtnCMDActiveGroupArgs *) pArgs ;

      rc = rtnCoordCMD2Phase::_doOnCataGroup( pMsg, cb, ppContext, pArgs,
                                              NULL, pReplyObjs ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to %s on [%s]: "
                 "execute on catalog-node failed, rc: %d",
                 _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;

         // For catalog group only
         if ( 0 == pSelfArgs->_targetName.compare( CATALOG_GROUPNAME ) &&
              SDB_OK == rtnCoordGetLocalCatGroupInfo( pSelfArgs->_catGroupInfo ) &&
              pSelfArgs->_catGroupInfo.get() &&
              pSelfArgs->_catGroupInfo->nodeCount() > 0 )
         {
            rc = SDB_OK ;
         }
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "execute on catalog-node failed, rc: %d",
                   _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDACTIVEGRP_DOONCATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDACTIVEGRP_DOONDATA, "rtnCoordCMDActiveGroup::_doOnDataGroup" )
   INT32 rtnCoordCMDActiveGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextCoord **ppContext,
                                                  _rtnCMDArguments *pArgs,
                                                  const CoordGroupList &groupLst,
                                                  const vector<BSONObj> &cataObjs,
                                                  CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDACTIVEGRP_DOONDATA ) ;

      vector<BSONObj> dataObjs ;
      vector<INT32> opList ;

      _rtnCMDActiveGroupArgs *pSelfArgs = (_rtnCMDActiveGroupArgs *) pArgs ;

      opList.push_back( SDBSTART ) ;

      if ( pSelfArgs->_catGroupInfo.get() &&
           pSelfArgs->_catGroupInfo->nodeCount() > 0 )
      {
         // For catalog group
         rc = _opOnCataNodes( opList, pSelfArgs->_catGroupInfo.get(), dataObjs ) ;
      }
      else if ( cataObjs.size() > 0 )
      {
         rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;
      }
      else
      {
         rc = SDB_SYS ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to %s on [%s]: start nodes failed, rc: %d",
                 _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
         _logErrorForNodes( _getCommandName(), pSelfArgs->_targetName, dataObjs ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDACTIVEGRP_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDShutdownGroup implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSHUTDOWNGRP_PARSEMSG, "rtnCoordCMDShutdownGroup::_parseMsg" )
   INT32 rtnCoordCMDShutdownGroup::_parseMsg ( MsgHeader *pMsg,
                                               _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDSHUTDOWNGRP_PARSEMSG ) ;

      try
      {
         string groupName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      CAT_GROUPNAME_NAME, groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_GROUPNAME_NAME );

         pArgs->_targetName = groupName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDSHUTDOWNGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSHUTDOWNGRP_GENCATAMSG, "rtnCoordCMDShutdownGroup::_generateCataMsg" )
   INT32 rtnCoordCMDShutdownGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                      pmdEDUCB *cb,
                                                      _rtnCMDArguments *pArgs,
                                                      CHAR **ppMsgBuf,
                                                      MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDSHUTDOWNGRP_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_SHUTDOWN_GROUP_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDSHUTDOWNGRP_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSHUTDOWNGRP_DOONDATA, "rtnCoordCMDShutdownGroup::_doOnDataGroup" )
   INT32 rtnCoordCMDShutdownGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    rtnContextCoord **ppContext,
                                                    _rtnCMDArguments *pArgs,
                                                    const CoordGroupList &groupLst,
                                                    const vector<BSONObj> &cataObjs,
                                                    CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDSHUTDOWNGRP_DOONDATA ) ;

      vector<BSONObj> dataObjs ;
      vector<INT32> opList ;

      PD_CHECK( 1 == cataObjs.size(),
                SDB_SYS, error, PDERROR,
                "Could not find group in catalog" ) ;

      opList.push_back( SDBSTOP ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to stop node, rc: %d",
                 rc ) ;
         UINT32 i = 0;
         string strNodeList;
         for ( ; i < dataObjs.size(); i++ )
         {
            strNodeList += dataObjs[i].toString( false, false );
         }
         PD_LOG_MSG( PDERROR,
                     "Failed to stop nodes: %s",
                     strNodeList.c_str() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDSHUTDOWNGRP_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSHUTDOWNGRP_DOCOMMIT, "rtnCoordCMDShutdownGroup::_doCommit" )
   INT32 rtnCoordCMDShutdownGroup::_doCommit ( MsgHeader *pMsg,
                                               pmdEDUCB * cb,
                                               rtnContextCoord **ppContext,
                                               _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDSHUTDOWNGRP_DOCOMMIT ) ;

      rc = rtnCoordNodeCMD2Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         if ( SDB_OK != rc )
         {
            // Should have problem (e.g. -79) when removing Catalog group
            PD_LOG( PDWARNING, "Ignored error %d when %s on [%s]",
                    rc, _getCommandName(), pArgs->_targetName.c_str() ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXITRC ( CMD_RTNCOCMDSHUTDOWNGRP_DOCOMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDSHUTDOWNGRP_DOAUDIT, "rtnCoordCMDShutdownGroup::_doAudit" )
   INT32 rtnCoordCMDShutdownGroup::_doAudit ( _rtnCMDArguments *pArgs,
                                              INT32 rc )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDSHUTDOWNGRP_DOAUDIT ) ;

      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_SYSTEM, _getCommandName(),
                           AUDIT_OBJ_GROUP, pArgs->_targetName.c_str(),
                           rc, "" ) ;
      }

      PD_TRACE_EXIT ( CMD_RTNCOCMDSHUTDOWNGRP_DOAUDIT );

      return SDB_OK ;
   }

   /*
    * rtnCoordCMDRemoveGroup implement
    */
   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMGRP_PARSEMSG, "rtnCoordCMDRemoveGroup::_parseMsg" )
   INT32 rtnCoordCMDRemoveGroup::_parseMsg ( MsgHeader *pMsg,
                                             _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMGRP_PARSEMSG ) ;

      try
      {
         string groupName ;

         rc = rtnGetSTDStringElement( pArgs->_boQuery,
                                      CAT_GROUPNAME_NAME, groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_GROUPNAME_NAME ) ;

         pArgs->_targetName = groupName ;

      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMGRP_GENCATAMSG, "rtnCoordCMDRemoveGroup::_generateCataMsg" )
   INT32 rtnCoordCMDRemoveGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    _rtnCMDArguments *pArgs,
                                                    CHAR **ppMsgBuf,
                                                    MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMGRP_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_RM_GROUP_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDRMGRP_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMGRP_DOONDATA, "rtnCoordCMDRemoveGroup::_doOnDataGroup" )
   INT32 rtnCoordCMDRemoveGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextCoord **ppContext,
                                                  _rtnCMDArguments *pArgs,
                                                  const CoordGroupList &groupLst,
                                                  const vector<BSONObj> &cataObjs,
                                                  CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMGRP_DOONDATA ) ;

      vector<INT32> opList ;
      vector<BSONObj> dataObjs ;

      PD_CHECK( 1 == cataObjs.size(),
                SDB_SYS, error, PDERROR,
                "Failed to %s on [%s]: could not find group in catalog",
                _getCommandName(), pArgs->_targetName.c_str() ) ;

      opList.push_back( SDBTEST ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to %s on [%s]: test nodes failed, rc: %d",
                 _getCommandName(), pArgs->_targetName.c_str(), rc ) ;
         _logErrorForNodes( _getCommandName(), pArgs->_targetName, dataObjs ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMGRP_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMGRP_DOONDATAP2, "rtnCoordCMDRemoveGroup::_doOnDataGroupP2" )
   INT32 rtnCoordCMDRemoveGroup::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    rtnContextCoord **ppContext,
                                                    _rtnCMDArguments *pArgs,
                                                    const CoordGroupList &groupLst,
                                                    const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMGRP_DOONDATAP2 ) ;

      vector<INT32> opList ;
      vector<BSONObj> dataObjs ;
      UINT32 groupID = INVALID_GROUPID ;

      PD_CHECK( 1 == cataObjs.size(),
                SDB_SYS, error, PDERROR,
                "Failed to %s on [%s], could not find group in catalog",
                _getCommandName(), pArgs->_targetName.c_str() ) ;

      rc = rtnGetIntElement( cataObjs[0], FIELD_NAME_GROUPID, (INT32 &)groupID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "failed to get the field [%s], rc: %d",
                   _getCommandName(), pArgs->_targetName.c_str(),
                   FIELD_NAME_GROUPID, rc ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         // clean catalog info
         sdbGetCoordCB()->getLock( EXCLUSIVE ) ;
         sdbGetCoordCB()->clearCatNodeAddrList() ;
         sdbGetCoordCB()->releaseLock( EXCLUSIVE ) ;

         CoordGroupInfo *pEmptyGroupInfo = NULL ;
         pEmptyGroupInfo = SDB_OSS_NEW CoordGroupInfo( CAT_CATALOG_GROUPID ) ;
         if ( NULL != pEmptyGroupInfo )
         {
            CoordGroupInfoPtr groupInfo( pEmptyGroupInfo ) ;
            sdbGetCoordCB()->updateCatGroupInfo( groupInfo ) ;
            sdbGetCoordCB()->invalidateGroupInfo() ;
         }
      }

      opList.push_back( SDBSTOP ) ;
      opList.push_back( SDBRM ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;

      rtnCoordRemoveGroup( groupID ) ;
      {
         CoordSession *session = cb->getCoordSession();
         if ( NULL != session )
         {
            session->removeLastNode( groupID ) ;
         }
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to %s on [%s]: stop nodes failed, rc: %d",
                 _getCommandName(), pArgs->_targetName.c_str(), rc ) ;
         _logErrorForNodes( _getCommandName(), pArgs->_targetName, dataObjs ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMGRP_DOONDATAP2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMGRP_DOCOMMIT, "rtnCoordCMDRemoveGroup::_doCommit" )
   INT32 rtnCoordCMDRemoveGroup::_doCommit ( MsgHeader *pMsg,
                                             pmdEDUCB * cb,
                                             rtnContextCoord **ppContext,
                                             _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMGRP_DOCOMMIT ) ;

      rc = rtnCoordNodeCMD3Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         if ( SDB_OK != rc )
         {
            // Should have problem (e.g. -79) when removing Catalog group
            PD_LOG( PDWARNING, "Ignored error %d when %s on [%s]",
                    rc, _getCommandName(), pArgs->_targetName.c_str() ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMGRP_DOCOMMIT, rc ) ;

      return rc ;
   }

   /*
    * rtnCoordCMDCreateNode implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDCreateNode::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDCreateNodeArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_PARSEMSG, "rtnCoordCMDCreateNode::_parseMsg" )
   INT32 rtnCoordCMDCreateNode::_parseMsg ( MsgHeader *pMsg,
                                            _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_PARSEMSG ) ;

      _rtnCMDCreateNodeArgs *pSelfArgs = ( _rtnCMDCreateNodeArgs * ) pArgs ;

      try
      {
         UINT32 validCount = 0 ;
         string groupName ;
         string hostName ;
         string svcname ;
         BOOLEAN onlyAttach = FALSE ;
         BOOLEAN keepData = FALSE ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      CAT_GROUPNAME_NAME, groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_GROUPNAME_NAME ) ;
         ++validCount ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      CAT_HOST_FIELD_NAME, hostName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_HOST_FIELD_NAME ) ;
         ++validCount ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      PMD_OPTION_SVCNAME, svcname ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), PMD_OPTION_SVCNAME ) ;
         ++validCount ;

         rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                    FIELD_NAME_ONLY_ATTACH, onlyAttach ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            onlyAttach = FALSE ;
            rc = SDB_OK ;
         }
         else
         {
            ++validCount ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_ONLY_ATTACH ) ;

         if ( onlyAttach )
         {
            PD_CHECK( 0 != groupName.compare( COORD_GROUPNAME ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to %s: only data-group or catalog-group "
                      "supports \"attachNode\" now", _getCommandName() ) ;

            rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                       FIELD_NAME_KEEP_DATA, keepData ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Not specify the field[%s] or it has" 
                           " an invalid value in options"
                           " when attaching node.", FIELD_NAME_KEEP_DATA ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               ++validCount ;
            }

            if ( (UINT32)pSelfArgs->_boQuery.nFields() > validCount )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Unknown parameters in command's args[%s]",
                       pSelfArgs->_boQuery.toString().c_str() ) ;
               goto error ;
            }
         }
         else
         {
            rc = _checkNodeInfo( pSelfArgs->_boQuery ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to %s: failed to check node info, rc: %d",
                         _getCommandName(), rc ) ;
         }

         pSelfArgs->_targetName = groupName ;
         pSelfArgs->_hostName = hostName ;
         pSelfArgs->_onlyAttach = onlyAttach ;
         pSelfArgs->_keepData = keepData ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_GENCATAMSG, "rtnCoordCMDCreateNode::_generateCataMsg" )
   INT32 rtnCoordCMDCreateNode::_generateCataMsg ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   CHAR **ppMsgBuf,
                                                   MsgHeader **ppCataMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_GENCATAMSG ) ;

      CHAR *pBuf = NULL ;

      _rtnCMDCreateNodeArgs *pSelfArgs = ( _rtnCMDCreateNodeArgs * ) pArgs ;

      if ( pSelfArgs->_onlyAttach )
      {
         INT32 retCode = SDB_OK ;
         INT32 bufSize = 0 ;
         BSONObjBuilder builder ;
         std::vector<BSONObj> objs ;
         BSONObj nodeConf ;
         BSONObj obj ;

         rc = rtnRemoteExec( SDBGETCONF, pSelfArgs->_hostName.c_str(),
                             &retCode, &(pSelfArgs->_boQuery), NULL, NULL, NULL,
                             &objs ) ;
         rc = SDB_OK == rc ? retCode : rc ;
         PD_RC_CHECK( rc, PDERROR,
                    "Failed to %s: failed to get conf of node, rc: %d",
                    _getCommandName(), rc ) ;

         PD_CHECK( 1 == objs.size(),
                   SDB_SYS, error, PDERROR,
                   "Failed to %s: invalid objs's size: %d",
                   _getCommandName(), objs.size() ) ;

         nodeConf = objs.at( 0 ) ;

         builder.append( CAT_GROUPNAME_NAME, pSelfArgs->_targetName ) ;
         builder.append( FIELD_NAME_HOST, pSelfArgs->_hostName ) ;
         builder.appendElements( nodeConf ) ;
         obj = builder.obj() ;
         rc = msgBuildQueryMsg( &pBuf, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE,
                                0, 0, 0, -1,
                                &obj ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to build cata msg, rc: %d",
                      _getCommandName(), rc ) ;

         (*ppCataMsg) = (MsgHeader *)pBuf ;
         (*ppCataMsg)->opCode = MSG_CAT_CREATE_NODE_REQ ;
      }
      else
      {
         pMsg->opCode = MSG_CAT_CREATE_NODE_REQ ;
         (*ppCataMsg) = pMsg ;
      }

   done :
      if ( pBuf )
      {
         (*ppMsgBuf) = pBuf ;
      }

      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_GENCATAMSG, rc ) ;

      return rc ;

   error :
      SAFE_OSS_FREE( pBuf ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_DONODATA, "rtnCoordCMDCreateNode::_doOnDataGroup" )
   INT32 rtnCoordCMDCreateNode::_doOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 _rtnCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs,
                                                 CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_DONODATA ) ;

      _rtnCMDCreateNodeArgs *pSelfArgs = ( _rtnCMDCreateNodeArgs * ) pArgs ;

      if ( !pSelfArgs->_onlyAttach )
      {
         // We need to test the sdbcm
         INT32 retCode = SDB_OK ;
         rc = rtnRemoteExec( SDBTEST, pSelfArgs->_hostName.c_str(), &retCode ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: "
                      "could not connect to sdbcm[HostName: %s], rc: %d",
                      _getCommandName(), pSelfArgs->_targetName.c_str(),
                      pSelfArgs->_hostName.c_str(), rc) ;

         // Acquire the Catalog group info before insert node info into Catalog
         rc = rtnCoordGetCatGroupInfo( cb, TRUE, pSelfArgs->_catGroupInfo ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: "
                      "Failed to get info of catalog group, rc: %d",
                      _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
      }
      else
      {
         // When attaching, sdbcm is tested by getting node configure in
         // previous phase, so do nothing here.
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_DONODATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_DONODATAP2, "rtnCoordCMDCreateNode::_doOnDataGroupP2" )
   INT32 rtnCoordCMDCreateNode::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   rtnContextCoord **ppContext,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &groupLst,
                                                   const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_DONODATAP2 ) ;

      INT32 retCode = SDB_OK ;

      _rtnCMDCreateNodeArgs *pSelfArgs = ( _rtnCMDCreateNodeArgs * ) pArgs ;

      if ( pSelfArgs->_onlyAttach )
      {
         if ( !pSelfArgs->_keepData )
         {
            rc = rtnRemoteExec( SDBCLEARDATA, pSelfArgs->_hostName.c_str(),
                                &retCode, &(pSelfArgs->_boQuery) ) ;
            rc = rc ? rc : retCode ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to %s on [%s]: "
                         "clear data of node failed, rc: %d",
                         _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
         }

         rc = rtnRemoteExec ( SDBSTART, pSelfArgs->_hostName.c_str(),
                              &retCode, &(pSelfArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: start node failed, rc: %d",
                      _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
      }
      else
      {
         BSONObj boNodeConfig ;
         SINT32 retCode;

         rc = _getNodeConf( pSelfArgs->_boQuery, boNodeConfig,
                            pSelfArgs->_catGroupInfo ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: failed to get node config, rc: %d",
                      _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;

         rc = rtnRemoteExec( SDBADD, pSelfArgs->_hostName.c_str(),
                             &retCode, &boNodeConfig ) ;
         rc = rc ? rc : retCode ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s on [%s]: add node failed, rc: %d",
                      _getCommandName(), pSelfArgs->_targetName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_DONODATAP2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_DOCOMPLETE, "rtnCoordCMDCreateNode::_doComplete" )
   INT32 rtnCoordCMDCreateNode::_doComplete ( MsgHeader *pMsg,
                                              pmdEDUCB * cb,
                                              _rtnCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_DOCOMPLETE ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         CoordGroupInfoPtr catGroupInfo ;

         // update catalog group
         rtnCoordGetCatGroupInfo( cb, TRUE, catGroupInfo ) ;
         // notify all nodes
         rtnCataChangeNtyToAllNodes( cb ) ;
      }

      PD_TRACE_EXIT ( CMD_RTNCOCMDCREATENODE_DOCOMPLETE ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_GETNODECONF, "rtnCoordCMDCreateNode::_getNodeConf" )
   INT32 rtnCoordCMDCreateNode::_getNodeConf ( const BSONObj &boQuery,
                                               BSONObj &nodeConf,
                                               const CoordGroupInfoPtr &catGroupInfo )
   {
      INT32 rc   = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_GETNODECONF ) ;

      const CHAR *roleStr = NULL ;
      SDB_ROLE role = SDB_ROLE_DATA ;

      try
      {
         // role and catalogaddr will be added if user doesn't provide
         BSONObjBuilder bobNodeConf ;
         BSONObjIterator iter( boQuery ) ;
         BOOLEAN hasCatalogAddrKey = FALSE ;

         // loop through each input parameter
         while ( iter.more() )
         {
            BSONElement beField = iter.next() ;
            std::string strFieldName( beField.fieldName() ) ;
            // make sure to skip hostname and group name
            if ( strFieldName == FIELD_NAME_HOST ||
                 strFieldName == PMD_OPTION_ROLE )
            {
               continue;
            }
            if ( strFieldName == FIELD_NAME_GROUPNAME )
            {
               if ( 0 == ossStrcmp( CATALOG_GROUPNAME, beField.valuestr() ) )
               {
                  role = SDB_ROLE_CATALOG ;
               }
               else if ( 0 == ossStrcmp( COORD_GROUPNAME, beField.valuestr() ) )
               {
                  role = SDB_ROLE_COORD ;
               }
               continue ;
            }

            // append into beField
            bobNodeConf.append( beField );

            // for the ones we need to add default value
            if ( PMD_OPTION_CATALOG_ADDR == strFieldName )
            {
               hasCatalogAddrKey = TRUE ;
            }
         }
         // assign role if it doesn't include
         roleStr = utilDBRoleStr( role ) ;
         if ( *roleStr == 0 )
         {
            goto error ;
         }
         bobNodeConf.append ( PMD_OPTION_ROLE, roleStr ) ;

         // assign catalog address, make sure to include all catalog nodes
         // that configured in the system ( for HA ), each system should be
         // separated by "," and sit in a single key: PMD_OPTION_CATALOG_ADDR
         if ( !hasCatalogAddrKey )
         {
            MsgRouteID routeID ;
            std::string cataNodeLst = "";
            UINT32 i = 0;
            if ( catGroupInfo->nodeCount() == 0 )
            {
               rc = SDB_CLS_EMPTY_GROUP ;
               PD_LOG ( PDERROR,
                        "Failed to get catalog group info, rc: %d",
                        rc ) ;
               goto error ;
            }

            routeID.value = MSG_INVALID_ROUTEID ;
            string host ;
            string service ;

            while ( SDB_OK == catGroupInfo->getNodeInfo( i, routeID, host,
                                                         service,
                                                         MSG_ROUTE_CAT_SERVICE ) )
            {
               if ( i > 0 )
               {
                  cataNodeLst += "," ;
               }
               cataNodeLst += host + ":" + service ;
               ++i ;
            }
            bobNodeConf.append( PMD_OPTION_CATALOG_ADDR, cataNodeLst ) ;
         }
         nodeConf = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Occurred unexpected error: %s", e.what() );
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_GETNODECONF, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDCREATENODE_CHECKNODEINFO, "rtnCoordCMDCreateNode::_checkNodeInfo" )
   INT32 rtnCoordCMDCreateNode::_checkNodeInfo ( const BSONObj &boQuery )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDCREATENODE_CHECKNODEINFO ) ;

      try
      {
         std::string dummy ;
         BSONElement ele ;

         rc = rtnGetSTDStringElement( boQuery, FIELD_NAME_GROUPNAME, dummy ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s], rc: %d",
                      FIELD_NAME_GROUPNAME, rc ) ;

         rc = rtnGetSTDStringElement( boQuery, FIELD_NAME_HOST, dummy );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s], rc: %d",
                      FIELD_NAME_HOST, rc ) ;

         rc = rtnGetSTDStringElement( boQuery, PMD_OPTION_SVCNAME, dummy );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s], rc: %d",
                      PMD_OPTION_SVCNAME, rc ) ;

         rc = rtnGetSTDStringElement( boQuery, PMD_OPTION_DBPATH, dummy );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s], rc: %d",
                      PMD_OPTION_DBPATH, rc ) ;

         /// check instance ID
         ele = boQuery.getField( PMD_OPTION_INSTANCE_ID ) ;
         if ( ele.eoo() || ele.isNumber() || ele.type() == bson::String )
         {
            UINT32 instanceID = NODE_INSTANCE_ID_UNKNOWN ;
            if ( ele.isNumber() )
            {
               instanceID = ele.numberInt() ;
            }
            else if ( ele.type() == bson::String )
            {
               rc = utilStr2Num( ele.str().c_str(), (INT32 &)instanceID,
                                 UTIL_STR2NUM_DEC ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Fail to convert field[%s] %s to int, rc: %d",
                            PMD_OPTION_INSTANCE_ID, ele.str().c_str(), rc ) ;
            }
            PD_CHECK( utilCheckInstanceID( instanceID, TRUE ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to check field [%s], "
                      "should be %d, or between %d to %d",
                      PMD_OPTION_INSTANCE_ID, NODE_INSTANCE_ID_UNKNOWN,
                      NODE_INSTANCE_ID_MIN + 1, NODE_INSTANCE_ID_MAX - 1 ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", PMD_OPTION_INSTANCE_ID, _getCommandName(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Occurred unexpected error:%s", e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDCREATENODE_CHECKNODEINFO, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * rtnCoordCMDRemoveNode implement
    */
   rtnCoordCMD2Phase::_rtnCMDArguments *rtnCoordCMDRemoveNode::_generateArguments ()
   {
      return SDB_OSS_NEW _rtnCMDRemoveNodeArgs () ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMNODE_PARSEMSG, "rtnCoordCMDRemoveNode::_parseMsg" )
   INT32 rtnCoordCMDRemoveNode::_parseMsg ( MsgHeader *pMsg,
                                            _rtnCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      UINT32 validCount = 0 ;
      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMNODE_PARSEMSG ) ;

      _rtnCMDRemoveNodeArgs *pSelfArgs = ( _rtnCMDRemoveNodeArgs * ) pArgs ;

      try
      {
         string groupName ;
         string hostName ;
         string serviceName ;
         BOOLEAN onlyDetach = FALSE ;
         BOOLEAN keepData = FALSE ;
         BOOLEAN force = FALSE ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      CAT_GROUPNAME_NAME, groupName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_GROUPNAME_NAME ) ;
         ++validCount ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      CAT_HOST_FIELD_NAME, hostName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), CAT_HOST_FIELD_NAME ) ;
         ++validCount ;

         rc = rtnGetSTDStringElement( pSelfArgs->_boQuery,
                                      PMD_OPTION_SVCNAME, serviceName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), PMD_OPTION_SVCNAME ) ;
         ++validCount ;

         rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                    FIELD_NAME_ONLY_DETACH, onlyDetach ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            onlyDetach = FALSE ;
            rc = SDB_OK ;
         }
         else
         {
            ++validCount ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_ONLY_DETACH ) ;

         if ( onlyDetach )
         {
            PD_CHECK( 0 != groupName.compare( COORD_GROUPNAME ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to %s: only data-group or catalog-group "
                      "supports \"detachNode\" now", _getCommandName() ) ;

            rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                       FIELD_NAME_KEEP_DATA, keepData ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Not specify the field[%s] or it has" 
                           " an invalid value in options"
                           " when detaching node.", FIELD_NAME_KEEP_DATA ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               ++validCount ;
            }
         }

         /// if there are both Enforced and enforced, just use Enforced.
         rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                    FIELD_NAME_ENFORCED, force ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else
         {
            ++validCount ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_ENFORCED ) ;

         rc = rtnGetBooleanElement( pSelfArgs->_boQuery,
                                    FIELD_NAME_ENFORCED1, force ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else
         {
            ++validCount ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s: failed to get the field [%s] from query",
                      _getCommandName(), FIELD_NAME_ENFORCED1 ) ;

         if ( (UINT32)pSelfArgs->_boQuery.nFields() > validCount )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown parameters in command's args[%s]",
                    pSelfArgs->_boQuery.toString().c_str() ) ;
            goto error ;
         }

         pSelfArgs->_targetName = groupName ;
         pSelfArgs->_hostName = hostName ;
         pSelfArgs->_serviceName = serviceName ;
         pSelfArgs->_onlyDetach = onlyDetach ;
         pSelfArgs->_keepData = keepData ;
         pSelfArgs->_force = force ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMNODE_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMNODE_GENCATAMSG, "rtnCoordCMDRemoveNode::_generateCataMsg" )
   INT32 rtnCoordCMDRemoveNode::_generateCataMsg ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   _rtnCMDArguments *pArgs,
                                                   CHAR **ppMsgBuf,
                                                   MsgHeader **ppCataMsg )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMNODE_GENCATAMSG ) ;

      pMsg->opCode = MSG_CAT_DEL_NODE_REQ ;
      (*ppCataMsg) = pMsg ;

      PD_TRACE_EXIT ( CMD_RTNCOCMDRMNODE_GENCATAMSG ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMNODE_DOONDATA, "rtnCoordCMDRemoveNode::_doOnDataGroup" )
   INT32 rtnCoordCMDRemoveNode::_doOnDataGroup ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord **ppContext,
                                                 _rtnCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs,
                                                 CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMNODE_DOONDATA ) ;

      _rtnCMDRemoveNodeArgs *pSelfArgs = ( _rtnCMDRemoveNodeArgs * ) pArgs ;

      // We need to test the sdbcm
      INT32 retCode = SDB_OK ;
      rc = rtnRemoteExec ( SDBTEST, pSelfArgs->_hostName.c_str(), &retCode ) ;
      if ( pSelfArgs->_force && SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Remove node is forced, ignore error: %d", rc ) ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "could not connect to sdbcm[HostName: %s], rc: %d",
                   _getCommandName(), pSelfArgs->_targetName.c_str(),
                   pSelfArgs->_hostName.c_str(), rc) ;

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMNODE_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMNODE_DOONDATAP2, "rtnCoordCMDRemoveNode::_doOnDataGroupP2" )
   INT32 rtnCoordCMDRemoveNode::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                   pmdEDUCB *cb,
                                                   rtnContextCoord **ppContext,
                                                   _rtnCMDArguments *pArgs,
                                                   const CoordGroupList &groupLst,
                                                   const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMNODE_DOONDATAP2 ) ;

      INT32 retCode = SDB_OK ;
      netMultiRouteAgent *pAgent = pmdGetKRCB()->getCoordCB()->getRouteAgent() ;
      CoordGroupInfoPtr groupInfo ;

      _rtnCMDRemoveNodeArgs *pSelfArgs = ( _rtnCMDRemoveNodeArgs * ) pArgs ;

      /// get group info from catalog
      rc = rtnCoordGetGroupInfo( cb, pSelfArgs->_targetName.c_str(), TRUE, groupInfo ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to %s on [%s]: "
                   "failed to get groupinfo[%s] from catalog, rc: %d",
                   _getCommandName(), pSelfArgs->_targetName.c_str(),
                   pSelfArgs->_targetName.c_str(), rc ) ;

      /// notify the other nodes to update groupinfo.
      /// here we do not care whether they succeed.
      {
         _MsgClsGInfoUpdated updated ;
         updated.groupID = groupInfo->groupID() ;
         MsgRouteID routeID ;
         UINT32 index = 0 ;

         while ( SDB_OK == groupInfo->getNodeID( index++, routeID,
                                                 MSG_ROUTE_SHARD_SERVCIE ) )
         {
            rtnCoordSendRequestToNodeWithoutReply( (void *)(&updated),
                                                    routeID, pAgent );
         }
      }

      /// ignore the stop result, because remove or clear will
      /// retry to stop in sdbcm
      rc = rtnRemoteExec ( SDBSTOP, pSelfArgs->_hostName.c_str(),
                           &retCode, &(pSelfArgs->_boQuery) ) ;
      rc = rc ? rc : retCode ;
      if ( rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to %s on [%s]: "
                 "stop node[GroupName: %s, HostName: %s, SvcName: %s] failed, "
                 "rc: %d",
                 _getCommandName(), pSelfArgs->_targetName.c_str(),
                 pSelfArgs->_targetName.c_str(), pSelfArgs->_hostName.c_str(),
                 pSelfArgs->_serviceName.c_str(), rc ) ;
      }

      if ( !pSelfArgs->_onlyDetach )
      {
         rc = rtnRemoteExec ( SDBRM, pSelfArgs->_hostName.c_str(),
                              &retCode, &(pSelfArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         if ( rc )
         {
            PD_LOG( pSelfArgs->_force ? PDWARNING : PDERROR,
                    "Failed to %s on %s: "
                    "remove node[GroupName: %s, HostName: %s, SvcName: %s] failed, "
                    "rc: %d",
                    _getCommandName(), pSelfArgs->_targetName.c_str(),
                    pSelfArgs->_targetName.c_str(), pSelfArgs->_hostName.c_str(),
                    pSelfArgs->_serviceName.c_str(), rc ) ;
         }
      }
      else if ( !pSelfArgs->_keepData )
      {
         rc = rtnRemoteExec( SDBCLEARDATA, pSelfArgs->_hostName.c_str(),
                             &retCode, &(pSelfArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         if ( rc )
         {
            PD_LOG( pSelfArgs->_force ? PDWARNING : PDERROR,
                    "Failed to %s on %s: "
                    "Clear node[GroupName: %s, HostName: %s, SvcName: %s] failed, "
                    "rc: %d",
                    _getCommandName(), pSelfArgs->_targetName.c_str(),
                    pSelfArgs->_targetName.c_str(), pSelfArgs->_hostName.c_str(),
                    pSelfArgs->_serviceName.c_str(), rc ) ;
         }
      }

      if ( pSelfArgs->_force )
      {
         // Ignore errors from cm
         PD_LOG( PDWARNING, "Remove node is forced, ignore error: %d", rc ) ;
         rc = SDB_OK ;
      }

   done :
      PD_TRACE_EXITRC ( CMD_RTNCOCMDRMNODE_DOONDATAP2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( CMD_RTNCOCMDRMNODE_DOCOMPLETE, "rtnCoordCMDRemoveNode::_doComplete" )
   INT32 rtnCoordCMDRemoveNode::_doComplete ( MsgHeader *pMsg,
                                              pmdEDUCB * cb,
                                              _rtnCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( CMD_RTNCOCMDRMNODE_DOCOMPLETE ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         CoordGroupInfoPtr catGroupInfo ;

         // update catalog group
         rtnCoordGetCatGroupInfo( cb, TRUE, catGroupInfo ) ;
         // notify all nodes
         rtnCataChangeNtyToAllNodes( cb ) ;
      }

      PD_TRACE_EXIT ( CMD_RTNCOCMDRMNODE_DOCOMPLETE ) ;

      return SDB_OK ;
   }
}
