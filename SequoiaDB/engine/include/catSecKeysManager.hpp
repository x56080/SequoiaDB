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

   Source File Name = catSecKeysManager.hpp

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
#ifndef CATSECKEYSMANAGER_HPP__
#define CATSECKEYSMANAGER_HPP__

#include "ISecKeysManager.hpp"
#include "../bson/bson.h"
#include "pmd.hpp"
#include "utilSecurityKeys.hpp"

namespace engine
{
   class _catSecKeysManager : public _ISecKeysManager
   {
      private:
         enum CAT_SEC_KEYS_STATUS
         {
            CAT_SEC_KEYS_UNINITIALIZED = 0,
            CAT_SEC_KEYS_CORRUPTED,
            CAT_SEC_KEYS_AVAILABLE
         };

      public:
         _catSecKeysManager();
         virtual ~_catSecKeysManager();

         INT32 init();
         INT32 fini();

         void attachCB( _pmdEDUCB *cb );
         void detachCB( _pmdEDUCB *cb );

         INT32 active();
         INT32 deactive();

      public:
         BOOLEAN isUninitialized() ;
         BOOLEAN isCorrupted() ;
         BOOLEAN isAvailable() ;
         void getStatusBson( bson::BSONObjBuilder &builder ) ;
         const UINT8* getDEK() ;
         INT32 backupMKPublicAndDEK( bson::BSONObj &obj ) ;
         INT32 packKeyFileContents( bson::BSONObj &contents ) ;
         INT32 replayCrtKeyLogRecord( const bson::BSONObj &contents ) ;
         INT32 rollbackCrtKeyLogRecord( const bson::BSONObj &oldContents ) ;

         INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

         virtual INT32 getDEKInfo( bson::BSONObj &dekInfo, BOOLEAN &got ) ;

      private:
         INT32 _processInitSecKeys( const NET_HANDLE &handle, MsgHeader *pMsg );
         INT32 _processFetchDEK( const NET_HANDLE &handle, MsgHeader *pMsg );

         INT32 _loadKeysInfo();
         INT32 _readKeyFiles( const CHAR *mkDir,
                              const CHAR *indexPath,
                              const CHAR *dekPath,
                              const CHAR *dekSignPath );
         INT32 _verifyKeyFiles();

         INT32 _statusToRC();

         INT32 _packKeyFileContents( bson::BSONObj &contents );
         INT32 _writeCrtKeyLogRecord( const bson::BSONObj &oldKeyFiles,
                                      const bson::BSONObj &newKeyFiles );

      private:
         _SDB_DMSCB *_pDmsCB;
         _dpsLogWrapper *_pDpsCB;
         _SDB_RTNCB *_pRtnCB;
         sdbCatalogueCB *_pCatCB;
         _pmdEDUCB *_pEduCB;

         ossSpinSLatch _latch;
         CAT_SEC_KEYS_STATUS _status;

         CHAR _indexContent[ UTIL_SEC_MK_INDEX_FILE_BUF_SZ ];
         CHAR _mkContent[ UTIL_SEC_ENCRYPTED_MK_FILE_BUF_SZ ];
         CHAR _dekContent[ UTIL_SEC_DEK_FILE_BUF_SZ ];
         CHAR _dekSignContent[ UTIL_SEC_DEK_SIGNATURE_FILE_BUF_SZ ];
         ossSM4Key _dek;
   };
   typedef _catSecKeysManager catSecKeysManager;
} // namespace engine

#endif