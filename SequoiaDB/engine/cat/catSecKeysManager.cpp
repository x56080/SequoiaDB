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

   Source File Name = catSecKeysManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2023  ZHY Init draft
   Last Changed =

*******************************************************************************/
#include "catSecKeysManager.hpp"
#include "ossGMCrypto.hpp"
#include "pmdCB.hpp"
#include "utilSecurityKeys.hpp"
#include "catCommand.hpp"
#include "catCommon.hpp"
#include "catTrace.hpp"

namespace engine
{
#define CAT_SEC_KEYS_UNINITIALIZED_STR "Uninitialized"
#define CAT_SEC_KEYS_CORRUPTED_STR "Corrupted"
#define CAT_SEC_KEYS_AVAILABLE_STR "Available"

   _catSecKeysManager::_catSecKeysManager() {
      _pDmsCB = NULL ;
      _pDpsCB = NULL ;
      _pRtnCB = NULL ;
      _pCatCB = NULL ;
      _status = CAT_SEC_KEYS_UNINITIALIZED ;

      ossMemset( _indexContent, 0, sizeof( _indexContent ) ) ;
      ossMemset( _mkContent, 0, sizeof( _mkContent ) ) ;
      ossMemset( _dekContent, 0, sizeof( _dekContent ) ) ;
      ossMemset( _dekSignContent, 0, sizeof( _dekSignContent ) ) ;
      ossMemset( _dek, 0, sizeof( _dek ) ) ;
   }

   _catSecKeysManager::~_catSecKeysManager() {}

