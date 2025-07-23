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

   Source File Name = ossGMCrypto.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2023  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_GM_CRYPTO_H_
#define OSS_GM_CRYPTO_H_

#include "core.h"
#include "ossTypes.h"
#include "oss.h"
#include "ossUtil.h"
#include "openssl/evp.h"
#include "openssl/ec.h"
#include <string>

/*
    SM3 digest
*/
INT32 ossSM3Digest( const UINT8 * msg, size_t msgLen,
                    UINT8 * digest, UINT32 * digestLen ) ;

/*
   SM4
*/

/* SM4 block size is 16Bytes */
#define OSS_SM4_BLOCK_SIZE   ( 16  )
#define OSS_SM4_KEY_SIZE     OSS_SM4_BLOCK_SIZE 
#define OSS_SM4_BLOCK_BITS   ( 128 )
#define OSS_SM4_KEY_BITS     ( 128 )

/* SM4 encryption operation mode */
enum OSS_SM4_ENCYPT_OP_MODE
{
   OSS_SM4_ECB = 0,  /* EVP_sm4_ecb() */
   OSS_SM4_CBC,      /* EVP_sm4_cbc() */
   OSS_SM4_CFB,      /* EVP_sm4_cfb() */
   OSS_SM4_OFB,      /* EVP_sm4_ofb() */
   OSS_SM4_CTR       /* EVP_sm4_ctr() */

   // EVP_sm4_gcm, EVP_sm4_ccm are not available in openssl 1.1.1.o
   // OSS_SM4_GCM,      /* EVP_sm4_gcm() */
   // OSS_SM4_CCM       /* EVP_sm4_ccm() */
} ;

/* Both the length of SM4 key and initial vector are 128bits */
typedef UINT8 ossSM4Key[ OSS_SM4_KEY_SIZE ] ;
#define ossSM4Vec ossSM4Key

class _ossSM4Context
{
public:
   EVP_CIPHER_CTX * ctx ;     /* EVP_CIPHER_CTX *, cipher context  */
   ossSM4Key        key ;     /* key for encryption and decryption */
   ossSM4Vec        iv  ;     /* initial vector                    */
   const EVP_CIPHER * cipher; /* EVP_CIPHER *, e.g, EVP_sm4_cbc()  */
public:
   _ossSM4Context()
   : ctx( NULL )
   {
      ossMemset( key, 0, sizeof( ossSM4Key ) ) ;
      ossMemset( iv,  0, sizeof( ossSM4Vec ) ) ;
      cipher = EVP_sm4_cbc() ;
   }

   _ossSM4Context( OSS_SM4_ENCYPT_OP_MODE mode ) ;

   INT32 setKey( const ossSM4Key iKey ) ; 
   INT32 setVec( const ossSM4Vec iVec ) ;

   virtual ~ _ossSM4Context()
   {
      if ( ctx )
      {
         EVP_CIPHER_CTX_free( ctx ) ;
         ctx = NULL ;
      }
      cipher = NULL ;
   }
} ;
typedef _ossSM4Context ossSM4Context ; 

/*
   Generate a random number ( 128bits, a BIGNUM ), which can be served as
   SM4 encryption key
*/
INT32 ossSM4GetRand128( UINT8 * out, UINT32 len ) ;


INT32 ossSM4Encrypt( ossSM4Context * pctx, 
                     const UINT8 * in, UINT32 iLen,
                     UINT8 * out, UINT32 * oLen ) ;

INT32 ossSM4Decrypt( ossSM4Context * pctx, 
                     const UINT8 * in, UINT32 iLen,
                     UINT8 * out, UINT32 * oLen ) ;

INT32 ossSM4EncryptHybrid( ossSM4Context * pctx,
                           const UINT8 * in, UINT8 * out, size_t len ) ; 

INT32 ossSM4DecryptHybrid( ossSM4Context * pctx,
                           const UINT8 * in, UINT8 * out, size_t len ) ; 

INT32 ossSM4Init( ossSM4Context * pctx, OSS_SM4_ENCYPT_OP_MODE mode ) ;


/*
   SM2
*/
INT32 ossSM2GenKeyPair( EVP_PKEY ** pSM2Key ) ;
void ossSM2FreeKeyPair( EVP_PKEY * pSM2Key ) ;

INT32 ossSM2EstimateEncryptionSize( EVP_PKEY *   pkey,
                                    const UINT8 * in,  size_t iLen,
                                    size_t * oLen ) ;

INT32 ossSM2EstimateDecryptionSize( EVP_PKEY *   pkey,
                                    const UINT8 * in,  size_t iLen,
                                    size_t * oLen ) ;

INT32 ossSM2EstimateSignatureSize( EVP_PKEY *   pkey,
                                   const UINT8 * in,  size_t iLen,
                                   size_t * oLen ) ;

INT32 ossSM2Encrypt( EVP_PKEY *   pkey,
                     const UINT8 * in,  size_t iLen,
                     UINT8 * out, size_t * oLen ) ;

INT32 ossSM2Decrypt( EVP_PKEY *   pkey,
                     const UINT8 * in,  size_t iLen,
                     UINT8 * out, size_t * oLen ) ;

INT32 ossSM2Sign( EVP_PKEY *   pkey,
                  const UINT8 * in,  size_t iLen,
                  UINT8 * out, size_t * oLen ) ;

INT32 ossSM2Verify( EVP_PKEY * pkey,
                    const UINT8 * signature, size_t signatureLen,
                    const UINT8 * message,   size_t messageLen,
                    BOOLEAN & result ) ;

#define OSS_SM2_KEYFILE_PASSPHRASE  "202301@SequoiaDB" 
INT32 ossSM2WriteKeyPairToFile( const CHAR * fileName,
                                EVP_PKEY * pSM2KeyPair ) ;
INT32 ossSM2WritePublicKeyToFile( const CHAR * fileName,
                                  EVP_PKEY * pSM2PublicKey ) ;
INT32 ossSM2WritePrivateKeyToFile( const CHAR * fileName,
                                   EVP_PKEY * pSM2PrivateKey ) ;
INT32 ossSM2ReadPublickKeyFromFile( const CHAR * fileName,
                                    EVP_PKEY ** pSM2PublicKey ) ;
INT32 ossSM2ReadPrivateKeyFromFile( const CHAR * fileName,
                                    EVP_PKEY ** pSM2PrivateKey ) ;

INT32 ossSM2WriteKeyPairToStrings( EVP_PKEY * pSM2KeyPair,
                                   std::string & priKey,
                                   std::string & pubKey ) ;

INT32 ossSM2WritePublicKeyToString( EVP_PKEY * pSM2PublicKey,
                                    std::string & priKey ) ;

INT32 ossSM2WritePrivateKeyToString( EVP_PKEY * pSM2PrivateKey,
                                     std::string & pubKey ) ;

INT32 ossSM2ReadPrivateKeyFromString( const std::string & privateKey,
                                      EVP_PKEY ** pSM2PrivateKey ) ;

INT32 ossSM2ReadPublicKeyFromString( const std::string & publicKey,
                                      EVP_PKEY ** pSM2PublicKey ) ; 

#endif
