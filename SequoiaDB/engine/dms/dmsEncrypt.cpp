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

   Source File Name = dmsEncrypt.cpp 

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/10/2023  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsEncrypt.hpp"
#include "pmdEDU.hpp"
#include "dmsRecord.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "../bson/lib/base64.h"  // base64
#include "utilSecurityKeys.hpp"

using namespace bson ;
using namespace base64 ;

namespace engine
{

void dmsInitEncVecByRID( const dmsRecordID * pRID, ossSM4Vec vec )
{
   UINT32 ridSize = sizeof( dmsRecordID ),
          vecSize = sizeof( ossSM4Vec ) ;

   SDB_ASSERT ( pRID, "Input pointer can't be NULL" ) ;
   
   ossMemset( vec, 0, vecSize ) ;

   if ( pRID )
   {
      if ( ridSize < vecSize )
      {
         ossMemcpy( vec, pRID, ridSize ) ;
      }
      else
      {
         ossMemcpy( vec, pRID, vecSize ) ; 
      }
   }
}

INT32 dmsBSONEncrypt( _pmdEDUCB *cb, ossSM4Context * ctx,
                      const CHAR *pInData, INT32 inSize,
                      const CHAR **ppData, INT32 *pDataSize )
{
   INT32 rc = SDB_OK ;
   CHAR *pBuff = NULL ;
   INT32 outSize = inSize - sizeof( INT32 ) ;

   SDB_ASSERT ( pInData && ppData && ctx && cb,
                "Input pointer can't be NULL" ) ;

   if ( ( NULL == pInData ) || ( NULL == ppData ) ||
        ( NULL == cb ) || ( NULL == ctx ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc=:%d", rc ) ;
      goto error ;
   }
   if ( inSize < 5 )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR,
              "Invalid parameter, inSize:%d, rc=:%d", inSize, rc ) ;
      goto error ;
   }

   pBuff = cb->getEncryptionBuff( ossAlign4( outSize ) ) ;
   if ( ! pBuff )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc encryption buff, size: %d", inSize ) ;
      goto error ;
   }

   rc = ossSM4EncryptHybrid( ctx, (UINT8 *)( pInData + sizeof( INT32 ) ), (UINT8 *)( pBuff ),
                             inSize - sizeof( INT32 ) ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to compress data, rc: %d", rc ) ;
      goto error ;
   }

#if defined (_DEBUG)
   {
      BSONObj dbgObj( pInData ) ;
      PD_LOG( PDWARNING, "Encrypting BSON obj:%s",
              dbgObj.toString( FALSE, TRUE ).c_str() ) ;
   }
#endif

   if ( ppData )
   {
      *ppData = pBuff ;
   }
   if ( pDataSize )
   {
      *pDataSize = outSize ;
   }

done :
   return rc ;
error :
   goto done ;
}


// caller shall make sure the key and vector are same as they
// were used while encrypting.
//
// As mentioned above, when encrypt we skip off the first 4 bytes, so when
// decrypt we start from pInputData + sizeof(int).
INT32 dmsBSONDecrypt( _pmdEDUCB *cb, ossSM4Context * ctx,
                      const CHAR *pInData, INT32 inSize,
                      const CHAR **ppData, INT32 *pDataSize )
{

   INT32 rc = SDB_OK ;
   CHAR *pBuff = NULL ;
   INT32 outSize = inSize + sizeof(INT32) ;

   SDB_ASSERT ( pInData && ppData && ctx && cb,
                "Input pointer can't be NULL" ) ;

   if ( ( NULL == pInData ) || ( NULL == ppData ) ||
        ( NULL == cb ) || ( NULL == ctx ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc=:%d", rc ) ;
      goto error ;
   }
   if ( inSize < 5 )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR,
              "Invalid parameter, inSize:%d, rc=:%d", inSize, rc ) ;
      goto error ;
   }

   pBuff = cb->getDecryptionBuff( ossAlign4( outSize ) ) ;
   if ( ! pBuff )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc decryption buff, size: %d", inSize ) ;
      goto error ;
   }

   // skip first 4 bytes, which contains the length of a BSON obj
   *(INT32 *)( pBuff ) = outSize ;

   rc = ossSM4DecryptHybrid( ctx, (UINT8 *)( pInData ), (UINT8 *)( pBuff + sizeof( INT32 ) ),
                             inSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to compress data, rc: %d", rc ) ;
      goto error ;
   }

