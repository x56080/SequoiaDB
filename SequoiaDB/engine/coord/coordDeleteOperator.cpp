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

   Source File Name = coordDeleteOperator.cpp

   Descriptive Name = Coord Delete

   When/how to use: this program may be used on binary and text-formatted
   version of runtime component. This file contains code logic for
   data delete request from coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#include "coordDeleteOperator.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "rtnCommandDef.hpp"
#include "coordUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordDeleteOperator implement
   */
   _coordDeleteOperator::_coordDeleteOperator()
   {
      const static string s_name( "Delete" ) ;
      setName( s_name ) ;
   }

   _coordDeleteOperator::~_coordDeleteOperator()
   {
      SDB_ASSERT( 0 == _vecBlock.size(), "Block must be empty" ) ;
   }

   BOOLEAN _coordDeleteOperator::isReadOnly() const
   {
      return FALSE ;
   }

   UINT32 _coordDeleteOperator::getDeletedNum() const
   {
      return _recvNum ;
   }

   void _coordDeleteOperator::clearStat()
   {
      _recvNum = 0 ;
   }
 
   //PD_TRACE_DECLARE_FUNCTION ( COORD_OPERATORDEL_EXE, "_coordDeleteOperator::execute" )
   INT32 _coordDeleteOperator::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_OPERATORDEL_EXE ) ;

      // process define
      coordSendOptions sendOpt( TRUE ) ;
      coordSendMsgIn inMsg( pMsg ) ;
      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      coordCataSel cataSel ;
      MsgRouteID errNodeID ;

      BSONObj boDeletor ;

      // fill default-reply(delete success)
      MsgOpDelete *pDelMsg             = (MsgOpDelete *)pMsg ;
      INT32 oldFlag                    = pDelMsg->flags ;
      pDelMsg->flags                  |= FLG_DELETE_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0;
      CHAR *pCollectionName = NULL ;
      CHAR *pDeletor = NULL ;
      CHAR *pHint = NULL ;
      rc = msgExtractDelete( (CHAR*)pMsg, &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;
      if( rc )
      {
         PD_LOG( PDERROR,"Failed to parse delete request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      if ( 0 == ossStrncmp( pCollectionName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         rc = SDB_COORD_UNKNOWN_OP_REQ ;
         goto error ;
      }

      try
      {
         boDeletor = BSONObj( pDeletor ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Delete failed, received unexpected error: %s",
                      e.what() ) ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Collection:%s, Deletor:%s, Hint:%s, "
                          "Flag:0x%08x(%u)",
                          pCollectionName,
                          boDeletor.toString().c_str(),
                          BSONObj(pHint).toString().c_str(),
                          oldFlag, oldFlag ) ;

      rc = cataSel.bind( _pResource, pCollectionName, cb, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pCollectionName, rc ) ;
         goto error ;
      }

   retry:
      pDelMsg->version = cataSel.getCataPtr()->getVersion() ;
      pDelMsg->w = 0 ;
      rcTmp = doOpOnCL( cataSel, boDeletor, inMsg, sendOpt, cb, result ) ;

      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         goto done ;
      }
      else if ( checkRetryForCLOpr( rcTmp, &nokRC, cataSel, inMsg.msg(),
                                    cb, rc, &errNodeID, TRUE ) )
      {
         nokRC.clear() ;
         _groupSession.getGroupCtrl()->incRetry() ;
         goto retry ;
      }
      else
      {
         PD_LOG( PDERROR, "Delete failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( oldFlag & FLG_DELETE_RETURNNUM )
      {
         contextID = _recvNum ;
      }
      if ( pCollectionName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_DELETE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "DeletedNum:%u, Deletor:%s, Hint:%s, Flag:0x%08x(%u)",
                      _recvNum, boDeletor.toString().c_str(),
                      BSONObj(pHint).toString().c_str(), oldFlag, oldFlag ) ;
      }
      PD_TRACE_EXITRC ( COORD_OPERATORDEL_EXE, rc ) ;
      return rc ;
   error:
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                   cb, &nokRC ) ) ;
      }
      goto done ;
   }

   void _coordDeleteOperator::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_DELETE_REQ ;
   }

   void _coordDeleteOperator::_clearBlock( pmdEDUCB *cb )
   {
      for ( INT32 i = 0 ; i < (INT32)_vecBlock.size() ; ++i )
      {
         cb->releaseBuff( _vecBlock[ i ] ) ;
      }
      _vecBlock.clear() ;
   }

   INT32 _coordDeleteOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                 coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 pmdEDUCB *cb,
                                                 coordProcessResult &result )
   {
      INT32 rc                = SDB_OK ;
      MsgOpDelete *pDelMsg    = ( MsgOpDelete* )inMsg.msg() ;

      INT32 flag              = 0 ;
      CHAR *pCollectionName   = NULL;
      CHAR *pDeletor          = NULL;
      CHAR *pHint             = NULL;

      CoordGroupSubCLMap &grpSubCl = cataSel.getGroup2SubsMap() ;
      CoordGroupSubCLMap::iterator it ;

      inMsg.data()->clear() ;

      rc = msgExtractDelete( (CHAR*)inMsg.msg(), &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse delete request, rc: %d",
                   rc ) ;

      try
      {
         BSONObj boDeletor( pDeletor ) ;
         BSONObj boHint( pHint ) ;
         BSONObj boNew ;

         CHAR *pBuff             = NULL ;
         UINT32 buffLen          = 0 ;
         UINT32 buffPos          = 0 ;

         it = grpSubCl.begin() ;
         while( it != grpSubCl.end() )
         {
            CoordSubCLlist &subCLLst = it->second ;

            netIOVec &iovec = inMsg._datas[ it->first ] ;
            netIOV ioItem ;

            // 1. first vec
            ioItem.iovBase = (const CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
            ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpDelete, name) +
                                                    pDelMsg->nameLength + 1, 4 ) -
                            sizeof( MsgHeader ) ;
            iovec.push_back( ioItem ) ;

            // 2. new deletor vec
            boNew = _buildNewDeletor( boDeletor, subCLLst ) ;
            // 2.1 add to buff
            UINT32 roundLen = ossRoundUpToMultipleX( boNew.objsize(), 4 ) ;
            if ( buffPos + roundLen > buffLen )
            {
               UINT32 alignLen = ossRoundUpToMultipleX( roundLen,
                                                        DMS_PAGE_SIZE4K ) ;
               rc = cb->allocBuff( alignLen, &pBuff, &buffLen ) ;
               PD_RC_CHECK( rc, PDERROR, "Alloc buff[%u] failed, rc: %d",
                            alignLen, rc ) ;
               _vecBlock.push_back( pBuff ) ;
               buffPos = 0 ;
            }
            ossMemcpy( &pBuff[ buffPos ], boNew.objdata(), boNew.objsize() ) ;
            ioItem.iovBase = &pBuff[ buffPos ] ;
            ioItem.iovLen = roundLen ;
            buffPos += roundLen ;
            iovec.push_back( ioItem ) ;

            // 3. hinter vec
            ioItem.iovBase = boHint.objdata() ;
            ioItem.iovLen = boHint.objsize() ;
            iovec.push_back( ioItem ) ;         

            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse delete message occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _clearBlock( cb ) ;
      goto done ;
   }

   void _coordDeleteOperator::_doneMainCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      _clearBlock( cb ) ;
      inMsg._datas.clear() ;
   }

   BSONObj _coordDeleteOperator::_buildNewDeletor( const BSONObj &deletor,
                                                   const CoordSubCLlist &subCLList )
   {
      BSONObjBuilder builder( deletor.objsize() +
                              subCLList.size() * COORD_SUBCL_NAME_DFT_LEN ) ;

      builder.appendElements( deletor ) ;

      /*
         Append array, as { SubCLName : [ "a.a", "b.a" ] }
      */
      BSONArrayBuilder babSubCL( builder.subarrayStart( CAT_SUBCL_NAME ) ) ;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin();
      while( iterCL != subCLList.end() )
      {
         babSubCL.append( *iterCL ) ;
         ++iterCL ;
      }
      babSubCL.done() ;

      return builder.obj() ;
   }

}

