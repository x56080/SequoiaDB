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
#include "catLocation.hpp"

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

   INT32 _catDCManager::getCATVersion( UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      // update catalog cache
      rc = updateDCCache() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update DC cache, rc: %d", rc ) ;

      version = _pDCBaseInfo->getCATVersion() ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _catDCManager::setCATVersion( UINT32 version )
   {
      INT32 rc = SDB_OK ;

      BSONObj matcher, updator, dummy ;

      try
      {
         matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
         updator = BSON( "$set" <<
                         BSON( FIELD_NAME_CAT_VERSION << (INT32)version ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher and updator, "
                 "occur exception %s", e.what() ) ;
      }

      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      dummy, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection [%s], rc: %d",
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      // update catalog cache
      updateDCCache() ;

   done:
      return rc ;

   error:
      goto done ;
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
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      _fillRspHeader( &(replyHeader.header), &(pQueryReq->header) ) ;

      // extract msg
      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
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
            rc = processCmdDisableReadonly( handle, &dcMgr,
                                            objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_SET_ACTIVE_LOCATION ) )
         {
            rc = processCmdSetActiveLocation( handle, &dcMgr,
                                              objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_SET_LOCATION ) )
         {
            rc = processCmdSetLocation( handle, &dcMgr,
                                        objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_START_MAINTENANCE_MODE ) )
         {
            rc = processCmdAlterMaintenanceMode( handle, &dcMgr,
                                                 objQuery, retObjBuilder, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_STOP_MAINTENANCE_MODE ) )
         {
            rc = processCmdAlterMaintenanceMode( handle, &dcMgr,
                                                 objQuery, retObjBuilder, FALSE ) ;
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

      // force to secondary to reelect, then the primary node can resume
      // active works did not finish in read-only mode
      _pCatCB->setNeedForceSecondary( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdSetActiveLocation( const NET_HANDLE &handle,
                                                     _clsDCMgr *pDCMgr,
                                                     const BSONObj &objQuery,
                                                     BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      ossPoolString newActLoc ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get new ActiveLocation, this field should not be empty
         ele = options.getField( CAT_ACTIVE_LOCATION_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
            goto error ;
         }
         // ele.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < ele.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         newActLoc = ele.valuestrsafe() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString oldActLoc ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            // Check and get active location
            rc = catCheckAndGetActiveLocation( groupObj, groupID, newActLoc, oldActLoc ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get and check active location, rc: %d", rc ) ;
               continue ;
            }

            // Compare oldLocation and newLocation
            if ( oldActLoc == newActLoc )
            {
               PD_LOG( PDDEBUG, "The old and new ActiveLocation are same, do nothing" ) ;
               continue ;
            }

            // Set new ActiveLocation
            if ( ! newActLoc.empty() )
            {
               rc = pCatNodeMgr->setActiveLocation( groupID, newActLoc ) ;
            }
            // Remove old ActiveLocation
            else
            {
               rc = pCatNodeMgr->removeActiveLocation( groupID ) ;
            }
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to set active location, rc: %d", rc ) ;
               continue ;
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdSetLocation( const NET_HANDLE & handle,
                                               _clsDCMgr * pDCMgr,
                                               const BSONObj & objQuery,
                                               BSONObjBuilder & retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      ossPoolString newLoc ;
      ossPoolString hostName ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get new Location, this field should not be empty
         ele = options.getField( CAT_LOCATION_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_LOCATION_NAME ) ;
            goto error ;
         }
         // ele.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < ele.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         newLoc = ele.valuestrsafe() ;

         ele = options.getField( CAT_HOST_FIELD_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_HOST_FIELD_NAME ) ;
            goto error ;
         }
         hostName = ele.valuestrsafe() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;
         allGroups.push_back( COORD_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString oldActLoc ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            rc = pCatNodeMgr->setGroupLocation( groupObj, groupID, newLoc, hostName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to set group location, rc: %d", rc  ) ;
               continue ;
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdAlterMaintenanceMode( const NET_HANDLE &handle,
                                                        _clsDCMgr *pDCMgr,
                                                        const BSONObj &objQuery,
                                                        BSONObjBuilder &retObjBuilder,
                                                        const BOOLEAN &isStartMode )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString hostName ;
            clsGrpModeItem item ;
            clsGroupMode groupMode ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Check group mode info
            rc = _checkMaintenanceMode( options, groupObj, groupID,
                                        isStartMode, groupMode, &hostName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to check group[%u] mode info, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            if ( isStartMode )
            {
               rc = pCatNodeMgr->startGrpMode( groupMode, groupName, groupObj ) ;
               if ( SDB_OK != rc )
               {
                  failedGroups.push_back( groupID ) ;
                  PD_LOG( PDERROR, "Failed to start maintenance mode on group[%u], rc: %d", groupID, rc ) ;
                  continue ;
               }
            }
            else
            {
               rc = pCatNodeMgr->stopGrpMode( groupMode ) ; 
               if ( SDB_OK != rc )
               {
                  failedGroups.push_back( groupID ) ;
                  PD_LOG( PDERROR, "Failed to stop maintenance mode on group[%u], rc: %d", groupID, rc ) ;
                  continue ;
               }
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _catDCManager::_fillRspHeader( MsgHeader * rspMsg,
                                       const MsgHeader * reqMsg )
   {
      rspMsg->opCode = MAKE_REPLY_TYPE( reqMsg->opCode ) ;
      rspMsg->requestID = reqMsg->requestID ;
      rspMsg->routeID.value = 0 ;
      rspMsg->TID = reqMsg->TID ;
      rspMsg->globalID = reqMsg->globalID ;
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
                         FIELD_NAME_READONLY << false <<
                         FIELD_NAME_CSUNIQUEHWM << 0 <<
                         FIELD_NAME_TASKHWM << 0 <<
                         FIELD_NAME_CAT_VERSION << CATALOG_VERSION_CUR <<
                         FIELD_NAME_RECYCLEBIN <<
                         BSON( FIELD_NAME_ENABLE <<
                                     (bool)( UTIL_RECYCLEBIN_DFT_ENABLE ) <<
                               FIELD_NAME_RECYCLEIDHWM << (INT64)0 <<
                               FIELD_NAME_EXPIRETIME <<
                                     UTIL_RECYCLEBIN_DFT_EXPIRETIME <<
                               FIELD_NAME_MAXITEMNUM <<
                                     UTIL_RECYCLEBIN_DFT_MAXITEMNUM <<
                               FIELD_NAME_MAXVERNUM <<
                                     UTIL_RECYCLEBIN_DFT_MAXVERNUM <<
                               FIELD_NAME_AUTODROP <<
                                     (bool)( UTIL_RECYCLEBIN_DFT_AUTODROP ) ) ) ;
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

         if ( !dcBaseInfo.isReadonly() )
         {
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

            // add recycle bin if not exists
            if ( !infoObj.hasField( FIELD_NAME_RECYCLEBIN ) )
            {
               BSONObj updator =
                     BSON( "$set" <<
                           BSON( FIELD_NAME_RECYCLEBIN <<
                                 BSON( FIELD_NAME_ENABLE <<
                                          (bool)( UTIL_RECYCLEBIN_DFT_ENABLE ) <<
                                       FIELD_NAME_RECYCLEIDHWM << (INT64)0 <<
                                       FIELD_NAME_EXPIRETIME <<
                                             UTIL_RECYCLEBIN_DFT_EXPIRETIME <<
                                       FIELD_NAME_MAXITEMNUM <<
                                             UTIL_RECYCLEBIN_DFT_MAXITEMNUM <<
                                       FIELD_NAME_MAXVERNUM <<
                                             UTIL_RECYCLEBIN_DFT_MAXVERNUM <<
                                       FIELD_NAME_AUTODROP <<
                                          (bool)( UTIL_RECYCLEBIN_DFT_AUTODROP ) ) ) ) ;

               BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                       CAT_BASE_TYPE_GLOBAL_STR ) ;
               rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                               BSONObj(), 0, _pEduCB, _pDmsCB, _pDpsCB, 1,
                               &upResult ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to update recycle info [%s], "
                            "rc: %d", updator.toString().c_str(), rc ) ;
               PD_CHECK( upResult.updateNum() > 0, SDB_SYS, error, PDERROR,
                         "Not found global info, matcher: %s",
                         matcher.toString().c_str() ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_checkMaintenanceMode( const BSONObj &option,
                                               const BSONObj &groupObj,
                                               const UINT32 &groupID,
                                               const BOOLEAN &isStartMode,
                                               clsGroupMode &groupMode,
                                               ossPoolString *pHostName )
   {
      INT32 rc = SDB_OK ;

      BSONElement grpModeEle, primaryEle ;

      // Init groupMode
      groupMode.groupID = groupID ;
      groupMode.mode = CLS_GROUP_MODE_MAINTENANCE ;

      // Check if this group is in critical mode
      grpModeEle = groupObj.getField( CAT_GROUP_MODE_NAME ) ;
      if ( ! grpModeEle.eoo() )
      {
         if ( String != grpModeEle.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDWARNING, "Failed to get the field[%s], type[%d] is not String",
                    CAT_GROUP_MODE_NAME, grpModeEle.type() ) ;
            goto error ;
         }
         else if ( 0 == ossStrcmp( CAT_CRITICAL_MODE_NAME, grpModeEle.valuestrsafe() ) )
         {
            rc = SDB_OPERATION_CONFLICT ;
            PD_LOG_MSG( PDERROR, "Failed to %s maintenance mode in group[%u], "
                        "critical mode is operating", isStartMode ? "start" : "stop", groupID ) ;
            goto error ;
         }
      }
      // If the command is stop maintenance mode, do nothing
      else if ( ! isStartMode )
      {
         goto done ;
      }

      // If the command is stop maintenance mode and options is empty, it means stop all maintenance mode
      if ( ! isStartMode && option.isEmpty() )
      {
         goto done ;
      }

      rc = catParseGroupModeInfo( option, groupObj, groupID, isStartMode, groupMode, pHostName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group mode info[%s], rc: %d",
                   option.toPoolString().c_str(), rc ) ;

      rc = _buildGroupModeInfo( groupObj, *pHostName, groupMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build group[%u] mode info, rc: %d", groupID, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_buildGroupModeInfo( const BSONObj &groupObj,
                                             const ossPoolString &hostName,
                                             clsGroupMode &groupMode )
   {
      INT32 rc = SDB_OK ;

      clsGrpModeItem item = groupMode.grpModeInfo[0] ;
      groupMode.grpModeInfo.clear() ;

      BSONObjIterator nodeItr ;
      BSONObj nodeListObj ;
      ossPoolString tmpHostName ;
      ossPoolString tmpSvcName ;

      rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, nodeListObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d", CAT_GROUP_NAME, rc ) ;

      // Parse nodeList array
      nodeItr = BSONObjIterator( nodeListObj ) ;
      while ( nodeItr.more() )
      {
         clsGrpModeItem tmpItem ;
         BSONObj boNode = nodeItr.next().embeddedObject() ;

         if ( ! item.location.empty() )
         {
            BSONElement beLocation = boNode.getField( FIELD_NAME_LOCATION ) ;
            if ( beLocation.eoo() )
            {
               continue ;
            }
            else if ( String != beLocation.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_LOCATION, beLocation.type() ) ;
               goto error ;
            }
            else if ( 0 != ossStrcmp( beLocation.valuestrsafe(), item.location.c_str() ) )
            {
               continue ;
            }

         }
         else if ( ! hostName.empty() )
         {
            BSONElement beHostName = boNode.getField( FIELD_NAME_HOST ) ;
            if ( beHostName.eoo() )
            {
               continue ;
            }
            else if ( String != beHostName.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_HOST, beHostName.type() ) ;
               goto error ;
            }
            else if ( 0 != ossStrcmp( beHostName.valuestrsafe(), hostName.c_str() ) )
            {
               continue ;
            }
         }

         BSONElement beHost = boNode.getField( FIELD_NAME_HOST ) ;
         if ( beHost.eoo() || String != beHost.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                     FIELD_NAME_HOST, beHost.type() ) ;
            goto error ;
         }
         tmpHostName = beHost.valuestrsafe() ;

         BSONElement beService = boNode.getField( FIELD_NAME_SERVICE ) ;
         if ( beService.eoo() || Array != beService.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                     FIELD_NAME_SERVICE, beService.type() ) ;
            goto error ;
         }
         tmpSvcName = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE ) ;

         BSONElement beNodeID = boNode.getField( FIELD_NAME_NODEID ) ;
         if ( beNodeID.eoo() || ! beNodeID.isNumber() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                     FIELD_NAME_NODEID, beService.type() ) ;
            goto error ;
         }
         tmpItem.nodeID = beNodeID.numberInt() ;

         tmpItem.nodeName = tmpHostName + ":" + tmpSvcName ;
         tmpItem.minKeepTime = item.minKeepTime ;
         tmpItem.maxKeepTime = item.maxKeepTime ;
         tmpItem.updateTime = item.updateTime ;

         groupMode.grpModeInfo.push_back( tmpItem ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}