   INT32 _catSecKeysManager::init()
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _pDmsCB = krcb->getDMSCB() ;
      _pDpsCB = krcb->getDPSCB() ;
      _pRtnCB = krcb->getRTNCB() ;
      _pCatCB = krcb->getCATLOGUECB() ;
      ossScopedLock lock( &_latch, EXCLUSIVE, TRUE );
      return _loadKeysInfo() ;
   }

   INT32 _catSecKeysManager::fini()
   {
      return SDB_OK ;
   }

   void _catSecKeysManager::attachCB( pmdEDUCB *cb )
   {
      _pEduCB = cb ;
   }

   void _catSecKeysManager::detachCB( pmdEDUCB *cb )
   {
      _pEduCB = NULL ;
   }

   INT32 _catSecKeysManager::active()
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock( &_latch, EXCLUSIVE, TRUE );
      if( CAT_SEC_KEYS_AVAILABLE != _status )
      {
         rc = _loadKeysInfo() ;
      }
      return rc ;
   }

   INT32 _catSecKeysManager::deactive()
   {
      return SDB_OK ;
   }

   BOOLEAN _catSecKeysManager::isUninitialized()
   {
      ossScopedLock lock( &_latch, SHARED, TRUE );
      return _status == CAT_SEC_KEYS_UNINITIALIZED;
   }

   BOOLEAN _catSecKeysManager::isCorrupted()
   {
      ossScopedLock lock( &_latch, SHARED, TRUE );
      return _status == CAT_SEC_KEYS_CORRUPTED;
   }

   BOOLEAN _catSecKeysManager::isAvailable()
   {
      ossScopedLock lock( &_latch, SHARED, TRUE );
      return _status == CAT_SEC_KEYS_AVAILABLE;
   }

   void _catSecKeysManager::getStatusBson( bson::BSONObjBuilder &builder )
   {
      ossScopedLock lock( &_latch, SHARED, TRUE ) ;
      if ( CAT_SEC_KEYS_UNINITIALIZED == _status )
      {
         builder.append( FIELD_NAME_STATUS, CAT_SEC_KEYS_UNINITIALIZED_STR );
      }
      else if ( CAT_SEC_KEYS_CORRUPTED == _status )
      {
         builder.append( FIELD_NAME_STATUS, CAT_SEC_KEYS_CORRUPTED_STR );
      }
      else
      {
         builder.append( FIELD_NAME_STATUS, CAT_SEC_KEYS_AVAILABLE_STR );
         builder.append( FIELD_NAME_MK, _indexContent );
      }
   }

   const UINT8 *_catSecKeysManager::getDEK()
   {
      ossScopedLock lock( &_latch, SHARED, TRUE );
      SDB_ASSERT( _status == CAT_SEC_KEYS_AVAILABLE, "keys must be available" );
      return _dek;
   }

   INT32 _catSecKeysManager::backupMKPublicAndDEK( BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock( &_latch, SHARED, TRUE );
      SDB_ASSERT( _status == CAT_SEC_KEYS_AVAILABLE, "keys must be available" );
      
      const CHAR *dekCipher = NULL;
      UINT32 sizeOfCipher = 0;
      const CHAR *dekSign = NULL;
      UINT32 sizeOfSign = 0;

      if ( !utilSecGetDEKCipher( _dekContent, &dekCipher, &sizeOfCipher ) ||
           !utilSecGetDEKSign( _dekSignContent, &dekSign, &sizeOfSign ) )
      {
         _status = CAT_SEC_KEYS_CORRUPTED;
         rc = SDB_SEC_KEYS_CORRUPTED;
         goto error;
      }

      rc = utilSecBackupToBson( dekCipher, sizeOfCipher, dekSign, sizeOfSign, _mkContent,
                                ossStrlen( _mkContent ), obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to backup public MK and DEK to bson, rc: %d", rc );

   done:
      return rc ;
   error:
      obj = BSONObj();
      goto done ;
   }

   INT32 _catSecKeysManager::packKeyFileContents( bson::BSONObj &contents )
   {
      ossScopedLock lock( &_latch, SHARED, TRUE ) ;
      return _packKeyFileContents( contents );
   }

   INT32 _catSecKeysManager::processMsg( const NET_HANDLE &handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      switch ( pMsg->opCode )
      {
      case MSG_CAT_FETCH_DEK_REQ : {
         rc = _processFetchDEK( handle, pMsg ) ;
         break ;
      }
      case MSG_CAT_INIT_SECKEYS_REQ : {
         _pCatCB->getCatDCMgr()->setWritedCommand( TRUE ) ;
         rc = _processInitSecKeys( handle, pMsg ) ;
         break ;
      }
      default : {
         rc = SDB_UNKNOWN_MESSAGE ;
         PD_LOG( PDWARNING, "received unknown message (opCode: [%d]%u)",
                 IS_REPLY_TYPE( pMsg->opCode ), GET_REQUEST_TYPE( pMsg->opCode ) ) ;
         break ;
      }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSECKEYSMGR_GET_DEKINFO, "_catSecKeysManager::getDEKInfo" )
   INT32 _catSecKeysManager::getDEKInfo( bson::BSONObj &dekInfo, BOOLEAN &got )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSECKEYSMGR_GET_DEKINFO ) ;
      ossScopedLock lock( &_latch, SHARED, TRUE );
      if ( CAT_SEC_KEYS_AVAILABLE == _status )
      {
         rc = utilSecPrepareDEKInfoBSONObj( _dek, _dekSignContent, dekInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare DEKInfo, rc: %d", rc ) ;
         got = TRUE ;
      }
      else
      {
         rc = _statusToRC();
         got = FALSE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATSECKEYSMGR_GET_DEKINFO, rc ) ;
      return rc ;
   error:
      dekInfo = BSONObj();
      got = FALSE ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSECKEYSMGR_FETCH_DEK, "_catSecKeysManager::_processFetchDEK" )
   INT32 _catSecKeysManager::_processFetchDEK( const NET_HANDLE &handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSECKEYSMGR_FETCH_DEK ) ;

      MsgOpReply *pReply = NULL ;
      CHAR *pBuffer = NULL ;

      BSONObj dekInfo ;

      ossScopedLock lock( &_latch, SHARED, TRUE ) ;
      BOOLEAN isDelay = FALSE ;
      rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay );
      if ( isDelay )
      {
         goto done;
      }
      else if ( rc )
      {
         PD_LOG( PDWARNING,
                 "Service deactive but received msg "
                 "opCode: %d, rc: %d",
                 pMsg->opCode, rc );
         goto error;
      }

      if ( CAT_SEC_KEYS_UNINITIALIZED == _status || CAT_SEC_KEYS_CORRUPTED == _status )
      {
         rc = _statusToRC();
         goto error ;
      }

      rc = utilSecPrepareDEKInfoBSONObj( _dek, _dekSignContent, dekInfo );
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare DEK info, rc: %d", rc );

      pBuffer = (CHAR *)SDB_OSS_MALLOC( sizeof( MsgOpReply ) + dekInfo.objsize() ) ;
      if ( !pBuffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to allocate memory for dek, rc: %d", rc ) ;
         goto error ;
      }

      ossMemcpy( (CHAR *)pBuffer + sizeof( MsgOpReply ), dekInfo.objdata(), dekInfo.objsize() ) ;
      pReply = reinterpret_cast< MsgOpReply * >( pBuffer ) ;
      pReply->header.messageLength = sizeof( MsgOpReply ) + dekInfo.objsize() ;
      pReply->header.TID = pMsg->TID ;
      pReply->header.routeID = pMsg->routeID ;
      pReply->header.requestID = pMsg->requestID ;
      pReply->header.opCode = MSG_CAT_FETCH_DEK_RSP ;
      pReply->header.globalID = pMsg->globalID ;
      pReply->flags = SDB_OK ;
      pReply->numReturned = 1 ;
      pReply->dataLen = dekInfo.objsize() ;

   done:
      if ( !_pCatCB->isDelayed() )
      {
         if ( SDB_OK == rc && pReply )
         {
            rc = _pCatCB->sendReply( handle, pReply, rc ) ;
         }
         else
         {
            // if something wrong happened, return a reply with rc
            MsgOpReply replyMsg ;
            replyMsg.header.messageLength = sizeof( MsgOpReply ) ;
            replyMsg.header.opCode = MSG_CAT_FETCH_DEK_RSP ;
            replyMsg.header.TID = pMsg->TID ;
            replyMsg.header.routeID.value = 0 ;
            replyMsg.header.requestID = pMsg->requestID ;
            replyMsg.header.globalID = pMsg->globalID ;
            replyMsg.numReturned = 0 ;
            replyMsg.flags = rc ;
            replyMsg.contextID = -1 ;
            PD_TRACE1( SDB_CATSECKEYSMGR_FETCH_DEK, PD_PACK_INT( rc ) ) ;

            if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               replyMsg.startFrom = _pCatCB->getPrimaryNode() ;
            }
            rc = _pCatCB->sendReply( handle, &replyMsg, rc ) ;
         }
      }
      SAFE_OSS_FREE( pBuffer ) ;
      PD_TRACE_EXITRC( SDB_CATSECKEYSMGR_FETCH_DEK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSECKEYSMGR_INIT_KEYS, "_catSecKeysManager::_processInitSecKeys" )
   INT32 _catSecKeysManager::_processInitSecKeys( const NET_HANDLE &handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSECKEYSMGR_INIT_KEYS ) ;
      rtnContextBuf buf ;
      MsgOpReply replyHeader ;
      BOOLEAN needRemoveSecDir = FALSE ;
      // init reply msg
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      replyHeader.header.opCode = MSG_CAT_INIT_SECKEYS_RSP ;
      replyHeader.header.requestID = pMsg->requestID ;
      replyHeader.header.routeID.value = 0 ;
      replyHeader.header.TID = pMsg->TID ;
      replyHeader.header.globalID = pMsg->globalID ;
      try
      {
         const CHAR *pCMDName = NULL ;
         INT32 flag = 0 ;
         INT64 numToSkip = 0 ;
         INT64 numToReturn = -1 ;
         const CHAR *pQuery = NULL ;
         const CHAR *pFieldSelector = NULL ;
         const CHAR *pOrderBy = NULL ;
         const CHAR *pHint = NULL ;
         rc = msgExtractQuery( (const CHAR *)pMsg, &flag, &pCMDName, &numToSkip, &numToReturn,
                               &pQuery, &pFieldSelector, &pOrderBy, &pHint ) ;
         BSONObj matcher( pQuery ) ;
         BSONObj selector( pFieldSelector ) ;
         BSONObj orderBy( pOrderBy ) ;
         BSONObj hint( pHint ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract message, rc = %d", rc ) ;

         // primary check
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG( PDWARNING,
                    "Service deactive but received command: %s,"
                    "opCode: %d, rc: %d",
                    pCMDName, pMsg->opCode, rc ) ;
            goto error ;
         }

         if ( _pCatCB->getCatDCMgr()->isWritedCommand() && _pCatCB->isDCReadonly() )
         {
            rc = SDB_CAT_CLUSTER_IS_READONLY;
            goto error;
         }

         rc = catTransEnd(rc, _pEduCB, _pDpsCB) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to end transaction, rc: %d", rc ) ;

         ossScopedLock lock( &_latch, SHARED, TRUE ) ;
         if ( CAT_SEC_KEYS_UNINITIALIZED != _status )
         {
            rc = SDB_SEC_KEYS_INITIALIZED ;
            goto error ;
         }
         lock.release() ;

         ossScopedLock lockExclusive( &_latch, EXCLUSIVE, TRUE ) ;
         if ( CAT_SEC_KEYS_UNINITIALIZED != _status )
         {
            PD_LOG( PDDEBUG, "Security key files were initialized by other thread recently" ) ;
            goto done ;
         }

         needRemoveSecDir = TRUE ;

         rc = utilSecCreateKeyFiles( pmdGetOptionCB()->getDbPath() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create security key files, rc: %d", rc ) ;

         rc = _loadKeysInfo() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load keys info after initialization, rc: %d", rc );

         if ( CAT_SEC_KEYS_AVAILABLE != _status )
         {
            rc = _statusToRC() ;
            PD_LOG( PDERROR, "The recently generated key files are unavailable, rc: %d", rc ) ;
            goto error ;
         }

         BSONObj contents;
         rc = _packKeyFileContents( contents );
         PD_RC_CHECK( rc, PDERROR, "Failed to pack key file contents, rc: %d", rc );

         rc = _writeCrtKeyLogRecord( BSONObj(), contents );
         PD_RC_CHECK( rc, PDERROR, "Failed to write log, rc: %d", rc );

         rc = catUpdateBaseInfoMKIndex(_pEduCB, _pDmsCB, _pDpsCB, _indexContent );
         PD_RC_CHECK( rc, PDERROR, "Failed to upsert key files info to data center, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception :%s", e.what() ) ;
         goto error ;
      }

   done:
      if ( !_pCatCB->isDelayed() )
      {
         // send reply
         rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
      }
      PD_TRACE_EXITRC( SDB_CATSECKEYSMGR_INIT_KEYS, rc ) ;
      return rc ;

   error:
      replyHeader.flags = rc ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      if ( needRemoveSecDir )
      {
         utilSecRemoveSecDirectories( pmdGetOptionCB()->getDbPath() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSECKEYSMGR_LOAD_KEYS, "_catSecKeysManager::_loadKeysInfo" )
   INT32 _catSecKeysManager::_loadKeysInfo()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSECKEYSMGR_LOAD_KEYS ) ;

      BOOLEAN bIndexFileExist = FALSE, bDEKFileExists = FALSE, bDEKSignFileExists = FALSE ;
      CHAR mkDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR indexPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekSignPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;

      utilSecGetKeyDirs( pmdGetOptionCB()->getDbPath(), mkDir, dekDir );
      utilSecGetFilePaths( mkDir, dekDir, indexPath, dekPath, dekSignPath );

      rc = ossAccess( indexPath );
      if ( SDB_FNE == rc )
      {
         bIndexFileExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         bIndexFileExist = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to access index file:%s, rc:%d", indexPath, rc );
         goto error;
      }
      
      rc = ossAccess( dekPath );
      if ( SDB_FNE == rc )
      {
         bDEKFileExists = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         bDEKFileExists = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to access DEK file:%s, rc:%d", dekPath, rc );
         goto error;
      }

      // check if DEK verification file exists
      rc = ossAccess( dekSignPath ) ;
      if ( SDB_FNE == rc )
      {
         bDEKSignFileExists = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         bDEKSignFileExists = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to access DEK file:%s, rc:%d", dekSignPath, rc );
         goto error;
      }

      if ( !bIndexFileExist && !bDEKFileExists && !bDEKSignFileExists )
      {
         _status = CAT_SEC_KEYS_UNINITIALIZED ;
      }
      else if ( !bIndexFileExist || !bDEKFileExists || !bDEKSignFileExists )
      {
         _status = CAT_SEC_KEYS_CORRUPTED ;
      }
      else
      {
         rc = _readKeyFiles( mkDir, indexPath, dekPath, dekSignPath ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to verify key files, rc: %d", rc ) ;

         rc = _verifyKeyFiles();
         PD_RC_CHECK( rc, PDERROR, "Failed to verify key files, rc: %d", rc ) ;
      }

   done:
      if ( CAT_SEC_KEYS_CORRUPTED == _status )
      {
         PD_LOG( PDWARNING, "The security keys may be corrupted") ;
      }
      PD_TRACE_EXITRC( SDB_CATSECKEYSMGR_LOAD_KEYS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSECKEYSMGR_VERI_KEYS, "_catSecKeysManager::_verifyKeyFiles" )
   INT32 _catSecKeysManager::_verifyKeyFiles()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSECKEYSMGR_VERI_KEYS ) ;

      EVP_PKEY *pkey = NULL ;
      const CHAR *dekCipher = NULL;
      UINT32 sizeOfCipher = 0;
      const CHAR *dekSign = NULL;
      UINT32 sizeOfSign = 0;

      if ( !utilSecCompareTag( _indexContent, _dekContent ) )
      {
         _status = CAT_SEC_KEYS_CORRUPTED;
         goto done;
      }

      rc = utilSecReadMKPair( _mkContent, ossStrlen( _mkContent ), &pkey, FALSE, TRUE ) ;
      if ( SDB_SYS == rc )
      {
         _status = CAT_SEC_KEYS_CORRUPTED;
         rc = SDB_OK;
         goto done;
      }
      else if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read master key, rc: %d", rc );
         goto error;
      }

      if ( !utilSecGetDEKCipher( _dekContent, &dekCipher, &sizeOfCipher ) ||
           !utilSecGetDEKSign( _dekSignContent, &dekSign, &sizeOfSign ) )
      {
         _status = CAT_SEC_KEYS_CORRUPTED;
         goto done;
      }

      rc = utilSecDecryptDEK( dekCipher, sizeOfCipher, dekSign, sizeOfSign, pkey, _dek );
      if ( SDB_SYS == rc )
      {
         _status = CAT_SEC_KEYS_CORRUPTED;
         rc = SDB_OK;
         goto done;
      }
      else if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to decrypt DEK cipher, rc: %d", rc );
         goto error;
      }

      _status = CAT_SEC_KEYS_AVAILABLE;

   done:
      PD_TRACE_EXITRC( SDB_CATSECKEYSMGR_VERI_KEYS, rc ) ;
      ossSM2FreeKeyPair( pkey ) ;
      return rc ;
   error:
      _status = CAT_SEC_KEYS_CORRUPTED;
      goto done ;
   }

   INT32 _catSecKeysManager::_statusToRC()
   {
      if ( CAT_SEC_KEYS_UNINITIALIZED == _status )
      {
         return SDB_SEC_KEYS_UNINITIALIZED ;
      }
      else if ( CAT_SEC_KEYS_CORRUPTED == _status )
      {
         return SDB_SEC_KEYS_CORRUPTED ;
      }
      else
      {
         return SDB_OK ;
      }
   }

   INT32 _catSecKeysManager::_readKeyFiles( const CHAR *mkDir,
                                            const CHAR *indexPath,
                                            const CHAR *dekPath,
                                            const CHAR *dekSignPath )
   {
      INT32 rc = SDB_OK ;

      CHAR mkPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' };
      INT64 readIndexLen = sizeof( _indexContent );
      INT64 readMKLen = UTIL_SEC_ENCRYPTED_MK_FILE_BUF_SZ;
      INT64 readDEKLen = UTIL_SEC_DEK_FILE_BUF_SZ;
      INT64 readDEKSignLen = UTIL_SEC_DEK_SIGNATURE_FILE_BUF_SZ;
      rc = utilSecReadFileIntoBuf( indexPath, _indexContent, readIndexLen );
      PD_RC_CHECK( rc, PDERROR, "Failed to read file:%s, rc: %d", indexPath, rc );

      utilSecGetMKPath( mkDir, _indexContent, mkPath );
      // check if MK file exists
      rc = ossAccess( mkPath ) ;
      if ( SDB_FNE == rc )
      {
         _status = CAT_SEC_KEYS_CORRUPTED ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         rc = utilSecReadFileIntoBuf( mkPath, _mkContent, readMKLen );
         PD_RC_CHECK( rc, PDERROR, "Failed to read file:%s, rc: %d", mkPath, rc );
      }
      else
      {
         PD_LOG( PDERROR, "Failed to access DEK file:%s, rc:%d", mkPath, rc );
         goto error;
      }

      rc = utilSecReadFileIntoBuf( dekPath, _dekContent, readDEKLen );
      PD_RC_CHECK( rc, PDERROR, "Failed to read file:%s, rc: %d", dekPath, rc );

      rc = utilSecReadFileIntoBuf( dekSignPath, _dekSignContent, readDEKSignLen );
      PD_RC_CHECK( rc, PDERROR, "Failed to read file:%s, rc: %d", dekSignPath, rc );

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSecKeysManager::_packKeyFileContents( bson::BSONObj &contents )
   {
      INT32 rc = SDB_OK ;
      
      if (  CAT_SEC_KEYS_AVAILABLE != _status)
      {
         contents = BSONObj() ;
         goto done ;
      }
      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_MKINDEX, _indexContent ) ;
         builder.append( FIELD_NAME_DEK, _dekContent ) ;
         builder.append( FIELD_NAME_DEKVERIFICATION, _dekSignContent ) ;
         builder.append( FIELD_NAME_MK, _mkContent ) ;
         contents = builder.obj();
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception :%s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSecKeysManager::_writeCrtKeyLogRecord( const bson::BSONObj &oldKeyFiles,
                                                    const bson::BSONObj &newKeyFiles )
   {
      INT32 rc = SDB_OK ;
      dpsTransCB *transCB = sdbGetTransCB() ;
      UINT32 logRecSize = 0 ;

      dpsMergeInfo info ;
      info.setDefInfoEx() ;
      dpsLogRecord &record = info.getMergeBlock().record() ;

      rc = dpsCrtKeys2Record( oldKeyFiles, newKeyFiles, record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build create key log:%d", rc ) ;
         goto error ;
      }

      logRecSize = record.alignedLen() ;
      rc = transCB->reservedLogSpace( logRecSize, _pEduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to reserved log space for "
                 "create key log, rc: %d",
                 rc ) ;
         logRecSize = 0 ;
         goto error ;
      }

      rc = _pDpsCB->prepare( info ) ;
      if ( SDB_OK == rc )
      {
         _pDpsCB->writeData( info ) ;
      }

   done:
      if ( 0 != logRecSize )
      {
         transCB->releaseLogSpace( logRecSize, _pEduCB ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSecKeysManager::replayCrtKeyLogRecord( const bson::BSONObj &contents )
   {
      INT32 rc = SDB_OK;
      ossScopedLock lock( &_latch, EXCLUSIVE, TRUE );

      if ( contents.isEmpty() )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "key files can not be empty, rc: %d", rc );
         goto error;
      }

      try
      {
         const CHAR *indexContent = contents.getStringField( FIELD_NAME_MKINDEX );
         const CHAR *mkContent = contents.getStringField( FIELD_NAME_MK );
         const CHAR *dekContent = contents.getStringField( FIELD_NAME_DEK );
         const CHAR *dekSignContent = contents.getStringField( FIELD_NAME_DEKVERIFICATION );

         ossStrcpy( _indexContent, indexContent );
         ossStrcpy( _mkContent, mkContent );
         ossStrcpy( _dekContent, dekContent );
         ossStrcpy( _dekSignContent, dekSignContent );

         rc = _verifyKeyFiles();
         PD_RC_CHECK( rc, PDERROR, "Failed to verify key file contents, rc: %d", rc );

         if ( CAT_SEC_KEYS_AVAILABLE != _status )
         {
            rc = _statusToRC() ;
            PD_LOG( PDERROR, "The recently generated key files are unavailable, rc: %d", rc ) ;
            goto error ;
         }

         rc = utilSecWriteKeyFileContents( pmdGetOptionCB()->getDbPath(), _indexContent, _mkContent,
                                           _dekContent, _dekSignContent );
         PD_RC_CHECK( rc, PDERROR, "Failed to write key files, rc; %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Occur exception :%s", e.what() );
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSecKeysManager::rollbackCrtKeyLogRecord( const bson::BSONObj &oldContents )
   {
      INT32 rc = SDB_OK ;
      ossScopedLock lock( &_latch, EXCLUSIVE, TRUE );

      // In current version, the old key file contents must be empty
      if ( !oldContents.isEmpty() )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "old key files must be empty, rc: %d", rc );
         goto error;
      }

      rc = utilSecRemoveSecDirectories(pmdGetOptionCB()->getDbPath());
      PD_RC_CHECK( rc, PDERROR, "Failed to remove security directory, rc: %d", rc );

      rc = _loadKeysInfo();
      PD_RC_CHECK( rc, PDERROR, "Failed to load key files, rc: %d", rc );

      if ( CAT_SEC_KEYS_UNINITIALIZED != _status )
      {
         rc = SDB_SEC_KEYS_INITIALIZED;
         PD_LOG( PDERROR, "The keys must be uninitialized after rollback, rc: %d", rc );
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
} // namespace engine