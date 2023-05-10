#include "dmsLobCryptor.hpp"
#include "dmsLobDef.hpp"
#include "pmd.hpp"

namespace engine
{
   _dmsLobCryptor::_dmsLobCryptor( const ossSM4Key dek, const UINT8 *nonce, UINT32 offset ) : _offset( offset )
   {
      ossMemcpy( _dek, dek, OSS_SM4_KEY_SIZE ) ;
      ossMemcpy( _ctr, nonce, DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) ;
      *(UINT64 *)( _ctr + DMS_LOB_ENCRYPTION_CTR_NONCE_SIZE ) = offset / OSS_SM4_BLOCK_SIZE ;
   }

   INT32 _dmsLobCryptor::encrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen ) const
   {
      INT32 rc = SDB_OK ;
      UINT32 offset = _offset ;
      UINT32 len = ilen ;
      UINT32 startBlockSeq = offset / OSS_SM4_BLOCK_SIZE ;
      UINT32 startOffset = startBlockSeq * OSS_SM4_BLOCK_SIZE ;
      UINT32 extraLen = offset - startOffset ;
      UINT8 *pNewInBuffer = NULL ;

      if ( extraLen > 0 )
      {
         pNewInBuffer = (UINT8 *)SDB_THREAD_ALLOC( ilen + extraLen ) ;
         ossMemcpy( pNewInBuffer + extraLen, in, ilen ) ;
         in = pNewInBuffer + extraLen ;
      }
      ossSM4Context ctx( OSS_SM4_CTR ) ;
      rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to initialize encryption context, rc:%d. "
                   "Data encryption will not be performed for "
                   "operation this time",
                   rc ) ;
      {
         ctx.setKey( _dek ) ;
         ctx.setVec( _ctr ) ;
         UINT32 outLen = 0 ;
         rc = ossSM4Encrypt( &ctx, in - extraLen, len + extraLen, out, &outLen ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to encrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == len + extraLen, "unexpected length" ) ;
         if ( olen )
         {
            *olen = outLen ;
         }
      }

   done:
      ossSM4Fin( &ctx ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsLobCryptor::decrypt( const UINT8 *in, UINT32 ilen, UINT8 *out, UINT32 *olen ) const
   {
      INT32 rc = SDB_OK ;
      UINT32 offset = _offset ;
      UINT32 len = ilen ;
      UINT32 startBlockSeq = offset / OSS_SM4_BLOCK_SIZE ;
      UINT32 startOffset = startBlockSeq * OSS_SM4_BLOCK_SIZE ;
      UINT32 extraLen = offset - startOffset ;
      UINT8 *pNewInBuffer = NULL ;

      if ( extraLen > 0 )
      {
         pNewInBuffer = (UINT8 *)SDB_THREAD_ALLOC( ilen + extraLen ) ;
         ossMemcpy( pNewInBuffer + extraLen, in, ilen ) ;
         in = pNewInBuffer + extraLen ;
      }
      ossSM4Context ctx( OSS_SM4_CTR ) ;
      rc = ossSM4Init( &ctx, OSS_SM4_CTR ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to initialize encryption context, rc:%d. "
                   "Data encryption will not be performed for "
                   "operation this time",
                   rc ) ;
      {
         ctx.setKey( _dek ) ;
         ctx.setVec( _ctr ) ;
         UINT32 outLen = 0 ;
         rc = ossSM4Decrypt( &ctx, in - extraLen, len + extraLen, out, &outLen ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to encrypt data, rc: %d", rc ) ;
         SDB_ASSERT( outLen == len + extraLen, "unexpected length" ) ;
         if ( olen )
         {
            *olen = outLen ;
         }
      }

   done:
      ossSM4Fin( &ctx ) ;
      return rc ;
   error:
      goto done ;
   }
} // namespace engine