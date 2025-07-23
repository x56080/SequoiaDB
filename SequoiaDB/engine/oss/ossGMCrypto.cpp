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

   Source File Name = ossGMCrypto.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2023  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossGMCrypto.hpp"
#include "openssl/bn.h"
#include "openssl/bio.h"
#include "openssl/pem.h"
#include "openssl/err.h"

using std::string ;

INT32 ossSM3Digest( const UINT8 * msg, size_t msgLen,
                    UINT8 * digest, UINT32 * digestLen )
{
   INT32 rc = SDB_OK ;

   EVP_MD_CTX *md_ctx = NULL ;

   if ( ( NULL == digest ) || ( NULL == digestLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   md_ctx = EVP_MD_CTX_new() ;
   if ( NULL == md_ctx )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( ! EVP_DigestInit_ex( md_ctx, EVP_sm3(), NULL ) )
   {
      rc = SDB_SYS ;
      goto error ;
   } 

   if ( ! EVP_DigestUpdate( md_ctx, msg, msgLen ) )
   {
      rc = SDB_SYS ;
      goto error ;
   } 

   if ( ! EVP_DigestFinal_ex( md_ctx, digest, digestLen ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( NULL != md_ctx )
   {
      EVP_MD_CTX_free( md_ctx ) ;
   }
   return rc ;
error:
   goto exit ;
}


// SM4
INT32 ossSM4GetRand128( UINT8 * out, UINT32 len )
{
   INT32 rc = SDB_OK ;
   BIGNUM * rnd = NULL ;

   if ( ( NULL == out ) || ( OSS_SM4_KEY_SIZE > len ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rnd = BN_new() ;
   if ( NULL == rnd )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( ! BN_rand( rnd, OSS_SM4_KEY_BITS, 1, 1 ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( OSS_SM4_KEY_SIZE != BN_bn2binpad( rnd, out, OSS_SM4_KEY_SIZE ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( rnd )
   {
      BN_free( rnd ) ;
      rnd = NULL ;
   }
   return rc ;
error:
   goto exit ;
}


static const EVP_CIPHER * _ossSM4GetMode( OSS_SM4_ENCYPT_OP_MODE mode )
{
   switch ( mode )
   {
      case OSS_SM4_ECB :
           return EVP_sm4_ecb();
      case OSS_SM4_CBC :
           return EVP_sm4_cbc();
      case OSS_SM4_CFB :
           return EVP_sm4_cfb();
      case OSS_SM4_OFB :
           return EVP_sm4_ofb();
      case OSS_SM4_CTR :
           return EVP_sm4_ctr();
#if 0
      // EVP_sm4_gcm, EVP_sm4_ccm are not avaialble in openssl 1.1.1.o
      case OSS_SM4_GCM :
           return EVP_sm4_gcm();
      case OSS_SM4_CCM :
           return EVP_sm4_ccm();
#endif
      default:
         return NULL ;
   }
   return NULL ;
}

_ossSM4Context::_ossSM4Context( OSS_SM4_ENCYPT_OP_MODE mode )
{
   ctx    = NULL ;
   cipher = _ossSM4GetMode( mode ) ;
   ossMemset( key, 0, sizeof( ossSM4Key ) ) ;
   ossMemset( iv,  0, sizeof( ossSM4Vec ) ) ;
}

INT32 _ossSM4Context::setKey( const ossSM4Key iKey )
{
   if ( NULL == iKey )
   {
      return SDB_INVALIDARG ;
   }
   ossMemcpy( key, iKey, sizeof( ossSM4Key ) ) ;
   return SDB_OK ; 
}


INT32 _ossSM4Context::setVec( const ossSM4Vec iVec )
{
   if ( NULL == iVec )
   {
      return SDB_INVALIDARG ;
   }
   ossMemcpy( iv, iVec, sizeof( ossSM4Vec ) ) ;
   return SDB_OK ;
}

INT32 ossSM4Init( ossSM4Context * pCtx, OSS_SM4_ENCYPT_OP_MODE mode )
{
   INT32 rc = SDB_OK ;
   EVP_CIPHER_CTX * ctx = NULL ;

   if ( NULL == pCtx )
   {  
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // if ( ( OSS_SM4_ECB > mode ) || ( OSS_SM4_CCM < mode ) )
   if ( ( OSS_SM4_ECB > mode ) || ( OSS_SM4_CTR < mode ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ctx = EVP_CIPHER_CTX_new();
   if ( NULL == ctx )
   {
      rc = SDB_OOM ; 
      goto error ;
   }
   EVP_CIPHER_CTX_init( ctx ) ;

   pCtx->ctx    = ctx ;
   pCtx->cipher = _ossSM4GetMode( mode ) ;

exit :
   return rc ;

error :
  if ( NULL != ctx ) 
  {
     EVP_CIPHER_CTX_free( ctx ) ;
  }
  goto exit ;
}


INT32 ossSM4Encrypt( ossSM4Context * pCtx,
                     const UINT8 * in,  UINT32 iLen,
                     UINT8 * out, UINT32 * oLen )
{
   INT32 rc = SDB_OK ;
   INT32 len1 = 0, len2 = 0, padding = 0 ;

   if ( ( NULL == pCtx ) || ( NULL == out ) || ( NULL == in ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( ( NULL == pCtx->ctx ) || ( NULL == pCtx->cipher ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   EVP_EncryptInit_ex( pCtx->ctx, pCtx->cipher, NULL, pCtx->key, pCtx->iv ) ;     

   if ( 0 != ( iLen % OSS_SM4_BLOCK_SIZE ) )
   {
      padding = 1;
   }
   EVP_CIPHER_CTX_set_padding( pCtx->ctx, padding ) ;

   if ( ! EVP_EncryptUpdate( pCtx->ctx, out, &len1, in, iLen ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( NULL != oLen )
   {
      *oLen = len1 ;
   }

   if ( ! EVP_EncryptFinal_ex( pCtx->ctx, out + len1, &len2 ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( NULL != oLen )
   {
      *oLen += len2 ;
   }

exit:
   return rc ;

error:
   goto exit ;
}


INT32 ossSM4Decrypt( ossSM4Context * pCtx,
                     const UINT8 * in,  UINT32 iLen,
                     UINT8 * out, UINT32 * oLen )
{
   INT32 rc = SDB_OK ;
   INT32 len1 = 0, len2 = 0 ;

   if ( ( NULL == pCtx ) || ( NULL == out ) || ( NULL == in ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( ( NULL == pCtx->ctx ) || ( NULL == pCtx->cipher ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   EVP_DecryptInit_ex( pCtx->ctx, pCtx->cipher, NULL, pCtx->key, pCtx->iv ) ;     
   EVP_CIPHER_CTX_set_padding( pCtx->ctx, 0 );

   if ( ! EVP_DecryptUpdate( pCtx->ctx, out, &len1, in, iLen ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( NULL != oLen )
   {
      *oLen = len1 ;
   }

   if ( ! EVP_DecryptFinal_ex( pCtx->ctx, out + len1, &len2 ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( NULL != oLen )
   {
      *oLen += len2 ;
   }

exit:
   return rc ;

error:
   goto exit ;
}

#define OSS_SM4_CIPHER_CHUNK_SIZE ( 1024 )
#define OSS_SM4_CIPHER_SGMNT_SIZE ( 128  )
#define OSS_SM4_CIPHER_BLOCK_SIZE OSS_SM4_BLOCK_SIZE
INT32 ossSM4EncryptHybrid( ossSM4Context * pCtx,
                           const UINT8 * in, UINT8 * out, size_t len )
{
   INT32 rc = SDB_OK ;
   size_t offset = 0, len1 = len ;
   INT32 len2 = 0 ;
   UINT8 chunkBuf[ OSS_SM4_CIPHER_CHUNK_SIZE ] = { 0 },
         segmtBuf[ OSS_SM4_CIPHER_SGMNT_SIZE ] = { 0 },
         blockBuf[ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 },
         vctrBuf [ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 },
         dataBuf [ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 };
   BIGNUM * vec = NULL, * cnt = NULL ;
   EVP_CIPHER_CTX * pctx = NULL ;

   if ( ( NULL == pCtx ) || ( NULL == out ) || ( NULL == in ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( ( NULL == pCtx->ctx ) || ( NULL == pCtx->cipher ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( 0 == len )
   {
      goto exit ;
   }

   if ( 0 != ( len % OSS_SM4_CIPHER_BLOCK_SIZE ) )
   {
      pctx = EVP_CIPHER_CTX_new();
      if ( NULL == pctx )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      EVP_CIPHER_CTX_init( pctx ) ;
   }

   // new BIGNUM vec 
   vec = BN_new() ;
   if ( NULL == vec )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   // new BIGNUM cnt 
   cnt = BN_new() ;
   if ( NULL == cnt )
   {
      rc = SDB_OOM ;
      goto error ;
   }
  
   // convert pCtx->iv to BIGNUM vec
   if ( NULL == BN_bin2bn( pCtx->iv, sizeof( ossSM4Vec ), vec ) ) 
   {
      rc = SDB_SYS ;
      goto error ;
   } 
   // set BIGNUM cnt to 1
   if ( ! BN_one( cnt ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   EVP_EncryptInit_ex( pCtx->ctx, pCtx->cipher, NULL, pCtx->key, pCtx->iv ) ;
   EVP_CIPHER_CTX_set_padding( pCtx->ctx, 0 ) ;

   // encrypt by 1024Bytes chunk
   while ( OSS_SM4_CIPHER_CHUNK_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( chunkBuf, 0, sizeof( chunkBuf ) ) ;
      if ( ! EVP_EncryptUpdate( pCtx->ctx, chunkBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_CHUNK_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_CHUNK_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, chunkBuf, OSS_SM4_CIPHER_CHUNK_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_CHUNK_SIZE ;
   }
   // encrypt by 128Bytes segment
   while ( OSS_SM4_CIPHER_SGMNT_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( segmtBuf, 0, sizeof( segmtBuf ) ) ;
      if ( ! EVP_EncryptUpdate( pCtx->ctx, segmtBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_SGMNT_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_SGMNT_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, segmtBuf, OSS_SM4_CIPHER_SGMNT_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_SGMNT_SIZE ;
   }
   // encrypt by 16Bytes block 
   while ( OSS_SM4_CIPHER_BLOCK_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( blockBuf, 0, sizeof( blockBuf ) ) ;
      if ( ! EVP_EncryptUpdate( pCtx->ctx, blockBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_BLOCK_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_BLOCK_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, blockBuf, OSS_SM4_CIPHER_BLOCK_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_BLOCK_SIZE ;
   }
   // encrypt the left of input, which is less than 16bytes
   if ( ( len1 > 0 ) && ( OSS_SM4_CIPHER_BLOCK_SIZE > len1 ) )
   {
      INT32 tmpLen = 0 ;
      UINT8 oneByte = 0 ;

      EVP_EncryptInit_ex( pctx, EVP_sm4_ecb(), NULL, pCtx->key, NULL ) ;
      EVP_CIPHER_CTX_set_padding( pctx, 0 ) ;

      for ( UINT16 i = 0; i < len1; i++ )
      {
         ossMemset( vctrBuf, 0, sizeof( vctrBuf ) ) ;
         // convert BIGNUM vec to binary
         if ( BN_bn2binpad( vec, vctrBuf, OSS_SM4_BLOCK_SIZE ) !=
              OSS_SM4_BLOCK_SIZE )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         if ( i > 0 )
         {
            UINT8 delta = in[ offset + i - 1 ] ; 
            if ( ! BN_add_word( cnt, delta ) ) 
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else 
         {
            if ( ! BN_add_word( cnt, len ) )
            {
               rc = SDB_SYS ;
               goto error ;
            }
         } 
         // increase BIGBUM vec by cnt 
         if ( ! BN_add( vec, vec, cnt ) )
         {
            rc = SDB_SYS ;
            goto error ;
         } 
         // build a encrypted block from the vector as a key
         tmpLen = 0 ;
         ossMemset( dataBuf, 0, sizeof( dataBuf ) ) ;
         if ( ! EVP_EncryptUpdate( pctx, dataBuf, &tmpLen, vctrBuf,
                                   OSS_SM4_BLOCK_SIZE ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         if ( OSS_SM4_BLOCK_SIZE != tmpLen )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         // encrypt the left bytes by the 'key' built above 
         oneByte = dataBuf[0]  ;
         for ( UINT16 j = 1; j < OSS_SM4_BLOCK_SIZE; j++ )
         {
            oneByte = oneByte ^ dataBuf[j] ; 
         }
         out[ offset + i ] = in[ offset + i ] ^ oneByte ;
      }
   }

exit:
   if ( NULL != vec )
   {
      BN_free( vec ) ;
   }
   if ( NULL != cnt )
   {
      BN_free( cnt ) ;
   }
   if ( NULL != pctx )
   {
      EVP_CIPHER_CTX_free( pctx ) ;
   }
   return rc ;

error:
   goto exit ;
}


INT32 ossSM4DecryptHybrid( ossSM4Context * pCtx,
                           const UINT8 * in, UINT8 * out, size_t len )
{
   INT32 rc = SDB_OK ;
   size_t offset = 0, len1 = len ;
   INT32 len2 = 0 ;
   UINT8 chunkBuf[ OSS_SM4_CIPHER_CHUNK_SIZE ] = { 0 },
         segmtBuf[ OSS_SM4_CIPHER_SGMNT_SIZE ] = { 0 },
         blockBuf[ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 },
         vctrBuf [ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 },
         dataBuf [ OSS_SM4_CIPHER_BLOCK_SIZE ] = { 0 };
   BIGNUM * vec = NULL, * cnt = NULL ;
   EVP_CIPHER_CTX * pctx = NULL ;

   if ( ( NULL == pCtx ) || ( NULL == out ) || ( NULL == in ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( ( NULL == pCtx->ctx ) || ( NULL == pCtx->cipher ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( 0 == len )
   {
      goto exit ;
   }

   if ( 0 != ( len % OSS_SM4_CIPHER_BLOCK_SIZE ) )
   {
      pctx = EVP_CIPHER_CTX_new();
      if ( NULL == pctx )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      EVP_CIPHER_CTX_init( pctx ) ;
   }

   // new BIGNUM vec 
   vec = BN_new() ;
   if ( NULL == vec )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   // new BIGNUM cnt 
   cnt = BN_new() ;
   if ( NULL == cnt )
   {
      rc = SDB_OOM ;
      goto error ;
   }
  
   // convert pCtx->iv to BIGNUM vec
   if ( NULL == BN_bin2bn( pCtx->iv, sizeof( ossSM4Vec ), vec ) ) 
   {
      rc = SDB_SYS ;
      goto error ;
   } 
   // set BIGNUM cnt to 1
   if ( ! BN_one( cnt ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   EVP_DecryptInit_ex( pCtx->ctx, pCtx->cipher, NULL, pCtx->key, pCtx->iv ) ;
   EVP_CIPHER_CTX_set_padding( pCtx->ctx, 0 ) ;

   // decrypt by 1024Bytes chunk
   while ( OSS_SM4_CIPHER_CHUNK_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( chunkBuf, 0, sizeof( chunkBuf ) ) ;
      if ( ! EVP_DecryptUpdate( pCtx->ctx, chunkBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_CHUNK_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_CHUNK_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, chunkBuf, OSS_SM4_CIPHER_CHUNK_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_CHUNK_SIZE ;
   }
   // decrypt by 128Bytes segment
   while ( OSS_SM4_CIPHER_SGMNT_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( segmtBuf, 0, sizeof( segmtBuf ) ) ;
      if ( ! EVP_DecryptUpdate( pCtx->ctx, segmtBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_SGMNT_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_SGMNT_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, segmtBuf, OSS_SM4_CIPHER_SGMNT_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_SGMNT_SIZE ;
   }
   // decrypt by 16Bytes block 
   while ( OSS_SM4_CIPHER_BLOCK_SIZE <= len1 )
   {
      len2 = 0 ;
      ossMemset( blockBuf, 0, sizeof( blockBuf ) ) ;
      if ( ! EVP_DecryptUpdate( pCtx->ctx, blockBuf, &len2,
                                ( UINT8 * )( in + offset ),
                                OSS_SM4_CIPHER_BLOCK_SIZE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      if ( OSS_SM4_CIPHER_BLOCK_SIZE != len2 )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ossMemcpy( out + offset, blockBuf, OSS_SM4_CIPHER_BLOCK_SIZE ) ;
      offset += len2 ;
      len1 -= OSS_SM4_CIPHER_BLOCK_SIZE ;
   }
   // encrypt the left of input, which is less than 16bytes
   if ( ( len1 > 0 ) && ( OSS_SM4_CIPHER_BLOCK_SIZE > len1 ) )
   {
      INT32 tmpLen = 0 ;
      UINT8 oneByte = 0;

      EVP_EncryptInit_ex( pctx, EVP_sm4_ecb(), NULL, pCtx->key, NULL ) ;
      EVP_CIPHER_CTX_set_padding( pctx, 0 ) ;

      for ( UINT16 i = 0; i < len1; i++ )
      {
         ossMemset( vctrBuf, 0, sizeof( vctrBuf ) ) ;
         // convert BIGNUM vec to binary
         if ( BN_bn2binpad( vec, vctrBuf, OSS_SM4_BLOCK_SIZE ) !=
              OSS_SM4_BLOCK_SIZE )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         if ( i > 0 )
         {
            UINT8 delta = out[ offset + i - 1 ] ;
            if ( ! BN_add_word( cnt, delta ) )
            {
               rc = SDB_SYS ;
              goto error ;
            }
         }
         else
         {
            if ( ! BN_add_word( cnt, len ) )
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }
         // increase BIGBUM vec by cnt 
         if ( ! BN_add( vec, vec, cnt ) )
         {
            rc = SDB_SYS ;
            goto error ;
         } 
         // build a encrypted block from the vector as a key
         tmpLen = 0 ;
         ossMemset( dataBuf, 0, sizeof( dataBuf ) ) ;
         if ( ! EVP_EncryptUpdate( pctx, dataBuf, &tmpLen, vctrBuf,
                                   OSS_SM4_BLOCK_SIZE ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         if ( OSS_SM4_BLOCK_SIZE != tmpLen )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         // decrypt the left bytes by the 'key' built above
         oneByte = dataBuf[0]  ;
         for ( UINT16 j = 1; j < OSS_SM4_BLOCK_SIZE; j++ )
         {
            oneByte = oneByte ^ dataBuf[j] ;
         }
         out[ offset + i ] = in[ offset + i ] ^ oneByte ;
      }
   }

exit:
   if ( NULL != vec )
   {
      BN_free( vec ) ;
   }
   if ( NULL != cnt )
   {
      BN_free( cnt ) ;
   }
   if ( NULL != pctx )
   {
      EVP_CIPHER_CTX_free( pctx ) ;
   }
   return rc ;

error:
   goto exit ;
} 


/*
    SM2
*/
INT32 ossSM2GenKeyPair( EVP_PKEY ** pSM2Key )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX *ctx = NULL ;
   EVP_PKEY *params = NULL ;
   EVP_PKEY_CTX *keyctx = NULL ;

   if ( NULL == pSM2Key )
   {
      rc = SDB_INVALIDARG ;
      goto error ; 
   }

   if ( ! ( ctx = EVP_PKEY_CTX_new_id( EVP_PKEY_EC, NULL ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != ( EVP_PKEY_paramgen_init( ctx ) ) )
   {
      rc = SDB_SYS ;
      goto error ; 
   }

   if ( ( EVP_PKEY_CTX_set_ec_paramgen_curve_nid( ctx, NID_sm2 ) ) <= 0 )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != EVP_PKEY_paramgen(ctx, &params))
   {
      rc = SDB_SYS ;
      goto error;
   }

   keyctx = EVP_PKEY_CTX_new( params, NULL ) ;
   if ( !keyctx )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != EVP_PKEY_keygen_init(keyctx) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != ( EVP_PKEY_keygen( keyctx, pSM2Key ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( *pSM2Key, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( ctx )
   {
      EVP_PKEY_CTX_free( ctx ) ; 
   }
   if ( keyctx )
   {
      EVP_PKEY_CTX_free( keyctx ) ; 
   }
   if ( params )
   {
      EVP_PKEY_free( params ) ;
   }
   return rc ;

error:
   goto exit ;
}


void ossSM2FreeKeyPair( EVP_PKEY * pSM2Key )
{
   if ( pSM2Key )
   {
      EVP_PKEY_free( pSM2Key ) ;
   } 
}


INT32 ossSM2Encrypt( EVP_PKEY *   pkey,
                     const UINT8 * in,  size_t iLen,
                     UINT8 * out, size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * pctx = NULL ;

   if ( ( NULL == pkey ) || ( NULL == in ) ||
        ( NULL == out ) || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !( pctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_encrypt_init( pctx ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != ( EVP_PKEY_encrypt( pctx, out, oLen, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

exit:
   if ( NULL != pctx )
   {
      EVP_PKEY_CTX_free( pctx ) ; 
   }
   return rc ;
error:
   goto exit ;
}


INT32 ossSM2EstimateEncryptionSize( EVP_PKEY * pkey,
                                    const UINT8 * in,  size_t iLen,
                                    size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * pctx = NULL ;

   if ( ( NULL == pkey ) || ( NULL == in ) || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !( pctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_encrypt_init( pctx ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

   if ( 1 != ( EVP_PKEY_encrypt( pctx, NULL, oLen, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error;
   }

exit:
   if ( NULL != pctx )
   {
      EVP_PKEY_CTX_free( pctx ) ; 
   }
   return rc ;

error:
   goto exit ;
}


INT32 ossSM2Decrypt( EVP_PKEY *   pkey,
                     const UINT8 * in,  size_t iLen,
                     UINT8 * out, size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * pctx = NULL ;

   if ( ( NULL == pkey ) || ( NULL == in ) ||
        ( NULL == out )  || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !( pctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_decrypt_init( pctx ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_decrypt( pctx, out, oLen, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( NULL != pctx )
   {
      EVP_PKEY_CTX_free( pctx ) ; 
   }
   return rc ;
error:
   goto exit ;
}


INT32 ossSM2EstimateDecryptionSize( EVP_PKEY *   pkey,
                                    const UINT8 * in,  size_t iLen,
                                    size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * pctx = NULL ;

   if ( ( NULL == pkey ) || ( NULL == in ) || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( !( pctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_decrypt_init( pctx ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_decrypt( pctx, NULL, oLen, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( NULL != pctx )
   {
      EVP_PKEY_CTX_free( pctx ) ; 
   }
   return rc ;
error:
   goto exit ;
}


INT32 ossSM2Sign( EVP_PKEY *   pkey,
                  const UINT8 * in,  size_t iLen,
                  UINT8 * out, size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * sctx = NULL ;
   EVP_MD_CTX * md_ctx = NULL ;

   UINT8 sm2_id[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                      0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };
   UINT32 sm2_id_len = sizeof( sm2_id );

   if ( ( NULL == pkey ) || ( NULL == in ) ||
        ( NULL == out )  || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( ! ( md_ctx = EVP_MD_CTX_new() ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( ! ( sctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( EVP_PKEY_CTX_set1_id( sctx, sm2_id, sm2_id_len ) <= 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   EVP_MD_CTX_set_pkey_ctx( md_ctx, sctx );

   if ( 1 != ( EVP_DigestSignInit( md_ctx, NULL, EVP_sm3(), NULL, pkey ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestSignUpdate( md_ctx, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestSignFinal( md_ctx, out, oLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( NULL != md_ctx )
   {
      EVP_MD_CTX_free( md_ctx ) ;
   }

   if ( NULL != sctx )
   {
      EVP_PKEY_CTX_free( sctx ) ;
   }
   return rc ;

error:
   goto exit ;
}


INT32 ossSM2EstimateSignatureSize( EVP_PKEY *   pkey,
                                   const UINT8 * in,  size_t iLen,
                                   size_t * oLen )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * sctx = NULL ;
   EVP_MD_CTX * md_ctx = NULL ;

   UINT8 sm2_id[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                      0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };
   UINT32 sm2_id_len = sizeof( sm2_id );

   if ( ( NULL == pkey ) || ( NULL == in ) || ( NULL == oLen ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( ! ( md_ctx = EVP_MD_CTX_new() ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( ! ( sctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( EVP_PKEY_CTX_set1_id( sctx, sm2_id, sm2_id_len ) <= 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   EVP_MD_CTX_set_pkey_ctx( md_ctx, sctx );

   if ( 1 != ( EVP_DigestSignInit( md_ctx, NULL, EVP_sm3(), NULL, pkey ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestSignUpdate( md_ctx, in, iLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestSignFinal( md_ctx, NULL, oLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit:
   if ( NULL != md_ctx )
   {
      EVP_MD_CTX_free( md_ctx ) ;
   }

   if ( NULL != sctx )
   {
      EVP_PKEY_CTX_free( sctx ) ;
   }
   return rc ;

error:
   goto exit ;
}


INT32 ossSM2Verify( EVP_PKEY * pkey,
                    const UINT8 * signature, size_t signatureLen,
                    const UINT8 * message,   size_t messageLen,
                    BOOLEAN & result )
{
   INT32 rc = SDB_OK ;
   EVP_PKEY_CTX * sctx = NULL ;
   EVP_MD_CTX * md_ctx_verify = NULL ;

   UINT8 sm2_id[] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
                      0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 } ;
   UINT32 sm2_id_len = sizeof( sm2_id ) ;

   if ( ( NULL == pkey ) || ( NULL == signature ) || ( NULL == message ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( pkey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( ! ( md_ctx_verify = EVP_MD_CTX_new() ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( ! ( sctx = EVP_PKEY_CTX_new( pkey, NULL ) ) )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( EVP_PKEY_CTX_set1_id( sctx, sm2_id, sm2_id_len ) <= 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   EVP_MD_CTX_set_pkey_ctx( md_ctx_verify, sctx ) ;

   if (1 != (EVP_DigestVerifyInit(md_ctx_verify, NULL, EVP_sm3(), NULL, pkey)))
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestVerifyUpdate( md_ctx_verify, message, messageLen ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_DigestVerifyFinal(md_ctx_verify, signature, signatureLen) ) )
   {
      result = FALSE ;
   }
   else
   {
      result = TRUE ;
   }

exit:
   if ( NULL != md_ctx_verify )
   {
      EVP_MD_CTX_free( md_ctx_verify ) ;
   }

   if ( NULL != sctx )
   {
      EVP_PKEY_CTX_free( sctx ) ;
   }
   return rc ;

error:
   goto exit ;
}


INT32 ossSM2WriteKeyPairToFile( const CHAR * fileName,
                                EVP_PKEY * pSM2KeyPair )
{
   INT32 rc = SDB_OK ;
   BIO * bio_out = NULL ;
   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;

   if ( ( NULL == fileName ) || ( NULL == pSM2KeyPair ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_out = BIO_new_file( fileName, "w" ) ;
   if ( NULL == bio_out )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   // write priavate key to file
   if ( 1 != PEM_write_bio_PrivateKey( bio_out, pSM2KeyPair, EVP_sm4_cbc(), NULL, 0, NULL,
                                       myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   // write public key to file
   if ( 1 != PEM_write_bio_PUBKEY( bio_out, pSM2KeyPair ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   BIO_flush( bio_out ) ;
exit :
   if ( NULL != bio_out )
   {
      BIO_free( bio_out ) ;
   }
   return rc ;
error:
   goto exit ;   
}


INT32 ossSM2WritePrivateKeyToFile( const CHAR * fileName,
                                   EVP_PKEY * pSM2PrivateKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_out = NULL ;
   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;

   if ( ( NULL == fileName ) || ( NULL == pSM2PrivateKey ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_out = BIO_new_file( fileName, "a" ) ;
   if ( NULL == bio_out )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( 1 != PEM_write_bio_PrivateKey( bio_out, pSM2PrivateKey,
                                       EVP_sm4_cbc(),
                                       NULL, 0, NULL, myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   } 
   BIO_flush( bio_out ) ;
exit :
   if ( NULL != bio_out )
   {
      BIO_free( bio_out ) ;
   }
   return rc ;
error:
   goto exit ;   
}


INT32 ossSM2WritePublicKeyToFile( const CHAR * fileName,
                                  EVP_PKEY * pSM2PublicKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_out = NULL ;

   if ( ( NULL == fileName ) || ( NULL == pSM2PublicKey ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_out = BIO_new_file( fileName, "a" ) ;
   if ( NULL == bio_out )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   if ( 1 != PEM_write_bio_PUBKEY( bio_out, pSM2PublicKey ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   BIO_flush( bio_out ) ;
exit :
   if ( NULL != bio_out )
   {
      BIO_free( bio_out ) ;
   }
   return rc ;
error:
   goto exit ;   
}


INT32 ossSM2ReadPrivateKeyFromFile( const CHAR * fileName,
                                    EVP_PKEY ** pSM2PrivateKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_in = NULL ;
   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;

   if ( ( NULL == fileName ) || ( NULL == pSM2PrivateKey ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_in = BIO_new_file( fileName, "r" ) ;
   if ( NULL == bio_in )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( NULL == PEM_read_bio_PrivateKey( bio_in, pSM2PrivateKey,
                                         NULL, myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( *pSM2PrivateKey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit :
   if ( NULL != bio_in )
   {
      BIO_free( bio_in ) ;
   }
   return rc ;
error:
   goto exit ;
}


INT32 ossSM2ReadPublickKeyFromFile( const CHAR * fileName,
                                    EVP_PKEY ** pSM2PublicKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_in = NULL ;

   if ( ( NULL == fileName ) || ( NULL == pSM2PublicKey ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_in = BIO_new_file( fileName, "r" ) ;
   if ( NULL == bio_in )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( NULL == PEM_read_bio_PUBKEY( bio_in, pSM2PublicKey,
                                     NULL, NULL ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( *pSM2PublicKey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit :
   if ( NULL != bio_in )
   {
      BIO_free( bio_in ) ;
   }
   return rc ;
error:
   goto exit ;
}

#define OSS_SM2_MAX_KEY_STR_LEN ( 1024 )
INT32 ossSM2WriteKeyPairToStrings( EVP_PKEY * pSM2KeyPair,
                                   string & priKey,
                                   string & pubKey )
{
   INT32 rc = SDB_OK ;
   size_t priLen = 0, pubLen = 0 ;
   CHAR pri_key[ OSS_SM2_MAX_KEY_STR_LEN ] = { '\0' }, 
        pub_key[ OSS_SM2_MAX_KEY_STR_LEN ] = { '\0' } ;
   BIO * pri = NULL,  * pub = NULL ;

   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;
   
   if ( NULL == pSM2KeyPair )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pri = BIO_new( BIO_s_mem() ) ;
   pub = BIO_new( BIO_s_mem() ) ;
   if ( ( NULL == pri ) || ( NULL == pub ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   // priavate key
   if ( 1 != PEM_write_bio_PrivateKey( pri, pSM2KeyPair, EVP_sm4_cbc(),
                                       NULL, 0, NULL, myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   priLen = BIO_ctrl_pending( pri ) ;
   if ( BIO_read( pri, pri_key, priLen ) < 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   } 
   pri_key[ priLen ] = '\0' ;

   // public key
   if ( 1 != PEM_write_bio_PUBKEY( pub, pSM2KeyPair ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   pubLen = BIO_ctrl_pending( pub ) ;
   if ( BIO_read( pub, pub_key, pubLen ) < 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   pub_key[ pubLen ] = '\0' ;

   //
   priKey = pri_key ;
   pubKey = pub_key ;

exit :
   if ( NULL != pub )
   {
      BIO_free_all( pub ) ;
   }
   if ( NULL != pri )
   {
      BIO_free_all( pri ) ;  
   }
   return rc ; 
error:
   goto exit ;
}


INT32 ossSM2WritePrivateKeyToString( EVP_PKEY * pSM2PrivateKey,
                                     string & priKey ) 
{
   INT32 rc = SDB_OK ;
   size_t priLen = 0 ;
   CHAR pri_key[ OSS_SM2_MAX_KEY_STR_LEN ] = { 0 } ;
   BIO * pri = NULL ;
   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;
   
   if ( NULL == pSM2PrivateKey )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pri = BIO_new( BIO_s_mem() ) ;
   if ( NULL == pri )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   // priavate key
   if ( 1 != PEM_write_bio_PrivateKey( pri, pSM2PrivateKey, EVP_sm4_cbc(),
                                       NULL, 0, NULL, myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   priLen = BIO_pending( pri ) ;

   if ( BIO_read( pri, pri_key, priLen ) < 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   } 

   priKey = pri_key ;

exit :
   if ( NULL != pri )
   {
      BIO_free_all( pri ) ;  
   }
   return rc ; 
error:
   goto exit ;
}



INT32 ossSM2WritePublicKeyToString( EVP_PKEY * pSM2PublicKey,
                                    string & pubKey )
{
   INT32 rc = SDB_OK ;
   size_t pubLen = 0 ;
   CHAR pub_key[ OSS_SM2_MAX_KEY_STR_LEN ] = { 0 } ;
   BIO * pub = NULL ;
   
   if ( NULL == pSM2PublicKey )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pub = BIO_new( BIO_s_mem() ) ;
   if ( NULL == pub )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   if ( 1 != PEM_write_bio_PUBKEY( pub, pSM2PublicKey ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   pubLen = BIO_pending( pub ) ;
   
   if ( BIO_read( pub, pub_key, pubLen ) < 0 )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   pubKey = pub_key ;

exit :
   if ( NULL != pub )
   {
      BIO_free_all( pub ) ;
   }
   return rc ; 
error:
   goto exit ;
}


INT32 ossSM2ReadPrivateKeyFromString( const string & privateKey,
                                      EVP_PKEY ** pSM2PrivateKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_in = NULL ;
   CHAR * myPassphrase = OSS_SM2_KEYFILE_PASSPHRASE ;

   if ( NULL == pSM2PrivateKey )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bio_in = BIO_new_mem_buf( privateKey.data(), privateKey.length() ) ;
   if ( NULL == bio_in )
   {
      rc = SDB_SYS ;
      goto error ;

   }   
   if ( NULL == PEM_read_bio_PrivateKey( bio_in, pSM2PrivateKey,
                                         NULL, myPassphrase ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( *pSM2PrivateKey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

exit :
   if ( NULL != bio_in )
   {
      BIO_free_all( bio_in ) ;
   }
   return rc ;
error:
   goto exit ;
}

INT32 ossSM2ReadPublicKeyFromString( const string & publicKey,
                                      EVP_PKEY ** pSM2PublicKey )
{
   INT32 rc = SDB_OK ;
   BIO * bio_in = NULL ;

   if ( NULL == pSM2PublicKey )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bio_in = BIO_new_mem_buf( publicKey.data(), publicKey.length() ) ;
   if ( NULL == bio_in )
   {
      rc = SDB_SYS ;
      goto error ;

   }
   if ( NULL == PEM_read_bio_PUBKEY( bio_in, pSM2PublicKey,
                                     NULL, NULL ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( 1 != ( EVP_PKEY_set_alias_type( *pSM2PublicKey, EVP_PKEY_SM2 ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
exit :
   if ( NULL != bio_in )
   {
      BIO_free_all( bio_in ) ;
   }
   return rc ;
error:
   goto exit ;
}
