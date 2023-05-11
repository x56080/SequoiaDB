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