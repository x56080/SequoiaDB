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

   Source File Name = utilSecurityKeys.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/02/2023  JT  Initial Draft
          04/11/2023  ZHY Move to util

   Last Changed =

*******************************************************************************/
#ifndef UTIL_SECURITY_KEYS_HPP_
#define UTIL_SECURITY_KEYS_HPP_

#include "core.hpp"
#include "../bson/bson.h"
#include "openssl/ec.h"
#include "ossGMCrypto.hpp"

using namespace bson ;

namespace engine
{

/*
   MK, DEK creation and verification
 */
#define UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT "DEK$ignature#20230121@sequoiaDB"
#define UTIL_SEC_DEK_VERIFICATION_PLAIN_TXT_SZ ( 31 )
#define UTIL_SEC_DEK_SIGNATURE_SZ ( 128 )
#define UTIL_SEC_CIPHER_NAME_SM4_STR "SM4"

#define UTIL_SEC_SECURITY_DIR_NAME "security"
#define UTIL_SEC_MK_DIR_NAME "MK"
#define UTIL_SEC_DEK_DIR_NAME "DEK"
// mkyyyymmddhhmmssmmmmmm MK Tag foramt
#define UTIL_SEC_MK_TAG_STR_LEN ( 24 )
#define UTIL_SEC_DEK_FILE_NAME "dek"
#define UTIL_SEC_DEK_SIGN_FILE_NAME "dek.sign"
#define UTIL_SEC_INDEX_FILE_NAME "index"
#define UTIL_SEC_MK_FILE_SUFFIX ".pem"
#define UTIL_SEC_FILE_SUFFIX_BAK ".bak"

#define UTIL_SEC_TAG_MKTAG_TAG "Tag["
#define UTIL_SEC_TAG_MKTAG_END "]"
#define UTIL_SEC_TAG_ENCRYPTED_DEK "-------- Encrypted DEK --------"
#define UTIL_SEC_TAG_DIGEST "-------- Digest --------"
#define UTIL_SEC_TAG_END_OF_FILE "-------- End --------"

#define UTIL_SEC_ENCRYPTED_MK_FILE_BUF_SZ ( 1024 )
#define UTIL_SEC_MK_INDEX_FILE_BUF_SZ ( 32 )
#define UTIL_SEC_DEK_FILE_BUF_SZ ( 1024 )
#define UTIL_SEC_DEK_SIGNATURE_FILE_BUF_SZ ( 256 )
#define UTIL_SEC_SM3_DIGEST_SZ ( 64 )

   /** Remove security directory recursively
    *
    *  \param [in] dbpath:  a NULL terminated string, contains dbpath,
    *                       ususally pmdGetOptionCB()->getDbPath()
    *  \return SDB_OK on success,
    *          or other error code on I/O or permission problem
    */
   INT32 utilSecRemoveSecDirectories( const CHAR *dbpath ) ;

   /** Get MK tag from a DEK file
    *  \param [in] dekFullPathName : full path name of the DEK file,
    *                                NULL termianted string
    *  \param [out] pMKTag         : a pointer to a buffer, contains MK tag,
    *                                caller shall make sure the buffer is large
    *                                enough.
    *  \return SDB_OK on success
    */
   INT32 utilSecGetMKTagFromDEKFile( const CHAR *dekFullPathName, CHAR *pMKTag ) ;

   /** Backup DEK( Data Encryption Key ) file by rename, for example
    *     dbpath/security/DEK/dek, after backup, it is renamed as
    *     dbpath/security/DEK/dek.mk20230208000753
    *  where the "mk20230208000753" is the MK tag contained in that dek file
    *
    *  \param [in] fileName: full path name of DEK file, a NULL terminated string
    *  \return SDB_OK success,
    *          or other error code on I/O or permission problem
    */
   INT32 utilSecBackupDEKFile( const CHAR *fileName ) ;

   /** Read whole file content into buffer
    *  \param [in] fileName: full path name of a file. It is a NULL terminated
    *                        string
    *  \param [in,out] pBuf: a pointer of the buffer, contains the whole file,
    *                        the caller shall make sure the buffer is large enough
    *  \param [in,out] len:  it represents a buffer size as input param, and
    *                        it also represents number of bytes readed when return
    *  \return SDB_OK success
    */
   INT32 utilSecReadFileIntoBuf( const CHAR *fileName, CHAR *pBuf, INT64 &len ) ;

   /** Flush buf to file, the file will be over written if it already exists.
    *  \param [in] fileNm : full path name of a file, a NULL terminated string
    *  \param [in] pBuf   : a pointer to the input buffer
    *  \param [in] len    : the length of the input buffer
    *  \return SDB_OK success
    */
   INT32 utilSecWriteBufToFile( const CHAR *fileNm, const CHAR *pBuf, INT64 len );

