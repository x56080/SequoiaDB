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

   Source File Name = rtnCoordOpenLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2014  YW  Initial Draft
   Last Changed =

*******************************************************************************/
#include "rtnCoordLob.hpp"
#include "rtnLob.hpp"
#include "rtnTrace.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"
#include "rtnCoordLobStream.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDOPENLOB_EXECUTE, "rtnCoordOpenLob::execute" )
   INT32 rtnCoordOpenLob::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDOPENLOB_EXECUTE ) ;
      SDB_ASSERT( NULL != buf, "can not be null" ) ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      contextID = -1 ;

      rc = msgExtractOpenLobRequest( (const CHAR*)pMsg, &header, obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract open msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      rc = rtnOpenLob( obj, header->flags, FALSE, cb,
                       NULL, 0, contextID, *buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob:%s, rc:%d",
                 obj.toString( FALSE, TRUE ).c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDOPENLOB_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDWRITELOB_EXECUTE, "rtnCoordWriteLob::execute" )
   INT32 rtnCoordWriteLob::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDWRITELOB_EXECUTE ) ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      UINT32 len = 0 ;
      SINT64 offset = -1 ;
      const CHAR *data = NULL ;
      contextID = -1 ;

      rc = msgExtractWriteLobRequest( (const CHAR*)pMsg, &header, &len,
                                      &offset, &data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, len, offset ) ;

      rc = rtnWriteLob( header->contextID, cb, len, data, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDWRITELOB_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDREADLOB_EXECUTE, "rtnCoordReadLob::execute" )
   INT32 rtnCoordReadLob::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDREADLOB_EXECUTE ) ;
      SDB_ASSERT( NULL != buf, "can not be null" ) ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      UINT32 len = 0 ;
      SINT64 offset = -1 ;
      const CHAR *data = NULL ;
      UINT32 readLen = 0 ;
      contextID = -1 ;

      rc = msgExtractReadLobRequest( (const CHAR*)pMsg, &header, &len,
                                     &offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, readLen, offset ) ;

      rc = rtnReadLob( header->contextID, cb, len,
                       offset, &data, readLen, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;   
      }

      *buf = rtnContextBuf( data, readLen, 1 ) ;
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDREADLOB_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDCLOSELOB_EXECUTE, "rtnCoordCloseLob::execute" )
   INT32 rtnCoordCloseLob::execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDCLOSELOB_EXECUTE ) ;
      const MsgOpLob *header = NULL ;
      contextID = -1 ;

      rc = msgExtractCloseLobRequest( (const CHAR*)pMsg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      } 

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnCloseLob( header->contextID, cb, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDCLOSELOB_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOORDREMOVELOB_EXECUTE, "rtnCoordRemoveLob::execute" )
   INT32 rtnCoordRemoveLob::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDREMOVELOB_EXECUTE ) ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      BSONElement ele ;
      const CHAR *fullName = NULL ;
      _rtnCoordLobStream stream ;
      contextID = -1 ;

      rc = msgExtractRemoveLobRequest( (const CHAR*)pMsg, &header,
                                       obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract remove msg:%d", rc ) ;
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"collection\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      fullName = ele.valuestr() ;

      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"oid\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      rc = stream.open( fullName,
                        ele.__oid(), SDB_LOB_MODE_REMOVE,
                        header->flags,
                        NULL,
                        cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove lob:%s, rc:%d",
                 ele.__oid().str().c_str(), rc ) ;
         goto error ;
      }
      else
      {
         /// do nothing.
      }

      rc = stream.truncate( 0, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "faield to truncate lob:%d", rc ) ;
         /// get error info
         stream.getErrorInfo( rc, cb, buf ) ;
         goto error ;
      }

   done:
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = stream.close( cb ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to remove lob:%d", rcTmp ) ;
            rc = rc == SDB_OK ? rcTmp : rc ;
         }
      }
      PD_TRACE_EXITRC( SDB_RTNCOORDREMOVELOB_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

