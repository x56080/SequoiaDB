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

   Source File Name = sptRemote.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptRemote.hpp"
#include "client.h"
#include "client_internal.h"
#include "pd.hpp"
#include "msgDef.h"
#include "ossUtil.h"
#include "ossTypes.h"
#include "omagentDef.hpp"
#include "common.h"
#include "sptCommon.hpp"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

#define SDB_CLIENT_DFT_NETWORK_TIMEOUT 10000

namespace engine
{

/*
 define macro
*/
#define HANDLE_CHECK( handle, interhandle, handletype ) \
do                                                      \
{                                                       \
   if ( SDB_INVALID_HANDLE == handle )                  \
   {                                                    \
      rc = SDB_INVALIDARG ;                             \
      goto error ;                                      \
   }                                                    \
   if ( !interhandle ||                                 \
        handletype != interhandle->_handleType )        \
   {                                                    \
      rc = SDB_CLT_INVALID_HANDLE ;                     \
      goto error ;                                      \
   }                                                    \
}while( FALSE )


#define CHECK_RET_MSGHEADER( pSendBuf, pRecvBuf, connHandle )               \
do                                                                          \
{                                                                           \
   sdbConnectionStruct *db = (sdbConnectionStruct*)connHandle ;             \
   HANDLE_CHECK( connHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;             \
   rc = clientCheckRetMsgHeader( pSendBuf, pRecvBuf, db->_endianConvert ) ; \
   if ( SDB_OK != rc )                                                      \
   {                                                                        \
      if ( SDB_UNEXPECTED_RESULT == rc )                                    \
      {                                                                     \
         sdbDisconnect( connHandle ) ;                                      \
      }                                                                     \
      goto error ;                                                          \
   }                                                                        \
}while( FALSE )

   /*
      Local function define
   */

   static INT32 _extractErrorObj( const CHAR *pErrorBuf,
                                  INT32 *pFlag,
                                  const CHAR **ppErr,
                                  const CHAR **ppDetail )
   {
      INT32 rc = SDB_OK ;
      bson localobj ;
      bson_iterator it ;

      bson_init( &localobj ) ;

      if ( !pErrorBuf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = bson_init_finished_data( &localobj, pErrorBuf ) ;
      if ( rc )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      if ( pFlag && BSON_INT == bson_find( &it, &localobj, OP_ERRNOFIELD ) )
      {
         *pFlag = bson_iterator_int( &it ) ;
      }
      if ( ppErr && BSON_STRING == bson_find( &it, &localobj,
                                              OP_ERRDESP_FIELD ) )
      {
         *ppErr = bson_iterator_string( &it ) ;
      }
      if ( ppDetail && BSON_STRING == bson_find( &it, &localobj,
                                                 OP_ERR_DETAIL ) )
      {
         *ppDetail = bson_iterator_string( &it ) ;
      }

   done:
      bson_destroy( &localobj ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      define member functions
   */
   _sptRemote::_sptRemote()
   {
   }

   _sptRemote::~_sptRemote()
   {
   }

   INT32 _sptRemote::runCommand( ossValuePtr handle,
                                 const CHAR *pString,
                                 SINT32 flag,
                                 UINT64 reqID,
                                 SINT64 numToSkip,
                                 SINT64 numToReturn,
                                 const CHAR *arg1,
                                 const CHAR *arg2,
                                 const CHAR *arg3,
                                 const CHAR *arg4,
                                 CHAR **ppRetBuffer,
                                 INT32 &retCode,
                                 BOOLEAN needRecv )
   {
      SDB_ASSERT( handle, "handle can't be 0" ) ;
      SDB_ASSERT( pString, "pString can't be null" ) ;
      INT32 rc          = SDB_OK ;
      BOOLEAN extracted = FALSE ;
      SINT64 contextID  = 0 ;
      sdbConnectionStruct *connection = 0;
      retCode = SDB_OK ;

      if( 0 == handle )
      {
         rc = SDB_NETWORK ;
         PD_LOG( PDERROR, "network is closed, rc = %d", rc ) ;
         goto error ;
      }
      connection = (sdbConnectionStruct *) handle ;

      // build message
      rc = clientBuildQueryMsgCpp ( &( connection->_pSendBuffer ),
                                    &( connection->_sendBufferSize ),
                                    pString, flag, reqID,
                                    numToSkip, numToReturn,
                                    arg1, arg2, arg3, arg4,
                                    connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build query msg, rc = %d", rc ) ;

      // send and recv msg
      rc = sendAndRecv( handle, connection->_sock,
                        ( const MsgHeader* )connection->_pSendBuffer,
                        ( MsgHeader** )&( connection->_pReceiveBuffer ),
                        &( connection->_receiveBufferSize),
                        needRecv, connection->_endianConvert ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build send and recv msg, rc = %d", rc ) ;

      if ( needRecv )
      {
         // extract message
         rc = _extract( (MsgHeader *)connection->_pReceiveBuffer,
                        connection->_receiveBufferSize,
                        &contextID, extracted,
                        connection->_endianConvert ) ;

         if ( SDB_OK != rc )
         {
            if ( !extracted )
            {
               PD_LOG( PDERROR, "Failed to extract msg in client, rc = %d",
                       rc ) ;
               goto error ;
            }
            else
            {
               retCode = rc ;
               rc = SDB_OK ;
               PD_LOG( PDINFO, "Failed to run command in engine, rc = %d",
                       retCode ) ;
            }
         }

         // check whether the return message is what we want or not
         CHECK_RET_MSGHEADER( connection->_pSendBuffer,
                              connection->_pReceiveBuffer,
                              handle ) ;

         // try to get retObj
         rc = _getRetBuffer( connection->_pReceiveBuffer, ppRetBuffer ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get retObjArray, rc = %d", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_extract( MsgHeader * msg, INT32 msgSize,
                               SINT64 * contextID,
                               BOOLEAN &extracted,
                               BOOLEAN endianConvert )
   {
      INT32 rc = SDB_OK ;
      INT32 replyFlag = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom = -1 ;
      CHAR *pBuffer = ( CHAR* )msg ;

      extracted = FALSE ;
      rc = clientExtractReply ( pBuffer, &replyFlag, contextID,
                                &startFrom, &numReturned, endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = replyFlag ;
      extracted = TRUE ;

      if ( SDB_OK != replyFlag && SDB_DMS_EOC != replyFlag )
      {
         INT32 dataOff     = 0 ;
         INT32 dataSize    = 0 ;
         const CHAR *pErr  = NULL ;
         const CHAR *pDetail = NULL ;
         const CHAR *pErrorBuf = NULL ;

         dataOff = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
         dataSize = msg->messageLength - dataOff ;
         /// save error info
         if ( dataSize > 0 )
         {
            pErrorBuf = ( const CHAR* )msg + dataOff ;
            if ( SDB_OK == _extractErrorObj( pErrorBuf,
                                             NULL, &pErr, &pDetail ) )
            {
               sdbErrorCallback( pErrorBuf, (UINT32)dataSize,
                                 replyFlag, pErr, pDetail ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptRemote::_getRetBuffer( CHAR *pRetMsg, CHAR **ppRetBuffer )
   {
      INT32 rc     = SDB_OK ;
      INT32 offset = ossRoundUpToMultipleX( sizeof( MsgOpReply ), 4 ) ;

      MsgOpReply * msgReply = (MsgOpReply*)pRetMsg ;

      if ( NULL == ppRetBuffer )
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get ppRetBuffer, rc: %d", rc) ;

      // init retBuffer
      if ( offset < msgReply->header.messageLength )
      {
         *ppRetBuffer = &pRetMsg[offset] ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to init ppRetBuffer, rc: %d", rc) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