#if defined (_DEBUG)
   {
      BSONObj dbgObj( pBuff ) ;
      PD_LOG( PDDEBUG, "Decrypted BSON Obj:%s", dbgObj.toString( FALSE, TRUE ).c_str() ) ;
      SDB_ASSERT( dbgObj.valid(), "Invalid BSON" ) ;
   }
#endif

   if ( ppData )
   {
      *ppData = pBuff ;
   }
   if ( pDataSize )
   {
      *pDataSize = outSize ;
   }

done :
   return rc ;
error :
   goto done ;
}


INT32 dmsSecReadFileIntoBuf( const CHAR * fileName, CHAR * pBuf, INT64 & len )
{
   INT32 rc = SDB_OK;
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
         PD_LOG( PDERROR, "Failed to access file:%s, rc: %d", fileName, rc );
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
      PD_LOG( PDERROR, "Failed to access file:%s, rc: %d",
              fileName, rc );
      goto error ;
   }
   if ( fileSz > len )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "File size:%lld is larger than buffer size:%lld, rc: %d",
              fileSz, len, rc ) ;
      goto error ;
   }
   // read from file
   rc = ossOpen( fileName, OSS_READONLY, 0, osFile ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to open file:%s, rc: %d", fileName, rc ) ;
   bFileOpened = TRUE ;
   rc = ossSeek( &osFile, 0, OSS_SEEK_SET ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to access file:%s, rc: %d", fileName, rc );
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


INT32 dmsSecGetKeysFromMKFile( const CHAR * mkFullPathName,
                               EVP_PKEY ** pPubKey,
                               EVP_PKEY ** pPriKey )
{
   INT32 rc = SDB_OK ;

   if ( NULL == mkFullPathName )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error;
   }
   if ( ( NULL == pPubKey ) && ( NULL == pPriKey ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error;
   }

   rc = ossAccess( mkFullPathName, OSS_MODE_READ ) ;
   PD_RC_CHECK( rc, PDERROR,
                "Failed to access file:%s, rc: %d", mkFullPathName, rc ) ;

   // read private key from MK file name
   if ( pPriKey )
   {
      rc = ossSM2ReadPrivateKeyFromFile( mkFullPathName, pPriKey );
      PD_RC_CHECK( rc, PDERROR, "Failed to read key from file:%s, rc: %d",
                   mkFullPathName, rc ) ;
   }
   if ( pPubKey )
   {
      rc = ossSM2ReadPublickKeyFromFile( mkFullPathName, pPubKey ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read key from file:%s, rc: %d",
                   mkFullPathName, rc ) ;
   }
done:
   return rc ;
error:
   goto done ;
}


#define DMS_SEC_ENCRYPTED_DEK_SZ ( 128 )
static INT32 _dmsSecGetDekFromBuf( const CHAR * mkFullPathName,
                                   CHAR * pBuf,
                                   UINT8 * pDEK )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY * pSM2PriKey = NULL ;
   CHAR buf[ DMS_SEC_ENCRYPTED_DEK_SZ ] = { '\0' } ; // base64 encoded
   UINT8 encDEK[ DMS_SEC_ENCRYPTED_DEK_SZ ] = { 0 }; // encrypted by SM2 pub key
   CHAR * pos1 = NULL, * pos2 = NULL ;
   size_t dekLen = OSS_SM4_KEY_SIZE ;
   UINT16 ossNwlnLen = ossStrlen( OSS_NEWLINE ) ;
   string dekStr ;

   if ( ( NULL == mkFullPathName ) || ( NULL == pBuf ) || ( NULL == pDEK ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error ;
   }

   // read private key from MK file
   rc = dmsSecGetKeysFromMKFile( mkFullPathName, NULL, &pSM2PriKey ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to read key from MK file:%s, rc: %d",
                mkFullPathName, rc ) ;

   pos1 = ossStrstr( pBuf, UTIL_SEC_TAG_ENCRYPTED_DEK ) ;
   pos2 = ossStrstr( pBuf, UTIL_SEC_TAG_DIGEST ) ;
   if ( ( NULL == pos1 ) || ( NULL == pos2 ) )
   {
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Invalid input, rc: %d", rc ) ;
      goto error ;
   }

   // get base64 encoded dek string
   ossMemcpy( buf, pos1 + ossStrlen( UTIL_SEC_TAG_ENCRYPTED_DEK ) + ossNwlnLen,
              pos2 - pos1 - ossStrlen( UTIL_SEC_TAG_ENCRYPTED_DEK ) - ossNwlnLen
              - ossNwlnLen ) ;
   // decode and decrypt
   dekStr = base64::decode( string( buf ) ) ;
   ossMemcpy( encDEK, dekStr.c_str(), dekStr.length() ) ;
   rc = ossSM2Decrypt( pSM2PriKey, encDEK, dekStr.length(), pDEK, &dekLen ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to decrypt DEK, rc: %d", rc ) ;
   if ( OSS_SM4_KEY_SIZE != dekLen )
   {
      rc = SDB_SYS ;
      PD_LOG( PDERROR, "Failed to decrypt DEK, rc: %d", rc ) ;
      goto error ;
   }
done:
   ossSM2FreeKeyPair( pSM2PriKey ) ;
   return rc ;

error:
   goto done ;
}


static INT32 _dmsSecGetIndexFileContent( const CHAR * mkPath, CHAR * pContent )
{
   INT32 rc = SDB_OK;
   CHAR buf[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
   CHAR idxFileName[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
   INT64 readed = sizeof( buf ) ;

   if ( ( NULL == mkPath ) || ( NULL == pContent ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error ;
   }

   // index file name
   ossSnprintf( idxFileName, sizeof( idxFileName ), "%s%s%s",
                mkPath, OSS_FILE_SEP, UTIL_SEC_INDEX_FILE_NAME ) ;

   rc = ossAccess( idxFileName ) ;
   if ( SDB_OK != rc )
   {
      if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to access file:%s, rc: %d",
                 idxFileName, rc );
         goto error ;
      }
      else
      {
         pContent[0] = '\0' ;
         goto done ;
      }
   }
   // read from index file
   rc = dmsSecReadFileIntoBuf( idxFileName, buf, readed ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to read file:%s, rc: %d", idxFileName, rc ) ;
      goto error ;
   }
   //
   if ( readed && ( '\0' != buf[0] ) )
   {
      ossSnprintf( pContent, ossStrlen(buf) + ossStrlen(OSS_FILE_SEP) + 1,
                   "%s", buf ) ;
   }
done:
   return rc ;
error:
   goto done ;
}


INT32 dmsSecGetMKFileNameFromIndex( const CHAR * mkPath, CHAR * pMKFileName )
{
   INT32 rc = SDB_OK ;
   CHAR buf[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;

   if ( ( NULL == mkPath ) || ( NULL == pMKFileName ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error ;
   }

   rc = _dmsSecGetIndexFileContent( mkPath, buf ) ;
   if ( SDB_OK == rc )
   {
      if ( '\0' != buf[0] )
      {
         // mkPath contains '/' at the end
         ossSnprintf( pMKFileName,
                      ossStrlen( mkPath ) + ossStrlen( buf ) + 1,
                      "%s%s", mkPath, buf ) ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid index file, rc: %d", rc ) ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}


INT32 dmsSecGetDekFromDEKFile( const CHAR * mkFullPathName,
                               const CHAR * dekFullPathName,
                               UINT8 * pDEK )
{
   INT32 rc = SDB_OK ;
   CHAR buf[ UTIL_SEC_DEK_FILE_BUF_SZ  ] = { '\0' } ;
   INT64 readed = sizeof( buf ) ;

   if ( ( NULL == mkFullPathName ) || ( NULL == dekFullPathName ) ||
        ( NULL == pDEK ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error ;
   }

   rc = dmsSecReadFileIntoBuf( dekFullPathName, buf, readed ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to read file:%s, rc: %d", dekFullPathName, rc ) ;
      goto error ;
   }
   rc = _dmsSecGetDekFromBuf( mkFullPathName, buf, pDEK ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get DEK, rc: %d", rc ) ;

done:
   return rc ;

error:
   goto done ;
}


INT32 dmsSecGetDEK( const CHAR * securityPath, UINT8 * pDEK )
{
   INT32 rc = SDB_OK ;
   CHAR mkDir [ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
   CHAR dekDir[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
   CHAR dekFileName[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
   CHAR mkFileName[ OSS_MAX_PATHSIZE + 1 ]  = { '\0' } ;

   if ( ( NULL == securityPath ) || ( NULL == pDEK ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc: %d", rc ) ;
      goto error ;
   }

   // construct full path name of mkDir, dekDir
   ossSnprintf( mkDir, sizeof( mkDir ), "%s%s%s%s",
                securityPath, OSS_FILE_SEP,
                UTIL_SEC_MK_DIR_NAME, OSS_FILE_SEP ) ;
   ossSnprintf( dekDir, sizeof( dekDir ), "%s%s%s%s",
                securityPath, OSS_FILE_SEP,
                UTIL_SEC_DEK_DIR_NAME, OSS_FILE_SEP ) ;

   // construct full path name of DEK file
   ossSnprintf( dekFileName, sizeof( dekFileName ), "%s%s",
                dekDir, UTIL_SEC_DEK_FILE_NAME ) ;
   
   rc = dmsSecGetMKFileNameFromIndex( mkDir, mkFileName );
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to access MK index file, rc:%d", rc ) ;
      goto error ;
   }

   rc = dmsSecGetDekFromDEKFile( mkFileName, dekFileName, pDEK ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to get DEK from file[%s], rc: %d", dekFileName, rc ) ;

done:
   return rc ;

error:
   goto done ;
}

INT32 dmsBinEncrypt( _pmdEDUCB *cb, ossSM4Context * ctx,
                      const CHAR *pInData, INT32 inSize,
                      const CHAR **ppData, INT32 *pDataSize )
{
   INT32 rc = SDB_OK;
   CHAR *pBuff = NULL ;

   SDB_ASSERT ( pInData && ppData && ctx && cb,
                "Input pointer can't be NULL" ) ;

   if ( ( NULL == pInData ) || ( NULL == ppData ) ||
        ( NULL == cb ) || ( NULL == ctx ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc=:%d", rc ) ;
      goto error ;
   }
   
   pBuff = cb->getEncryptionBuff( ossAlign4( inSize ) ) ;
   if ( ! pBuff )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc encryption buff, size: %d", inSize ) ;
      goto error ;
   }

   rc = ossSM4EncryptHybrid( ctx, (UINT8 *)( pInData ), (UINT8 *)( pBuff ), inSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to compress data, rc: %d", rc ) ;
      goto error ;
   }

   if ( ppData )
   {
      *ppData = pBuff ;
   }
   if ( pDataSize )
   {
      *pDataSize = inSize ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 dmsBinDecrypt( _pmdEDUCB *cb, ossSM4Context * ctx,
                     const CHAR *pInData, INT32 inSize,
                     const CHAR **ppData, INT32 *pDataSize )
{

   INT32 rc = SDB_OK ;
   CHAR *pBuff = NULL ;

   SDB_ASSERT ( pInData && ppData && ctx && cb,
                "Input pointer can't be NULL" ) ;

   if ( ( NULL == pInData ) || ( NULL == ppData ) ||
        ( NULL == cb ) || ( NULL == ctx ) )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR, "Input parameter can't be NULL, rc=:%d", rc ) ;
      goto error ;
   }

   pBuff = cb->getDecryptionBuff( ossAlign4( inSize ) ) ;
   if ( ! pBuff )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to alloc decryption buff, size: %d", inSize ) ;
      goto error ;
   }

   rc = ossSM4DecryptHybrid( ctx, (UINT8 *)( pInData ), (UINT8 *)( pBuff ), inSize ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to compress data, rc: %d", rc ) ;
      goto error ;
   }

   if ( ppData )
   {
      *ppData = pBuff ;
   }
   if ( pDataSize )
   {
      *pDataSize = inSize ;
   }

done :
   return rc ;
error :
   goto done ;
}

} // namespace engine

