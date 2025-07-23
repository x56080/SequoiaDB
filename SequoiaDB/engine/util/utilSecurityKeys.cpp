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

   Source File Name = utilSecurityKeys.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/02/2023  JT  Initial Draft
          04/11/2023  ZHY Move to util

   Last Changed =

*******************************************************************************/
#include "ossGMCrypto.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "utilSecurityKeys.hpp"
#include "pmdEDU.hpp"
#include "../bson/lib/base64.h" // base64
#include "../bson/bson.h"
#include "msgDef.h"
#include "openssl/pem.h"

using namespace bson ;
using namespace base64 ;

namespace engine
{
   INT32 utilSecReadFileIntoBuf( const CHAR *fileName, CHAR *pBuf, INT64 &len )
   {
      INT32 rc = SDB_OK ;
      INT64 readed = 0, fileSz = 0 ;
      BOOLEAN bFileOpened = FALSE ;
      OSSFILE osFile ;

      if ( ( NULL == fileName ) || ( NULL == pBuf ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      rc = ossAccess( fileName ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to access file:%s, rc: %d", fileName, rc ) ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }
      // get file size
      rc = ossGetFileSizeByName( fileName, &fileSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to access file:%s, rc: %d", fileName, rc ) ;
         goto error ;
      }
      if ( fileSz > len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "File size:%lld is larger than buffer size:%lld, rc: %d", fileSz, len,
                 rc ) ;
         goto error ;
      }
      // read from file
      rc = ossOpen( fileName, OSS_READONLY, 0, osFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", fileName, rc ) ;
      bFileOpened = TRUE ;
      rc = ossSeek( &osFile, 0, OSS_SEEK_SET ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to access file:%s, rc: %d", fileName, rc ) ;
      rc = ossReadN( &osFile, fileSz + 1, pBuf, readed ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read file:%s, rc: %d", fileName, rc ) ;
      len = readed ;
   done:
      if ( bFileOpened )
      {
         ossClose( osFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecWriteBufToFile( const CHAR *fileNm, const CHAR *pBuf, INT64 len )
   {
      INT32 rc = SDB_OK ;
      INT64 writeLen = 0 ;
      BOOLEAN bFileOpened = FALSE ;
      OSSFILE osFile ;

      if ( ( NULL == fileNm ) || ( NULL == pBuf ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      // create new file
      rc = ossOpen( fileNm, OSS_REPLACE | OSS_READWRITE, ( OSS_RU | OSS_WU ), osFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", fileNm, rc ) ;
      bFileOpened = TRUE ;
      // write to file
      rc = ossSeek( &osFile, 0, OSS_SEEK_SET ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update file:%s, rc: %d", fileNm, rc ) ;
      rc = ossWrite( &osFile, (CHAR *)pBuf, len, &writeLen ) ;
      if ( ( SDB_OK != rc ) || ( len != writeLen ) )
      {
         if ( SDB_OK == rc )
         {
            rc = SDB_SYS ;
         }
         PD_LOG( PDERROR, "Failed to write to file:%s, rc: %d", fileNm, rc ) ;
         goto error ;
      }
      rc = ossFsync( &osFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", fileNm, rc ) ;
   done:
      if ( bFileOpened )
      {
         ossClose( osFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecGenMKTagByTime( CHAR *pMKTag )
   {
      INT32 rc = SDB_OK ;
      ossTimestamp Tm ;
      struct tm tmInfo ;

      if ( NULL == pMKTag )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      ossGetCurrentTime( Tm ) ;
      ossLocalTime( Tm.time, tmInfo ) ;

      if ( Tm.microtm >= OSS_ONE_MILLION )
      {
         tmInfo.tm_sec++ ;
         Tm.microtm %= OSS_ONE_MILLION ;
      }
      ossSnprintf( pMKTag, UTIL_SEC_MK_TAG_STR_LEN, "mk%04d%02d%02d%02d%02d%02d%06d",
                   tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday, tmInfo.tm_hour,
                   tmInfo.tm_min, tmInfo.tm_sec, Tm.microtm ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecGetMKTagFromDEKFile( const CHAR *dekFullPathName, CHAR *pMKTag )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[ UTIL_SEC_DEK_FILE_BUF_SZ ] = { '\0' } ;
      CHAR *pos = NULL ;
      INT64 readed = sizeof( buf ) ;

      if ( ( NULL == dekFullPathName ) || ( NULL == pMKTag ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }
      // read dek file into buffer
      rc = utilSecReadFileIntoBuf( dekFullPathName, buf, readed ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read file:%s, rc: %d", dekFullPathName, rc ) ;
         goto error ;
      }
      //
      pos = ossStrstr( buf, UTIL_SEC_TAG_MKTAG_END ) ;
      if ( NULL == pos )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid DEK file:%s, rc: %d", dekFullPathName, rc ) ;
         goto error ;
      }
      *pos = '\0' ;
      ossSnprintf( pMKTag, pos - buf - ossStrlen( UTIL_SEC_TAG_MKTAG_TAG ) + 1, "%s",
                   buf + ossStrlen( UTIL_SEC_TAG_MKTAG_TAG ) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecBackupDEKFile( const CHAR *fileName )
   {
      INT32 rc = SDB_OK ;
      CHAR bakFileName[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekTag[ UTIL_SEC_MK_TAG_STR_LEN + 1 ] = { '\0' } ;

      if ( ( NULL == fileName ) || ( '\0' == fileName[ 0 ] ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      // rename DEK file
      rc = ossAccess( fileName ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to access to file:%s, rc: %d", fileName, rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }
      else
      {
         rc = utilSecGetMKTagFromDEKFile( fileName, dekTag ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_FNE != rc )
            {
               PD_LOG( PDERROR, "Failed to access to file:%s, rc: %d", fileName, rc ) ;
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else
         {
            if ( '\0' != dekTag[ 0 ] )
            {
               ossSnprintf( bakFileName, sizeof( bakFileName ), "%s.%s", fileName, dekTag ) ;
               rc = ossRenamePath( fileName, bakFileName ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Rename file:%s to %s failed, rc: %d", fileName, bakFileName,
                          rc ) ;
                  goto error ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "Invalid DEK file:%s", fileName ) ;
               ossSnprintf( bakFileName, sizeof( bakFileName ), "%s%s", fileName,
                            UTIL_SEC_FILE_SUFFIX_BAK ) ;
               rc = ossRenamePath( fileName, bakFileName ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Rename file:%s to %s failed, rc: %d", fileName, bakFileName,
                          rc ) ;
                  goto error ;
               }
            }
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecVerifyDEK( const ossSM4Key pDEK, const CHAR *dekSignature, BOOLEAN &result )
   {
      INT32 rc = SDB_OK ;
      UINT8 buf[ UTIL_SEC_DEK_SIGNATURE_SZ ] = { 0 } ;
      UINT32 len = sizeof( buf ) ;
      string signatureStr ;
      ossSM4Context ctx ;

      result = FALSE ;

      if ( ( NULL == pDEK ) || ( NULL == dekSignature ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      rc = ossSM4Init( &ctx, OSS_SM4_CBC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize SM4 cipher context, rc: %d", rc ) ;
      ctx.setKey( pDEK ) ;

      len = UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
      rc = ossSM4EncryptHybrid( &ctx, (UINT8 *)UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT, buf, len ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to verify DEK signature, rc: %d", rc ) ;
      signatureStr = base64::encode( (CHAR *)buf, len ) ;

      // compare with DEK signaure
      if ( 0 == ossStrncmp( signatureStr.c_str(), dekSignature, signatureStr.length() ) )
      {
         result = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecRemoveSecDirectories( const CHAR *dbpath )
   {
      INT32 rc = SDB_OK ;

      CHAR secPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;

      if ( ( NULL == dbpath ) || ( '\0' == dbpath[ 0 ] ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         return rc ;
      }

      // construct security directory name
      ossSnprintf( secPath, sizeof( secPath ), "%s%s%s", dbpath, UTIL_SEC_SECURITY_DIR_NAME,
                   OSS_FILE_SEP ) ;

      rc = ossAccess( secPath ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to access directory:%s, rc: %d", secPath, rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }
      else
      {
         rc = ossDelete( secPath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to remove directory:%s, rc: %d", secPath, rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void utilSecGetKeyDirs( const CHAR *dbPath, CHAR *mkDir, CHAR *dekDir )
   {
      ossSnprintf( mkDir, OSS_MAX_PATHSIZE, "%s%s%s%s%s", dbPath, UTIL_SEC_SECURITY_DIR_NAME,
                   OSS_FILE_SEP, UTIL_SEC_MK_DIR_NAME, OSS_FILE_SEP ) ;

      ossSnprintf( dekDir, OSS_MAX_PATHSIZE, "%s%s%s%s%s", dbPath, UTIL_SEC_SECURITY_DIR_NAME,
                   OSS_FILE_SEP, UTIL_SEC_DEK_DIR_NAME, OSS_FILE_SEP ) ;
   }

   void utilSecGetFilePaths( const CHAR *mkDir,
                             const CHAR *dekDir,
                             CHAR *indexPath,
                             CHAR *dekPath,
                             CHAR *dekSignPath )
   {
      ossSnprintf( indexPath, OSS_MAX_PATHSIZE, "%s%s", mkDir, UTIL_SEC_INDEX_FILE_NAME ) ;
      ossSnprintf( dekPath, OSS_MAX_PATHSIZE, "%s%s", dekDir, UTIL_SEC_DEK_FILE_NAME ) ;
      ossSnprintf( dekSignPath, OSS_MAX_PATHSIZE, "%s%s", dekDir, UTIL_SEC_DEK_SIGN_FILE_NAME ) ;
   }

   void utilSecGetMKName( const CHAR *mkTag, CHAR *mkFileName )
   {
      ossSnprintf( mkFileName, UTIL_SEC_MK_INDEX_FILE_BUF_SZ, "%s%s", mkTag,
                   UTIL_SEC_MK_FILE_SUFFIX ) ;
   }

   void utilSecGetMKPath( const CHAR *mkDir, const CHAR *mkFileName, CHAR *mkPath )
   {
      ossSnprintf( mkPath, OSS_MAX_PATHSIZE, "%s%s", mkDir, mkFileName ) ;
   }

   INT32 utilSecEnsureSecDirectories( const CHAR *mkDir, const CHAR *dekDir )
   {
      INT32 rc = SDB_OK ;
      // create DbPath()/security/MK directory,
      // it's OK if the directory exists
      rc = ossMkdir( mkDir, OSS_RWXU ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FE != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to create directory:%s, rc: %d", mkDir, rc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }
      // create Dbpath()/security/DEK directory,
      // it's OK if the directory exists
      rc = ossMkdir( dekDir, OSS_RWXU ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FE != rc )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to create directory:%s, rc: %d", dekDir, rc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecCreateIndexFile( const CHAR *indexFilePath, const CHAR *mkFileName )
   {
      INT32 rc = SDB_OK ;
      OSSFILE idxFile ;
      BOOLEAN bFileOpened = FALSE ;
      INT64 writeLen = -1 ;
      rc = ossOpen( indexFilePath, OSS_REPLACE | OSS_READWRITE, ( OSS_RU | OSS_WU ), idxFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", indexFilePath, rc ) ;
      bFileOpened = TRUE ;

      rc = ossSeek( &idxFile, 0, OSS_SEEK_SET ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to access file:%s, rc: %d", indexFilePath, rc ) ;
      rc = ossWrite( &idxFile, mkFileName, ossStrlen( mkFileName ), &writeLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write file:%s, rc: %d", indexFilePath, rc ) ;
      rc = ossFsync( &idxFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write file:%s, rc: %d", indexFilePath, rc ) ;
   done:
      if ( bFileOpened )
      {
         ossClose( idxFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 utilSecCreateMKFile( const CHAR *mkFilePath, EVP_PKEY *pKey )
   {
      INT32 rc = SDB_OK ;
      rc = ossAccess( mkFilePath ) ;
      if ( SDB_OK == rc )
      {
         PD_LOG( PDERROR, "MK file:%s already exists, rc: %d", mkFilePath, rc ) ;
         goto error ;
      }
      else if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access MK file: %s, rc: %d", mkFilePath, rc ) ;
         goto error ;
      }

      rc = ossSM2WriteKeyPairToFile( mkFilePath, pKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write key pair to file: %s, rc: %d", mkFilePath, rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

#define UTIL_SEC_ENCRYPTED_DEK_SZ ( 128 )
// // ceil( UTIL_SEC_ENCRYPTED_DEK_SZ/3 ) * 4 = 172
// #define UTIL_SEC_ENCRYPTED_DEK_BASE64_SZ ( 172 )
#define UTIL_SEC_DEK_SIGN_FILE_BUF_SZ ( 256 )

   INT32 utilSecCreateDEKFile( const CHAR *dekFilePath,
                               const CHAR *mkTag,
                               ossSM4Key plainDEK,
                               EVP_PKEY *pKey )
   {
      INT32 rc = SDB_OK ;

      UINT8 encryptedDEK[ UTIL_SEC_ENCRYPTED_DEK_SZ ] = { 0 } ;
      size_t len = 0 ;
      OSSFILE dekFile ;
      BOOLEAN bFileOpened = FALSE ;

      rc = ossAccess( dekFilePath ) ;
      if ( SDB_OK == rc )
      {
         rc = utilSecBackupDEKFile( dekFilePath ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to backup DEK file:%s, rc: %d", dekFilePath, rc ) ;
      }
      else if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access DEK file: %s, rc: %d", dekFilePath, rc ) ;
         goto error ;
      }

      rc = ossSM2Encrypt( pKey, plainDEK, OSS_SM4_KEY_SIZE, encryptedDEK, &len ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to encrypted DEK by MK, rc: %d", rc ) ;

      try
      {
         UINT8 digest[ UTIL_SEC_SM3_DIGEST_SZ ] = { 0 } ;
         UINT32 digestLen = 0 ;
         UINT8 fileBuf[ UTIL_SEC_DEK_FILE_BUF_SZ ] = { 0 } ;
         UINT32 fileLen = 0 ;
         std::string encodedDEK = base64::encode( (const CHAR *)encryptedDEK, len ) ;
         INT64 writeLen = -1 ;

         rc = ossSM3Digest( (const UINT8 *)encodedDEK.data(), encodedDEK.length(), digest,
                            &digestLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to calculate digest, rc: %d", rc ) ;

         std::string encodedDigest = base64::encode( (const CHAR *)digest, digestLen ) ;

         // tag
         fileLen += ossSnprintf( (CHAR *)fileBuf, sizeof( fileBuf ), "%s%s%s" OSS_NEWLINE,
                                 UTIL_SEC_TAG_MKTAG_TAG, mkTag, UTIL_SEC_TAG_MKTAG_END ) ;
         // encrypted dek
         fileLen += ossSnprintf( (CHAR *)( fileBuf + fileLen ), sizeof( fileBuf ) - fileLen,
                                 "%s" OSS_NEWLINE, UTIL_SEC_TAG_ENCRYPTED_DEK ) ;
         fileLen += ossSnprintf( (CHAR *)( fileBuf + fileLen ), sizeof( fileBuf ) - fileLen,
                                 "%s" OSS_NEWLINE, encodedDEK.c_str() ) ;
         // digest
         fileLen += ossSnprintf( (CHAR *)( fileBuf + fileLen ), sizeof( fileBuf ) - fileLen,
                                 "%s" OSS_NEWLINE, UTIL_SEC_TAG_DIGEST ) ;
         fileLen += ossSnprintf( (CHAR *)( fileBuf + fileLen ), sizeof( fileBuf ) - fileLen,
                                 "%s" OSS_NEWLINE, encodedDigest.c_str() ) ;
         fileLen += ossSnprintf( (CHAR *)( fileBuf + fileLen ), sizeof( fileBuf ) - fileLen,
                                 "%s" OSS_NEWLINE, UTIL_SEC_TAG_END_OF_FILE ) ;

         // open dek file
         rc = ossOpen( dekFilePath, OSS_REPLACE | OSS_READWRITE, ( OSS_RU | OSS_WU ), dekFile ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", dekFilePath, rc ) ;
         bFileOpened = TRUE ;
         // write to file
         rc = ossSeek( &dekFile, 0, OSS_SEEK_SET ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update file:%s, rc: %d", dekFilePath, rc ) ;
         rc = ossWrite( &dekFile, (CHAR *)fileBuf, fileLen, &writeLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", dekFilePath, rc ) ;
         rc = ossFsync( &dekFile ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", dekFilePath, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( bFileOpened )
      {
         ossClose( dekFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecCreateDEKSignFile( const CHAR *deSignFilePath, ossSM4Key plainDEK )
   {
      INT32 rc = SDB_OK ;
      OSSFILE dekSignFile ;
      BOOLEAN bFileOpened = FALSE ;
      UINT8 encrypted[ UTIL_SEC_DEK_SIGNATURE_SZ ] = { 0 } ;
      UINT32 encLen = sizeof( encrypted ) ;
      ossSM4Context ctx ;

      rc = ossSM4Init( &ctx, OSS_SM4_CBC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize SM4 cipher context, rc: %d", rc ) ;
      ctx.setKey( plainDEK ) ;

      encLen = UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
      rc = ossSM4EncryptHybrid( &ctx, (UINT8 *)UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT, encrypted,
                                encLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to encrypte signature text, rc: %d", rc ) ;

      try
      {
         UINT8 digest[ UTIL_SEC_SM3_DIGEST_SZ ] = { 0 } ;
         UINT32 digestLen = 0 ;
         CHAR fileBuf[ UTIL_SEC_DEK_SIGN_FILE_BUF_SZ ] = { 0 } ;
         UINT32 fileLen = 0 ;
         std::string encoded = base64::encode( (CHAR *)encrypted, encLen ) ;
         INT64 writeLen = -1 ;

         rc = ossSM3Digest( (const UINT8 *)encoded.data(), encoded.length(), digest, &digestLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to calculate digest, rc: %d", rc ) ;

         std::string encodedDigest = base64::encode( (const CHAR *)digest, digestLen ) ;

         fileLen += ossSnprintf( fileBuf, UTIL_SEC_DEK_SIGN_FILE_BUF_SZ, "%s%s%s%s%s%s%s%s",
                                 encoded.c_str(), OSS_NEWLINE, UTIL_SEC_TAG_DIGEST, OSS_NEWLINE,
                                 encodedDigest.c_str(), OSS_NEWLINE, UTIL_SEC_TAG_END_OF_FILE,
                                 OSS_NEWLINE ) ;

         rc = ossAccess( deSignFilePath ) ;
         if ( SDB_OK == rc )
         {
            CHAR readBuf[ UTIL_SEC_DEK_SIGN_FILE_BUF_SZ ] = { 0 } ;
            INT64 readLen = UTIL_SEC_DEK_SIGN_FILE_BUF_SZ ;
            rc = utilSecReadFileIntoBuf( deSignFilePath, readBuf, readLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to read existing DEK signature file, rc: %d", rc ) ;
            if ( fileLen != readLen || 0 != ossMemcmp( fileBuf, readBuf, fileLen ) )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR,
                       "The existing DEK signature file is different from the file to be created" ) ;
               goto error ;
            }
         }
         else if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to access DEK file: %s, rc: %d", deSignFilePath, rc ) ;
            goto error ;
         }
         else
         {
            rc = ossOpen( deSignFilePath, OSS_CREATEONLY | OSS_READWRITE | OSS_EXCLUSIVE,
                          ( OSS_RU | OSS_WU ), dekSignFile ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", deSignFilePath, rc ) ;
            bFileOpened = TRUE ;

            // write to file
            rc = ossSeek( &dekSignFile, 0, OSS_SEEK_SET ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update file:%s, rc: %d", deSignFilePath, rc ) ;
            rc = ossWrite( &dekSignFile, fileBuf, fileLen, &writeLen ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", deSignFilePath, rc ) ;
            rc = ossFsync( &dekSignFile ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", deSignFilePath, rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( bFileOpened )
      {
         ossClose( dekSignFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecCreateKeyFiles( const CHAR *dbPath )
   {
      INT32 rc = SDB_OK ;
      CHAR mkDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR indexFilePath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR mkFilePath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekFilePath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekSignFilePath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR mkTag[ UTIL_SEC_MK_TAG_STR_LEN + 1 ] = { '\0' } ;
      CHAR mkFileName[ UTIL_SEC_MK_INDEX_FILE_BUF_SZ + 1 ] = { '\0' } ;
      ossSM4Key DEK ;
      EVP_PKEY *pKey = NULL ;
      utilSecGetKeyDirs( dbPath, mkDir, dekDir ) ;
      utilSecGetFilePaths( mkDir, dekDir, indexFilePath, dekFilePath, dekSignFilePath ) ;

      rc = utilSecEnsureSecDirectories( mkDir, dekDir ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to access security directories, rc: %d", rc ) ;

      // create MK tag by localtime
      rc = utilSecGenMKTagByTime( mkTag ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate MK Tag[%s], rc: %d", mkTag, rc ) ;
      utilSecGetMKName( mkTag, mkFileName ) ;
      utilSecGetMKPath( mkDir, mkFileName, mkFilePath ) ;

      // create DEK
      rc = ossSM4GetRand128( DEK, sizeof( DEK ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get random 128 bits, rc: %d", rc ) ;

#if defined( _DEBUG )
      {
         CHAR dekBuf[ 128 ] = { 0 } ;
         ossHexDumpBuffer( DEK, sizeof( DEK ), dekBuf, sizeof( dekBuf ), NULL,
                           OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
         PD_LOG( PDDEBUG, "DEK created:\n%s", dekBuf ) ;
      }
#endif

      rc = ossSM2GenKeyPair( &pKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate SM2 key pair, rc: %d", rc ) ;

      rc = utilSecCreateMKFile( mkFilePath, pKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create MK file, rc: %d", rc ) ;

      rc = utilSecCreateDEKFile( dekFilePath, mkTag, DEK, pKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create DEK file, rc: %d", rc ) ;

      rc = utilSecCreateDEKSignFile( dekSignFilePath, DEK ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create DEK signature file, rc: %d", rc ) ;

      rc = utilSecCreateIndexFile( indexFilePath, mkFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index file, rc: %d", rc ) ;
   done:
      ossSM2FreeKeyPair( pKey ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecReadMKPair( const CHAR *mkContent,
                            UINT32 size,
                            EVP_PKEY **ppKey,
                            BOOLEAN readPublic,
                            BOOLEAN readPrivate )
   {
      INT32 rc = SDB_OK ;
      BIO *bio = NULL ;
      bio = BIO_new_mem_buf( mkContent, size ) ;
      if ( !bio )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to construct BIO from mk content, rc: %d", rc ) ;
         goto error ;
      }

      if ( readPrivate )
      {
         if ( !PEM_read_bio_PrivateKey( bio, ppKey, NULL, (void *)OSS_SM2_KEYFILE_PASSPHRASE ) )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Failed to read private master key" ) ;
            goto error ;
         }
      }

      if ( readPublic )
      {
         if ( !PEM_read_bio_PUBKEY( bio, ppKey, NULL, NULL ) )
         {
            rc = SDB_SYS ;
            PD_LOG_MSG( PDERROR, "Failed to read public master key" ) ;
            goto error ;
         }
      }

      if ( 1 != ( EVP_PKEY_set_alias_type( *ppKey, EVP_PKEY_SM2 ) ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      BIO_free_all( bio ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecReadMKPair( const CHAR *mkPubContent, const CHAR *mkPrivPath, EVP_PKEY **ppKey )
   {
      INT32 rc = SDB_OK ;

      EVP_PKEY *pubKey = NULL, *privKey = NULL ;

      rc = ossSM2ReadPublicKeyFromString( mkPubContent, &pubKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read public key from string: %s, rc: %d", mkPubContent,
                   rc ) ;

      rc = ossSM2ReadPrivateKeyFromFile( mkPrivPath, &privKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read private key from file: %s, rc: %d", mkPrivPath,
                   rc ) ;

      *ppKey = EVP_PKEY_new() ;
      if ( !*ppKey )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      if ( !EVP_PKEY_copy_parameters( *ppKey, pubKey ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !EVP_PKEY_set1_EC_KEY( *ppKey, EVP_PKEY_get1_EC_KEY( privKey ) ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( 1 != ( EVP_PKEY_set_alias_type( *ppKey, EVP_PKEY_SM2 ) ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      ossSM2FreeKeyPair( privKey ) ;
      ossSM2FreeKeyPair( pubKey ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN utilSecGetDEKCipher( const CHAR *dekContent, const CHAR **dekCipher, UINT32 *size )
   {
      SDB_ASSERT( dekCipher && size, "can not be nullptr" ) ;
      const CHAR *pos1 = ossStrstr( dekContent, UTIL_SEC_TAG_ENCRYPTED_DEK ) ;
      const CHAR *pos2 = ossStrstr( dekContent, UTIL_SEC_TAG_DIGEST ) ;
      if ( !pos1 || !pos2 )
      {
         *dekCipher = NULL ;
         *size = 0 ;
         return FALSE ;
      }

      *dekCipher = pos1 + sizeof( UTIL_SEC_TAG_ENCRYPTED_DEK ) ;
      if ( size )
      {
         *size = pos2 - *dekCipher - OSS_NEWLINE_SIZE ;
      }

      return TRUE ;
   }

   BOOLEAN utilSecGetDEKSign( const CHAR *dekSignContent, const CHAR **dekSign, UINT32 *size )
   {
      SDB_ASSERT( dekSign && size, "can not be nullptr" ) ;
      const CHAR *pos1 = ossStrstr( dekSignContent, UTIL_SEC_TAG_DIGEST ) ;
      const CHAR *pos2 = ossStrstr( dekSignContent, UTIL_SEC_TAG_END_OF_FILE ) ;
      if ( !pos1 || !pos2 )
      {
         *dekSign = NULL ;
         *size = 0 ;
         return FALSE ;
      }

      *dekSign = dekSignContent ;
      if ( size )
      {
         *size = pos1 - dekSignContent - OSS_NEWLINE_SIZE ;
      }

      return TRUE ;
   }

   INT32 utilSecDecryptDEK( const CHAR *dekCipher,
                            const UINT32 sizeOfDekCipher,
                            const CHAR *dekSign,
                            const UINT32 sizeOfDekSign,
                            EVP_PKEY *mk,
                            ossSM4Key plainDEK )
   {
      INT32 rc = SDB_OK ;
      ossSM4Context ctx ;
      SDB_ASSERT( mk && plainDEK, "can not be nullptr" ) ;

      try
      {
         std::string origin( dekCipher, sizeOfDekCipher ) ;
         std::string decoded = base64::decode( origin ) ;
         UINT8 outDEKBuf[ UTIL_SEC_ENCRYPTED_DEK_SZ ] = { 0 } ;
         size_t out = UTIL_SEC_ENCRYPTED_DEK_SZ ;
         const size_t outSignLen = UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
         UINT8 outSign[ outSignLen ] = { 0 } ;

         rc = ossSM2Decrypt( mk, (const UINT8 *)decoded.data(), decoded.size(), outDEKBuf, &out ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to decrypt dek from cipher, rc: %d", rc ) ;
         ossMemcpy( plainDEK, outDEKBuf, sizeof( ossSM4Key ) ) ;

         rc = ossSM4Init( &ctx, OSS_SM4_CBC ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize SM4 cipher context, rc: %d", rc ) ;
         ctx.setKey( plainDEK ) ;

         rc = ossSM4EncryptHybrid( &ctx, (UINT8 *)UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT, outSign,
                                   outSignLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to encrypt DEK verification plain text, rc: %d", rc ) ;

         std::string encodedDekSign =
            base64::encode( std::string( (const CHAR *)outSign, outSignLen ) ) ;

         if ( strncmp( encodedDekSign.c_str(), dekSign, encodedDekSign.size() ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The dek signatures do not match,\nexpected: %s,\nactual: %s, rc: %d",
                    dekSign, encodedDekSign.c_str(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      ossMemset( plainDEK, 0, OSS_SM4_KEY_SIZE ) ;
      goto done ;
   }

   INT32 utilSecBackupToBson( const ossSM4Key plainDEK, const CHAR *mkContentPublic, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      EVP_PKEY *mkPublic = NULL ;
      UINT8 encryptedDek[ UTIL_SEC_ENCRYPTED_DEK_SZ ] = { 0 } ;
      size_t oLen = UTIL_SEC_ENCRYPTED_DEK_SZ ;
      ossSM4Context ctx ;
      const size_t outSignLen = UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
      UINT8 outSign[ outSignLen ] = { 0 } ;

      rc =
         utilSecReadMKPair( mkContentPublic, ossStrlen( mkContentPublic ), &mkPublic, TRUE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read public key, rc: %d", rc ) ;

      rc = ossSM2Encrypt( mkPublic, plainDEK, OSS_SM4_KEY_SIZE, encryptedDek, &oLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to encrypt dek, rc: %d", rc ) ;

      rc = ossSM4Init( &ctx, OSS_SM4_CBC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize SM4 cipher context, rc: %d", rc ) ;
      ctx.setKey( plainDEK ) ;

      rc = ossSM4EncryptHybrid( &ctx, (UINT8 *)UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT, outSign,
                                outSignLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to encrypt DEK verification plain text, rc: %d", rc ) ;

      try
      {
         std::string encodedDek = base64::encode( std::string( (const CHAR *)encryptedDek, oLen ) ) ;
         std::string encodedDekSign =
            base64::encode( std::string( (const CHAR *)outSign, outSignLen ) ) ;

         rc = utilSecBackupToBson( encodedDek.data(), encodedDek.size(), encodedDekSign.data(),
                                   encodedDekSign.size(), mkContentPublic,
                                   ossStrlen( mkContentPublic ), obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to backup, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      ossSM2FreeKeyPair( mkPublic ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecBackupToBson( const CHAR *dekCipher,
                              UINT32 sizeOfCipher,
                              const CHAR *dekSign,
                              UINT32 sizeOfSign,
                              const CHAR *mkContentPublic,
                              UINT32 sizeOfPub,
                              BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      EVP_PKEY *pKey = NULL ;
      BIO *bio = BIO_new( BIO_s_mem() ) ;
      if ( !bio )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to construct BIO buffer, rc: %d", rc ) ;
         goto error ;
      }

      rc = utilSecReadMKPair( mkContentPublic, sizeOfPub, &pKey, TRUE, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read public key, rc: %d", rc ) ;

      try
      {
         if ( !PEM_write_bio_PUBKEY( bio, pKey ) )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to write public key, rc: %d", rc ) ;
            goto error ;
         }

         CHAR *pubKeyStr = NULL ;
         size_t pubKeySize = BIO_get_mem_data( bio, &pubKeyStr ) ;

         BSONObjBuilder builder ;
         builder.appendStrWithNoTerminating( FIELD_NAME_DEK, dekCipher, sizeOfCipher ) ;
         builder.appendStrWithNoTerminating( FIELD_NAME_DEKVERIFICATION, dekSign, sizeOfSign ) ;
         builder.appendStrWithNoTerminating( FIELD_NAME_MK_PUBLIC, pubKeyStr, pubKeySize ) ;
         obj = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      ossSM2FreeKeyPair( pKey ) ;
      BIO_free_all( bio ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecRestoreKeyFiles( const CHAR *dbPath,
                                 const CHAR *dekCipher,
                                 const CHAR *dekSign,
                                 const CHAR *mkPublic,
                                 const CHAR *privateKeyPath,
                                 BOOLEAN tryToCreate,
                                 ossSM4Key plainDEK )
   {
      INT32 rc = SDB_OK ;
      EVP_PKEY *pKey = NULL ;
      EVP_PKEY *oldKey = NULL ;
      CHAR dekDir[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR mkDir[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      utilSecGetKeyDirs( dbPath, mkDir, dekDir ) ;

      rc = utilSecReadMKPair( mkPublic, privateKeyPath, &pKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read master key pair, rc: %d", rc ) ;

      rc = utilSecDecryptDEK( dekCipher, ossStrlen( dekCipher ), dekSign, ossStrlen( dekSign ),
                              pKey, plainDEK ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to decrypt dek by private master key, rc: %d", rc ) ;

      if ( tryToCreate )
      {
         BOOLEAN needCreateNew = FALSE ;
         CHAR mkFileName[ UTIL_SEC_MK_INDEX_FILE_BUF_SZ ] = { 0 } ;
         INT64 readLen = UTIL_SEC_MK_INDEX_FILE_BUF_SZ ;
         CHAR indexFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         CHAR mkFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         CHAR dekFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         CHAR dekSignFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

         utilSecGetFilePaths( mkDir, dekDir, indexFilePath, dekFilePath, dekSignFilePath ) ;

         rc = utilSecReadFileIntoBuf( indexFilePath, mkFileName, readLen ) ;
         if ( SDB_FNE != rc && SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read mk index file: %s, rc: %d", indexFilePath, rc ) ;
            goto error ;
         }
         else if ( SDB_FNE == rc )
         {
            PD_LOG( PDINFO, "need create new key files because mk index does not exist" ) ;
            needCreateNew = TRUE ;
         }
         else
         {
            CHAR oldMKContent[ UTIL_SEC_ENCRYPTED_MK_FILE_BUF_SZ ] = { 0 } ;
            INT64 len = UTIL_SEC_ENCRYPTED_MK_FILE_BUF_SZ ;

            utilSecGetMKPath( mkDir, mkFileName, mkFilePath ) ;

            rc = utilSecReadFileIntoBuf( mkFilePath, oldMKContent, len ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to read mk content file: %s, rc: %d", mkFilePath,
                         rc ) ;

            rc = utilSecReadMKPair( oldMKContent, len, &oldKey, TRUE, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to read mk pair, rc: %d", rc ) ;

            if ( 1 != EVP_PKEY_cmp( pKey, oldKey ) )
            {
               PD_LOG( PDINFO,
                       "need create new key files because the existing master key is defferent" ) ;
               needCreateNew = TRUE ;
            }
         }

         if ( needCreateNew )
         {
            CHAR mkTag[ UTIL_SEC_MK_TAG_STR_LEN ] = { 0 } ;

            rc = utilSecEnsureSecDirectories( mkDir, dekDir ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to ensure directories existence", rc ) ;

            rc = utilSecGenMKTagByTime( mkTag ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to generate mk tag by time, rc: %d", rc ) ;

            ossSnprintf( mkFileName, UTIL_SEC_MK_INDEX_FILE_BUF_SZ, "%s%s", mkTag,
                         UTIL_SEC_MK_FILE_SUFFIX ) ;
            ossSnprintf( mkFilePath, OSS_MAX_PATHSIZE, "%s%s%s", mkDir, OSS_FILE_SEP, mkFileName ) ;

            rc = ossSM2WriteKeyPairToFile( mkFilePath, pKey ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to write SM2 key pair to file, rc: %d", rc ) ;

            rc = utilSecCreateIndexFile( indexFilePath, mkFileName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update mk index to [%s], rc: %d", mkFileName, rc ) ;

            rc = utilSecCreateDEKFile( dekFilePath, mkTag, plainDEK, pKey ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create dek file, rc: %d", rc ) ;

            rc = utilSecReplaceDEKSignFile( dekSignFilePath, plainDEK ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create dek signature file, rc: %d", rc ) ;
         }
      }

   done:
      ossSM2FreeKeyPair( pKey ) ;
      ossSM2FreeKeyPair( oldKey ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecReplaceDEKSignFile( const CHAR *dekSignFilePath, const ossSM4Key pDEK )
   {
      INT32 rc = SDB_OK ;
      CHAR buf[ UTIL_SEC_DEK_SIGN_FILE_BUF_SZ ] = { '\0' } ;
      UINT8 encStr[ UTIL_SEC_DEK_SIGNATURE_SZ ] = { 0 } ;
      UINT8 sigBuf[ UTIL_SEC_DEK_SIGNATURE_SZ ] = { 0 } ;
      UINT8 digest[ UTIL_SEC_SM3_DIGEST_SZ ] = { 0 } ;
      UINT32 encStrLen = sizeof( encStr ), digestLen = sizeof( digest ), len = 0 ;
      BOOLEAN bFileOpened = FALSE ;
      OSSFILE dekVeriFile ;
      SINT64 writeLen = 0 ;
      string encStr64, digestStr64 ;
      ossSM4Context ctx ;

      if ( ( NULL == dekSignFilePath ) || ( NULL == pDEK ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
         goto error ;
      }

      // SM4 context init
      rc = ossSM4Init( &ctx, OSS_SM4_CBC ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize SM4 cipher context, rc: %d", rc ) ;
      ctx.setKey( pDEK ) ;

      // encrypt pre-defined DEK verification string
      encStrLen = UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
      rc = ossSM4EncryptHybrid( &ctx, (UINT8 *)UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT, encStr,
                                encStrLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to verify DEK signature, rc: %d", rc ) ;
      encStr64 = base64::encode( (CHAR *)encStr, encStrLen ) ;

      // calculate digest
      ossMemcpy( sigBuf, encStr64.c_str(), encStr64.length() ) ;
      len = encStr64.length() ;
      ossMemcpy( sigBuf + len, UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT,
                 UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ) ;
      len += UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ;
      rc = ossSM3Digest( sigBuf, len, digest, &digestLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate digest, rc: %d", rc ) ;
      digestStr64 = base64::encode( (CHAR *)digest, digestLen ) ;

      // construct the content
      ossSnprintf( buf, sizeof( buf ), "%s%s%s%s%s%s%s%s", encStr64.c_str(), OSS_NEWLINE,
                   UTIL_SEC_TAG_DIGEST, OSS_NEWLINE, digestStr64.c_str(), OSS_NEWLINE,
                   UTIL_SEC_TAG_END_OF_FILE, OSS_NEWLINE ) ;

      rc = ossOpen( dekSignFilePath, OSS_REPLACE | OSS_READWRITE | OSS_EXCLUSIVE,
                    ( OSS_RU | OSS_WU ), dekVeriFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", dekSignFilePath, rc ) ;
      bFileOpened = TRUE ;

      // write to file
      rc = ossSeek( &dekVeriFile, 0, OSS_SEEK_SET ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update file:%s, rc: %d", dekSignFilePath, rc ) ;
      rc = ossWrite( &dekVeriFile, (CHAR *)buf, ossStrlen( buf ), &writeLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", dekSignFilePath, rc ) ;
      rc = ossFsync( &dekVeriFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write to file:%s, rc: %d", dekSignFilePath, rc ) ;
   done:
      if ( bFileOpened )
      {
         ossClose( dekVeriFile ) ;
         bFileOpened = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN utilSecCompareTag( const CHAR *indexContent, const CHAR *dekContent )
   {
      const CHAR *pos1 = ossStrstr( indexContent, UTIL_SEC_MK_FILE_SUFFIX ) ;
      const CHAR *pos2 = dekContent + ossStrlen( UTIL_SEC_TAG_MKTAG_TAG ) ;
      const CHAR *pos3 = ossStrstr( dekContent, UTIL_SEC_TAG_MKTAG_END ) ;

      if ( !pos1 || !pos2 || !pos3 )
      {
         return FALSE ;
      }
      INT32 len1 = pos1 - indexContent ;
      INT32 len2 = pos3 - pos2 ;

      if ( len1 == len2 && 0 == ossMemcmp( indexContent, pos2, len1 ) )
      {
         return TRUE ;
      }
      else
      {
         return FALSE ;
      }
   }

   INT32 utilSecPrepareDEKInfoBSONObj( ossSM4Key plainDEK,
                                       const CHAR *dekSignContent,
                                       BSONObj &dekInfo )
   {
      INT32 rc = SDB_OK ;
      const CHAR *dekSign = NULL ;
      UINT32 sizeOfSign = 0 ;
      if ( !utilSecGetDEKSign( dekSignContent, &dekSign, &sizeOfSign ) )
      {
         rc = SDB_SEC_KEYS_CORRUPTED ;
         PD_LOG( PDERROR, "The dek signature is corrupted, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         builder.appendBinData( FIELD_NAME_DEK, sizeof( ossSM4Key ), BinDataGeneral, plainDEK ) ;
         builder.appendStrWithNoTerminating( FIELD_NAME_DEKVERIFICATION, dekSign, sizeOfSign ) ;
         dekInfo = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecExtractDEKFromDEKInfoBSONObj( BSONObj &dekInfo, UINT8 *pDEK, BOOLEAN &isValid )
   {
      INT32 rc = SDB_OK ;
      INT32 dekLen = 0 ;
      try
      {
         const CHAR *dek = dekInfo.getField( FIELD_NAME_DEK ).binData( dekLen ) ;
         const CHAR *dekSign = dekInfo.getStringField( FIELD_NAME_DEKVERIFICATION ) ;

         if ( OSS_SM4_KEY_SIZE != dekLen )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "The size of dek must be %d", OSS_SM4_KEY_SIZE ) ;
            goto error ;
         }

         rc = utilSecVerifyDEK( (UINT8 *)dek, dekSign, isValid ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to validate dek signature, rc: %d", rc ) ;

         ossMemcpy( pDEK, dek, sizeof( ossSM4Key ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilSecWriteKeyFileContents( const CHAR *dbPath,
                                      const CHAR *indexContent,
                                      const CHAR *mkContent,
                                      const CHAR *dekContent,
                                      const CHAR *dekSignContent )
   {
      INT32 rc = SDB_OK ;
      CHAR mkDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR indexPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR mkPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      CHAR dekSignPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;

      utilSecGetKeyDirs( dbPath, mkDir, dekDir ) ;
      utilSecGetFilePaths( mkDir, dekDir, indexPath, dekPath, dekSignPath ) ;
      utilSecGetMKPath( mkDir, indexContent, mkPath ) ;

      rc = utilSecEnsureSecDirectories( mkDir, dekDir );
      PD_RC_CHECK( rc, PDERROR, "Failed to access security directories, rc: %d", rc ) ;

      rc = utilSecWriteBufToFile( mkPath, mkContent, ossStrlen( mkContent ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write MK file, rc: %d", rc ) ;

      rc = ossAccess( dekPath ) ;
      if ( SDB_OK == rc )
      {
         rc = utilSecBackupDEKFile( dekPath ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to backup DEK file, rc: %d", rc ) ;
      }
      else if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access DEK file, rc: %d", rc ) ;
         goto error ;
      }

      rc = utilSecWriteBufToFile( dekPath, dekContent, ossStrlen( dekContent ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write DEK file, rc: %d", rc ) ;

      rc = utilSecWriteBufToFile( dekSignPath, dekSignContent, ossStrlen( dekSignContent ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write DEK signature file, rc: %d", rc ) ;

      rc = utilSecWriteBufToFile( indexPath, indexContent, ossStrlen( indexContent ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write DEK signature file, rc: %d", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

} // namespace engine
