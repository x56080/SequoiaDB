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

   Source File Name = catDCManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/
#include "catCommon.hpp"
#include "msgCatalog.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "catDCManager.hpp"
#include "clsDCMgr.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _catDCManager implement
   */
   _catDCManager::_catDCManager()
   {
      _pDmsCB = NULL ;
      _pDpsCB = NULL ;
      _pRtnCB = NULL ;
      _pCatCB = NULL ;
      _pEduCB = NULL ;
      _pDCMgr = NULL ;
      _pDCBaseInfo = NULL ;
      _isWritedCmd = FALSE ;
      _isActived = FALSE ;
   }

   _catDCManager::~_catDCManager()
   {
   }

   INT32 _catDCManager::init()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb     = pmdGetKRCB() ;
      _pDmsCB           = krcb->getDMSCB();
      _pDpsCB           = krcb->getDPSCB();
      _pRtnCB           = krcb->getRTNCB();
      _pCatCB           = krcb->getCATLOGUECB();

      _pDCMgr           = SDB_OSS_NEW clsDCMgr() ;
      if ( !_pDCMgr )
      {
         PD_LOG( PDERROR, "Alloc dc manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pDCBaseInfo = _pDCMgr->getDCBaseInfo() ;
      _pCatCB->regEventHandler( this ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::fini ()
   {
      // Check the pointer in case that it is not initialized
      // before unregister the handler
      if ( _pCatCB )
      {
         _pCatCB->unregEventHandler( this ) ;
      }
      _pDCBaseInfo = NULL ;
      if ( _pDCMgr )
      {
         SDB_OSS_DEL _pDCMgr ;
         _pDCMgr = NULL ;
      }

      return SDB_OK ;
   }

   void _catDCManager::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;

      /// ignore result
      _mapData2DCMgr( _pDCMgr ) ;
   }

   void _catDCManager::detachCB( pmdEDUCB * cb )
   {
      _pEduCB = NULL ;
   }

   INT32 _catDCManager::updateGlobalAddr()
   {
      // not primary
      if ( !pmdIsPrimary() )
      {
         return SDB_CLS_NOT_PRIMARY ;
      }
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      return catUpdateBaseInfoAddr( pmdGetOptionCB()->getCatAddr().c_str(), cb, 1 ) ;
   }

   BOOLEAN _catDCManager::isDCActivated() const
   {
      if ( _pDCBaseInfo )
      {
         return _pDCBaseInfo->isActivated() ;
      }
      return FALSE ;
   }

   BOOLEAN _catDCManager::isDCReadonly() const
   {
      if ( _pDCBaseInfo )
      {
         return _pDCBaseInfo->isReadonly() ;
      }
      return TRUE ;
   }

   INT32 _catDCManager::onBeginCommand ( MsgHeader *pMsg )
   {
      setWritedCommand( FALSE ) ;
      return SDB_OK ;
   }

   INT32 _catDCManager::onEndCommand( MsgHeader * pReqMsg,INT32 result )
   {
      return SDB_OK ;
   }

   INT32 _catDCManager::onSendReply ( MsgOpReply *pReply, INT32 result )
   {
      // Do nothing
      return SDB_OK ;
   }

   INT32 _catDCManager::active()
   {
      INT32 rc = SDB_OK;

      // update global info
      rc = _updateGlobalInfo() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update global info, rc: %d", rc ) ;

      // update dc base info
      rc = _mapData2DCMgr( _pDCMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to map dc base info, rc: %d", rc ) ;

      _isActived = TRUE ;

   done :
      return rc ;
   error :
      PD_LOG( PDSEVERE, "Stop program because of active dc manager failed, "
              "rc: %d", rc ) ;
      PMD_RESTART_DB( rc ) ;
      goto done ;
   }

   INT32 _catDCManager::deactive()
   {
      _isActived = FALSE ;
      return SDB_OK ;
   }

   INT32 _catDCManager::processMsg( const NET_HANDLE &handle,
                                    MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;

      switch ( pMsg->opCode )
      {
      // command message entry, should dispatch in the entry function
      case MSG_CAT_ALTER_IMAGE_REQ :
         rc = processCommandMsg( handle, pMsg, TRUE ) ;
         break ;

      default :
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG( PDWARNING, "Received unknown message (opCode: [%d]%u )",
                    IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode) ) ;
            break;
      }
      return rc ;
   }

   INT32 _catDCManager::processCommandMsg( const NET_HANDLE &handle,
                                           MsgHeader *pMsg,
                                           BOOLEAN writable )
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      MsgOpReply replyHeader ;
      rtnContextBuf ctxBuff ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;

      // init reply msg
      msgFillReplyByReq( replyHeader, pMsg ) ;

      // extract msg
      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay, writable ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG ( PDWARNING, "Service deactive but received command: %s, "
                     "opCode: %d, rc: %d", pCMDName,
                     pQueryReq->header.opCode, rc ) ;
            goto error ;
         }
      }

      // the second dispatch msg
      switch ( pQueryReq->header.opCode )
      {
         case MSG_CAT_ALTER_IMAGE_REQ :
            rc = processCmdAlterImage( handle, pQuery, ctxBuff ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Recieved unknow command: %s, opCode: %d",
                    pCMDName, pQueryReq->header.opCode ) ;
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Process command[%s] failed, opCode: %d, "
                   "rc: %d", pCMDName, pQueryReq->header.opCode, rc ) ;

   done:
      // send reply
      if ( !_pCatCB->isDelayed() )
      {
         if ( 0 == ctxBuff.size() )
         {
            rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
         }
         else
         {
            replyHeader.header.messageLength += ctxBuff.size() ;
            replyHeader.numReturned = ctxBuff.recordNum() ;
            rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                     (void *)ctxBuff.data(), ctxBuff.size() ) ;
         }
      }
      return rc ;
   error:
      replyHeader.flags = rc ;
      if( SDB_CLS_NOT_PRIMARY == rc )
      {
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done ;
   }

   INT32 _catDCManager::processCmdAlterImage( const NET_HANDLE &handle,
                                              const CHAR *pQuery,
                                              rtnContextBuf &ctxBuff )
   {
      INT32 rc = SDB_OK ;
      clsDCMgr dcMgr ;
      BSONObjBuilder retObjBuilder ;

      try
      {
         const CHAR *pAction = NULL ;
         BSONObj objQuery( pQuery ) ;
         BSONElement e = objQuery.getField( FIELD_NAME_ACTION ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "The field[%s] is not valid in command[%d]'s "
                    "param[%s]", FIELD_NAME_ACTION, MSG_CAT_ALTER_IMAGE_REQ,
                    objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pAction = e.valuestr() ;

         rc = _mapData2DCMgr( &dcMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Map dc base data to dc manager failed, "
                      "rc: %d", rc ) ;

         if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_ACTIVATE ) )
         {
            rc = processCmdActivate( handle, &dcMgr, objQuery,
                                     retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_DEACTIVATE ) )
         {
            rc = processCmdDeactivate( handle, &dcMgr, objQuery,
                                       retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_ENABLE_READONLY ) )
         {
            rc = processCmdEnableReadonly( handle, &dcMgr,
                                           objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_DISABLE_READONLY ) )
         {
            rc = processCmdDisableReadonly(  handle, &dcMgr,
                                             objQuery, retObjBuilder ) ;
         }
         else
         {
            PD_LOG( PDERROR, "The value[%s] of field[%s] is not valid "
                    "in command[%d]'s param[%s]", pAction, FIELD_NAME_ACTION,
                    MSG_CAT_ALTER_IMAGE_REQ, objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // need to update dc base info
         _mapData2DCMgr( _pDCMgr ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Parse command[%d]'s param occur exception: %s",
                 MSG_CAT_ALTER_IMAGE_REQ, e.what() ) ;
         goto error ;
      }

      if ( SDB_OK == rc )
      {
         BSONObj retObj = retObjBuilder.obj() ;
         if ( retObj.isEmpty() )
         {
            BSONObjBuilder tmpBuild ;
            vector< string > tmpGroup ;
            _pCatCB->makeGroupsObj( tmpBuild, tmpGroup ) ;
            retObj = tmpBuild.obj() ;
         }
         // get return groups
         ctxBuff = rtnContextBuf( retObj ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdActivate( const NET_HANDLE &handle,
                                            _clsDCMgr *pDCMgr,
                                            const BSONObj &objQuery,
                                            BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( !pBaseInfo->isActivated() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_ACTIVATED, TRUE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_ACTIVATED, FALSE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDeactivate( const NET_HANDLE &handle,
                                              _clsDCMgr *pDCMgr,
                                              const BSONObj &objQuery,
                                              BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( pBaseInfo->isActivated() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_ACTIVATED, FALSE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_ACTIVATED, TRUE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdEnableReadonly( const NET_HANDLE &handle,
                                                  _clsDCMgr *pDCMgr,
                                                  const BSONObj &objQuery,
                                                  BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( !pBaseInfo->isReadonly() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_READONLY, TRUE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_READONLY, FALSE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDisableReadonly( const NET_HANDLE &handle,
                                                   _clsDCMgr *pDCMgr,
                                                   const BSONObj &objQuery,
                                                   BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( pBaseInfo->isReadonly() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_READONLY, FALSE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_READONLY, TRUE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_mapData2DCMgr( _clsDCMgr *pDCMgr )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      BSONObj infoObj ;

      rc = pDCMgr->initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Init dc manager failed, rc: %d", rc ) ;

      // get data
      rc = catCheckBaseInfoExist( CAT_BASE_TYPE_GLOBAL_STR, exist,
                                  infoObj, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check dc base info exist failed, "
                   "rc: %d", rc ) ;

      if ( exist )
      {
         rc = pDCMgr->updateDCBaseInfo( infoObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Update dc base info failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT16 _catDCManager::_majoritySize()
   {
      return _pCatCB->majoritySize() ;
   }

   INT32 _catDCManager::_updateGlobalInfo()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      BSONObj infoObj ;
      pmdOptionsCB *option = pmdGetOptionCB() ;

      string clusterName ;
      string businessName ;
      option->getFieldStr( PMD_OPTION_CLUSTER_NAME, clusterName, "" ) ;
      option->getFieldStr( PMD_OPTION_BUSINESS_NAME, businessName, "" ) ;

      rc = catCheckBaseInfoExist( CAT_BASE_TYPE_GLOBAL_STR, exist,
                                  infoObj, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check dc base info exist failed, "
                   "rc: %d", rc ) ;

      if ( !exist )
      {
         // if the global info not exist, need to create
         infoObj = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR <<
                         FIELD_NAME_DATACENTER << BSON(
                           FIELD_NAME_CLUSTERNAME << clusterName <<
                           FIELD_NAME_BUSINESSNAME << businessName <<
                           FIELD_NAME_ADDRESS << option->getCatAddr() ) <<
                         FIELD_NAME_ACTIVATED << true <<
                         FIELD_NAME_READONLY << false ) ;
         rc = rtnInsert( CAT_SYSDCBASE_COLLECTION_NAME, infoObj, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Insert global info[%s] to collection[%s] "
                      "failed, rc: %d", infoObj.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }
      else
      {
         utilUpdateResult upResult ;
         string tmpClsName ;
         string tmpBusName ;
         clsDCBaseInfo dcBaseInfo ;

         // update dc base info
         rc = dcBaseInfo.updateFromBSON( infoObj, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse dc base info[%s] failed, rc: %d",
                      infoObj.toString().c_str() ) ;

         tmpClsName = dcBaseInfo.getClusterName() ;
         tmpBusName = dcBaseInfo.getBusinessName() ;

         if ( clusterName != tmpClsName || businessName != tmpBusName )
         {
            PD_LOG( PDEVENT, "Cluster name[%s] or business name[%s] has "
                    "changed to %s:%s", tmpClsName.c_str(), tmpBusName.c_str(),
                    clusterName.c_str(), businessName.c_str() ) ;
            BSONObj updator = BSON( "$set" << BSON(
              FIELD_NAME_DATACENTER"."FIELD_NAME_CLUSTERNAME << clusterName <<
              FIELD_NAME_DATACENTER"."FIELD_NAME_BUSINESSNAME << businessName )
                                   ) ;
            BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                    CAT_BASE_TYPE_GLOBAL_STR ) ;
            rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                            BSONObj(), 0, _pEduCB, _pDmsCB, _pDpsCB, 1,
                            &upResult ) ;
            PD_RC_CHECK( rc, PDERROR, "Update global info[%s] failed, rc: %d",
                         updator.toString().c_str(), rc ) ;
            if ( upResult.updateNum() <= 0 )
            {
               PD_LOG( PDERROR, "Not found global info, matcher: %s",
                       matcher.toString().c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}



