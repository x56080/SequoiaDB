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

   Source File Name = dmsLobCryptor.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsLobCryptor.hpp"
#include "dmsLobDef.hpp"
#include "pmd.hpp"

namespace engine
{
   _dmsLobCryptor::_dmsLobCryptor( const ossSM4Key dek, const UINT8 *nonce, UINT32 offset )
   : _offset( offset )
   {
      ossMemcpy( _dek, dek, OSS_SM4_KEY_SIZE ) ;
      ossMemcpy( _ctr, nonce, DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) ;

      UINT64 counter = offset / OSS_SM4_BLOCK_SIZE ;
      #ifndef SDB_BIG_ENDIAN
      ossEndianConvert8( counter, _ctr + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) ;
      #else
      *(UINT64 *)( _ctr + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) = counter ;
      #endif
   }

   INT32 _dmsLobCryptor::encrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen ) const
   {
      INT32 rc = SDB_OK ;
      UINT32 offset = _offset ;
      UINT32 len = ilen ;
      UINT32 extraLen = offset % OSS_SM4_BLOCK_SIZE ;
      UINT32 misalignedLen = ( offset + len ) / OSS_SM4_BLOCK_SIZE == offset / OSS_SM4_BLOCK_SIZE
                                ? ( offset + len ) % OSS_SM4_BLOCK_SIZE - extraLen
                                : OSS_SM4_BLOCK_SIZE - extraLen ;
      UINT8 *pNewInBuffer = NULL ;
      UINT8 *pNewOutBuffer = NULL ;
      SDB_ASSERT( len >= misalignedLen, "Must be greater" ) ;

      if ( extraLen > 0 )
      {
         pNewInBuffer = (UINT8 *)SDB_THREAD_ALLOC( OSS_SM4_BLOCK_SIZE ) ;
         pNewOutBuffer = (UINT8 *)SDB_THREAD_ALLOC( OSS_SM4_BLOCK_SIZE ) ;
         ossMemcpy( pNewInBuffer + extraLen, in, misalignedLen ) ;
         ossSM4Context ctx( OSS_SM4_CTR ) ;
         rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize encryption context, rc:%d", rc ) ;
         ctx.setKey( _dek ) ;
         ctx.setVec( _ctr ) ;
         UINT32 outLen = 0 ;
         rc = ossSM4Encrypt( &ctx, pNewInBuffer, OSS_SM4_BLOCK_SIZE, pNewOutBuffer, &outLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to encrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == OSS_SM4_BLOCK_SIZE, "Unexpected length") ;
         ossMemcpy( out, pNewOutBuffer + extraLen, misalignedLen ) ;
         in = in + misalignedLen ;
         len = len - misalignedLen ;
         out = out + misalignedLen ;
         if ( olen )
         {
            *olen = misalignedLen;
         }
      }

      if ( len > 0 )
      {
         ossSM4Context ctx( OSS_SM4_CTR ) ;
         rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize encryption context, rc:%d", rc ) ;
         ctx.setKey( _dek ) ;
         if ( extraLen > 0 )
         {
            ossMemcpy(ctx.iv, _ctr, DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE);
            UINT64 counter = offset / OSS_SM4_BLOCK_SIZE + 1;
            #ifndef SDB_BIG_ENDIAN
            ossEndianConvert8( counter, ctx.iv + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) ;
            #else
            *(UINT64 *)( ctx.iv + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) = counter ;
            #endif
         }
         else
         {
            ctx.setVec( _ctr ) ;
         }
         UINT32 outLen = 0 ;
         rc = ossSM4Encrypt( &ctx, in, len, out, &outLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to encrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == len, "Unexpected length" ) ;
         if ( olen )
         {
            *olen += outLen ;
         }
      }

   done:
      if ( pNewInBuffer )
      {
         SDB_THREAD_FREE( pNewInBuffer ) ;
      }
      if ( pNewOutBuffer )
      {
         SDB_THREAD_FREE( pNewOutBuffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsLobCryptor::decrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen ) const
   {
      INT32 rc = SDB_OK ;
      UINT32 offset = _offset ;
      UINT32 len = ilen ;
      UINT32 extraLen = offset % OSS_SM4_BLOCK_SIZE ;
      UINT32 misalignedLen = ( offset + len ) / OSS_SM4_BLOCK_SIZE == offset / OSS_SM4_BLOCK_SIZE
                                ? ( offset + len ) % OSS_SM4_BLOCK_SIZE - extraLen
                                : OSS_SM4_BLOCK_SIZE - extraLen ;
      UINT8 *pNewInBuffer = NULL ;
      UINT8 *pNewOutBuffer = NULL ;
      SDB_ASSERT( len >= misalignedLen, "Must be greater" ) ;

      if ( extraLen > 0 )
      {
         pNewInBuffer = (UINT8 *)SDB_THREAD_ALLOC( OSS_SM4_BLOCK_SIZE ) ;
         pNewOutBuffer = (UINT8 *)SDB_THREAD_ALLOC( OSS_SM4_BLOCK_SIZE ) ;
         ossMemcpy( pNewInBuffer + extraLen, in, misalignedLen ) ;
         ossSM4Context ctx( OSS_SM4_CTR ) ;
         rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize decryption context, rc:%d", rc ) ;
         ctx.setKey( _dek ) ;
         ctx.setVec( _ctr ) ;
         UINT32 outLen = 0 ;
         rc = ossSM4Decrypt( &ctx, pNewInBuffer, OSS_SM4_BLOCK_SIZE, pNewOutBuffer, &outLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to decrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == OSS_SM4_BLOCK_SIZE, "Unexpected length") ;
         ossMemcpy( out, pNewOutBuffer + extraLen, misalignedLen ) ;
         in = in + misalignedLen ;
         len = len - misalignedLen ;
         out = out + misalignedLen ;
         if ( olen )
         {
            *olen = misalignedLen;
         }
      }

      if ( len > 0 )
      {
         ossSM4Context ctx( OSS_SM4_CTR ) ;
         rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize decryption context, rc:%d", rc ) ;
         ctx.setKey( _dek ) ;
         if ( extraLen > 0 )
         {
            ossMemcpy(ctx.iv, _ctr, DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE);
            UINT64 counter = offset / OSS_SM4_BLOCK_SIZE + 1;
            #ifndef SDB_BIG_ENDIAN
            ossEndianConvert8( counter, ctx.iv + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) ;
            #else
            *(UINT64 *)( ctx.iv + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) = counter ;
            #endif
         }
         else
         {
            ctx.setVec( _ctr ) ;
         }
         UINT32 outLen = 0 ;
         rc = ossSM4Decrypt( &ctx, in, len, out, &outLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to decrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == len, "Unexpected length" ) ;
         if ( olen )
         {
            *olen += outLen ;
         }
      }

   done:
      if ( pNewInBuffer )
      {
         SDB_THREAD_FREE( pNewInBuffer ) ;
      }
      if ( pNewOutBuffer )
      {
         SDB_THREAD_FREE( pNewOutBuffer ) ;
      }
      return rc ;
   error:
      goto done ;
   }
} // namespace engine