   /** Create MKTag by local time
    * \param [out] pMKTag: a pointer to a string buffer. It is a NULL terminated
    *                      string, as unique identifier of a MK( Mater Key ) file.
    *                      It looks like mkYYYYMMDDMMHHSS, for example,
    *                      "mk20230208000852"
    * \return SDB_OK success
    */
   INT32 utilSecGenMKTagByTime( CHAR *pMKTag ) ;

   /** Verify if a DEK is the correct one
    *  \param [in] pDEK        : a pointer to a buffer containing the DEK, 16Bytes
    *  \param [in] dekSignature: the verification string, normall yreaded from
    *                            the DEK verification file.
    *                            NULL termianted string
    *  \param [out] result     : boolean, result of the validation of this DEK
    *  \return SDB_OK on success
    */
   INT32 utilSecVerifyDEK( const ossSM4Key pDEK, const CHAR *dekSignature, BOOLEAN &result ) ;

   void utilSecGetKeyDirs( const CHAR *dbPath, CHAR *mkDir, CHAR *dekDir ) ;

   void utilSecGetFilePaths( const CHAR *mkDir,
                             const CHAR *dekDir,
                             CHAR *indexFilePath,
                             CHAR *dekFilePath,
                             CHAR *dekSignFilePath ) ;

   void utilSecGetMKName( const CHAR *mkTag, CHAR *mkFileName ) ;

   void utilSecGetMKPath( const CHAR *mkDir, const CHAR *mkFileName, CHAR *mkPath ) ;

   INT32 utilSecEnsureSecDirectories( const CHAR *mkDir, const CHAR *dekDir ) ;

   INT32 utilSecCreateIndexFile( const CHAR *indexFilePath, const CHAR *mkFileName ) ;

   INT32 utilSecCreateMKFile( const CHAR *mkFilePath, EVP_PKEY *pKey ) ;

   INT32 utilSecCreateDEKFile( const CHAR *dekFilePath,
                               const CHAR *mkTag,
                               ossSM4Key plainDEK,
                               EVP_PKEY *pKey ) ;

   INT32 utilSecCreateDEKSignFile( const CHAR *deSignFilePath, ossSM4Key plainDEK ) ;

   INT32 utilSecCreateKeyFiles( const CHAR *dbPath ) ;

   INT32 utilSecReadMKPair( const CHAR *mkContent,
                            UINT32 size,
                            EVP_PKEY **ppKey,
                            BOOLEAN readPublic,
                            BOOLEAN readPrivate ) ;

   INT32 utilSecReadMKPair( const CHAR *mkPubContent, const CHAR *mkPrivPath, EVP_PKEY **ppKey ) ;

   BOOLEAN utilSecGetDEKCipher( const CHAR *dekContent, const CHAR **dekCipher, UINT32 *size ) ;

   BOOLEAN utilSecGetDEKSign( const CHAR *dekSignContent, const CHAR **dekSign, UINT32 *size ) ;

   BOOLEAN utilSecCompareTag( const CHAR *indexContent, const CHAR *dekContent ) ;

   INT32 utilSecDecryptDEK( const CHAR *dekCipher,
                            const UINT32 sizeOfCipherDek,
                            const CHAR *dekSign,
                            const UINT32 sizeOfDekSign,
                            EVP_PKEY *mk,
                            ossSM4Key plainDEK ) ;

   INT32 utilSecBackupToBson( const ossSM4Key plainDEK, const CHAR *mkContentPublic, BSONObj &obj ) ;

   INT32 utilSecBackupToBson( const CHAR *dekCipher,
                              UINT32 sizeOfCipher,
                              const CHAR *dekSign,
                              UINT32 sizeOfSign,
                              const CHAR *mkContent,
                              UINT32 sizeOfPub,
                              BSONObj &obj ) ;

   INT32 utilSecRestoreKeyFiles( const CHAR *dbPath,
                                 const CHAR *dekCipher,
                                 const CHAR *dekSign,
                                 const CHAR *mkPublic,
                                 const CHAR *privateKeyPath,
                                 BOOLEAN tryToCreate,
                                 ossSM4Key plainDEK ) ;

   INT32 utilSecReplaceDEKSignFile( const CHAR *dekPath, const ossSM4Key pDEK ) ;

   INT32 utilSecPrepareDEKInfoBSONObj( ossSM4Key plainDEK,
                                       const CHAR *dekSignContent,
                                       BSONObj &dekInfo ) ;

   INT32 utilSecExtractDEKFromDEKInfoBSONObj( bson::BSONObj &dekInfo,
                                              UINT8 *pDEK,
                                              BOOLEAN &isValid ) ;

   INT32 utilSecWriteKeyFileContents( const CHAR *dbPath,
                                      const CHAR *indexContent,
                                      const CHAR *mkContent,
                                      const CHAR *dekContent,
                                      const CHAR *dekSignContent );
} // namespace engine

#endif
