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

   Source File Name = impRecordImporter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordImporter.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include "msgDef.h"
#include "../client/client_internal.h"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

namespace import
{
   #define IMP_MAX_RECORDS_SIZE (SDB_MAX_MSG_LENGTH - 1024 * 1024 * 1)
   #define IMP_DEFAULT_NETWORK_TIMEOUT (-1)

   static INT32 defaultVersion = 1 ;
   static INT16 defaultW = 0 ;

   RecordImporter::RecordImporter( const string& hostname,
                                   const string& svcname,
                                   const string& user,
                                   const string& password,
                                   const string& csname,
                                   const string& clname,
                                   BOOLEAN useSSL,
                                   BOOLEAN enableTransaction,
                                   BOOLEAN allowKeyDuplication )
         : _insertBufferSize( 0 ),
           _recvBufferSize( 0 ),
           _useSSL( useSSL ),
           _enableTransaction( enableTransaction ),
           _allowKeyDuplication( allowKeyDuplication ),
           _endianConvert( FALSE ),
           _connection( SDB_INVALID_HANDLE ),
           _collectionSpace( SDB_INVALID_HANDLE ),
           _collection( SDB_INVALID_HANDLE ),
           _insertMsg( NULL ),
           _insertBuffer( NULL ),
           _recvBuffer( NULL ),
           _hostname( hostname ),
           _svcname( svcname ),
           _user( user ),
           _password( password ),
           _csname( csname ),
           _clname( clname )
   {

   }

   RecordImporter::~RecordImporter()
   {
      disconnect() ;

      SAFE_OSS_FREE( _insertBuffer ) ;
      SAFE_OSS_FREE( _recvBuffer ) ;
   }

   INT32 RecordImporter::connect()
   {
      INT32 rc = SDB_OK ;
      sdbConnectionStruct *connection = NULL ;

      SDB_ASSERT( SDB_INVALID_HANDLE == _connection, "already connected" ) ;
      SDB_ASSERT( SDB_INVALID_HANDLE == _collectionSpace,
                  "already get collection space" ) ;
      SDB_ASSERT( SDB_INVALID_HANDLE == _collection, "already get collection");

      if ( _useSSL )
      {
         rc = sdbSecureConnect( _hostname.c_str(), _svcname.c_str(),
                                _user.c_str(), _password.c_str(),
                                &_connection ) ;
      }
      else
      {
         rc = sdbConnect( _hostname.c_str(), _svcname.c_str(),
                          _user.c_str(), _password.c_str(),
                          &_connection ) ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to connect to database %s:%s, "
                          "rc = %d, usessl=%d",
                 _hostname.c_str(), _svcname.c_str(), rc, _useSSL ) ;
         goto error ;
      }

      connection = (sdbConnectionStruct*)_connection ;
      _endianConvert = connection->_endianConvert ;

      rc = sdbGetCollectionSpace( _connection, _csname.c_str(),
                                  &_collectionSpace ) ;
      if ( rc )
      {
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            ossPrintf( "collection space %s does not exist"OSS_NEWLINE,
                       _csname.c_str() ) ;
            PD_LOG( PDERROR, "collection space %s does not exist, rc = %d",
                    _csname.c_str(), rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to get collection space %s, rc = %d",
                    _csname.c_str(), rc ) ;
         }

         goto error ;
      }

      rc = sdbGetCollection1( _collectionSpace, _clname.c_str(),
                              &_collection ) ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc )
         {
            ossPrintf( "collection %s does not exist."OSS_NEWLINE,
                       _clname.c_str() ) ;
            PD_LOG( PDERROR, "collection %s does not exist, rc = %d",
                    _clname.c_str(), rc ) ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "failed to get collection %s, rc = %d",
                    _clname.c_str(), rc ) ;
         }

         goto error ;
      }

      rc = _initInsertMsg() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to init insert message, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void RecordImporter::disconnect()
   {
      if ( SDB_INVALID_HANDLE != _collection )
      {
         sdbReleaseCollection( _collection ) ;
         _collection = SDB_INVALID_HANDLE ;
      }

      if ( SDB_INVALID_HANDLE != _collectionSpace )
      {
         sdbReleaseCS( _collectionSpace ) ;
         _collectionSpace = SDB_INVALID_HANDLE ;
      }

      if ( SDB_INVALID_HANDLE != _connection )
      {
         sdbDisconnect( _connection ) ;
         sdbReleaseConnection( _connection ) ;
         _connection = SDB_INVALID_HANDLE ;
      }
   }

   INT32 RecordImporter::import( PageInfo* pageInfo )
   {
      INT32 rc = SDB_OK;
      INT32 flag = 0;

      SDB_ASSERT( NULL != pageInfo, "pageInfo can't be NULL" ) ;

      if ( _enableTransaction )
      {
         rc = sdbTransactionBegin( _connection ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to begin transaction, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( _allowKeyDuplication )
      {
         flag |= FLG_INSERT_CONTONDUP ;
      }

      rc = _bulkInsert( pageInfo, flag ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to bulk insert, rc=%d", rc ) ;
         // the transaction is rollbacked automatically
         goto error;
      }

      if ( _enableTransaction )
      {
         rc = sdbTransactionCommit( _connection ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to commit transaction, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_initInsertMsg()
   {
      INT32 rc = SDB_OK ;
      INT32 nameLength = 0 ;
      sdbCollectionStruct *cls = NULL ;
      const CHAR *clName = NULL ;

      cls = (sdbCollectionStruct*)_collection ;
      clName = cls->_collectionFullName ;

      nameLength = ossStrlen( clName ) ;

      _insertBufferSize = ossRoundUpToMultipleX(
            offsetof( MsgOpInsert, name ) + nameLength + 1, 4 ) ;

      _insertBuffer = (CHAR*)SDB_OSS_MALLOC( _insertBufferSize ) ;
      if ( NULL == _insertBuffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "failed to malloc buffer, size=%d",
                 _insertBufferSize ) ;
         goto error ;
      }

      _insertMsg = (MsgOpInsert*)_insertBuffer ;
      _insertMsg->header.requestID     = 0 ;
      _insertMsg->header.opCode        = MSG_BS_INSERT_REQ ;
      _insertMsg->header.routeID.value = 0 ;
      _insertMsg->header.TID           = ossGetCurrentThreadID() ;

      ossEndianConvertIf( defaultVersion, _insertMsg->version, _endianConvert );
      ossEndianConvertIf( defaultW, _insertMsg->w, _endianConvert ) ;
      ossEndianConvertIf( nameLength, _insertMsg->nameLength, _endianConvert ) ;

      ossStrncpy( _insertMsg->name, clName, nameLength ) ;
      _insertMsg->name[nameLength] = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_bulkInsert( PageInfo* pageInfo, SINT32 flag )
   {
      INT32 rc = SDB_OK ;
      INT32 packetLength = _insertBufferSize ;
      BsonPage* pages = pageInfo->pages ;

      while( pages )
      {
         packetLength += pages->getRecordsSize() ;
         pages = pages->getNext() ;
      }

      _insertMsg->header.messageLength = packetLength ;
      ossEndianConvertIf( flag, _insertMsg->flags, _endianConvert ) ;

      rc = _send( _insertBuffer, _insertBufferSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to send header buffer, rc=%d", rc ) ;
         goto error ;
      }

      pages = pageInfo->pages ;
      while( pages )
      {
         rc = _send( pages->getBuffer(), pages->getRecordsSize() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to send bson buffer, rc=%d", rc ) ;
            goto error ;
         }

         pages = pages->getNext() ;
      }

      rc = _recv() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to recv message, rc=%d", rc ) ;
         goto error ;
      }

      rc = _extract() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to extract message, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_send( const CHAR *pMsg, INT32 len )
   {
      INT32 rc = SDB_OK ;
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      sdbCollectionStruct *cls = (sdbCollectionStruct*)_collection ;
      Socket* sock = cls->_sock ;

      if ( NULL == sock )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      while( len > totalSentSize )
      {
         rc = clientSend( sock, pMsg + totalSentSize, len - totalSentSize,
                          &sentSize, IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_recv()
   {
      INT32 rc        = SDB_OK ;
      INT32 len       = 0 ;
      INT32 realLen   = 0 ;
      INT32 receivedLen = 0 ;
      INT32 totalReceivedLen = 0 ;
      sdbCollectionStruct *cls = (sdbCollectionStruct*)_collection ;
      Socket* sock = cls->_sock ;

      if ( NULL == sock )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      while ( TRUE )
      {
         // get length first
         rc = clientRecv ( sock, ((CHAR*)&len) + totalReceivedLen,
                           sizeof(len) - totalReceivedLen, &receivedLen,
                           IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            goto error ;
         }

   #if defined( _LINUX ) || defined (_AIX)
         #if defined (_AIX)
            #define TCP_QUICKACK TCP_NODELAYACK
         #endif
         // quick ack
         {
            INT32 i = 1 ;
            setsockopt( clientGetRawSocket( sock ),
                        IPPROTO_TCP, TCP_QUICKACK,
                        (void*)&i, sizeof(i) ) ;
         }
   #endif
         break ;
      }

      ossEndianConvertIf4 ( len, realLen, _endianConvert ) ;

      if ( _recvBufferSize < realLen )
      {
         _recvBufferSize = realLen ;

         _recvBuffer = (CHAR*)SDB_OSS_REALLOC( _recvBuffer, _recvBufferSize ) ;
         if ( NULL == _recvBuffer )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "failed to malloc buffer, size=%d",
                    _recvBufferSize ) ;
            goto error ;
         }

         ossMemcpy( _recvBuffer, &realLen, sizeof( realLen ) ) ;
      }

      receivedLen = 0 ;
      totalReceivedLen = sizeof( realLen ) ;
      while ( TRUE )
      {
         INT32 recvLen = realLen - totalReceivedLen ;

         rc = clientRecv ( sock, _recvBuffer + totalReceivedLen,
                           recvLen, &receivedLen,
                           IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            goto error ;
         }

         if ( realLen == totalReceivedLen )
         {
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_extract()
   {
      INT32 rc          = SDB_OK ;
      INT32 replyFlag   = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom   = -1 ;
      SINT64 contextID  = 0 ;

      rc = clientExtractReply( _recvBuffer, &replyFlag, &contextID,
                               &startFrom, &numReturned, _endianConvert ) ;

      if ( rc )
      {
         goto error ;
      }

      rc = replyFlag ;

   done :
      return rc ;
   error :
      goto done ;
   }
}
