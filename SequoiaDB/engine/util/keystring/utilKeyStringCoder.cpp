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

   Source File Name = utilKeyStringCoder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "keystring/utilKeyStringCoder.hpp"

namespace engine
{
namespace keystring
{
   void _keyStringCoder::encodeRID( const dmsRecordID &rid, void *buf )
   {
      CHAR *ptr = (CHAR *)buf ;
      encodeUnsignedNative( (UINT32)( rid._extent ), FALSE, ptr ) ;
      ptr += sizeof( UINT32 ) ;
      encodeUnsignedNative( (UINT32)( rid._offset ), FALSE, ptr ) ;
      return ;
   }

   dmsRecordID _keyStringCoder::decodeToRID( const void *buf ) const
   {
      dmsRecordID rid ;
      rid._extent = (dmsExtentID)(
            decodeToUnsignedNative<UINT32>( buf, FALSE ) ) ;
      rid._offset = (dmsOffset)(
            decodeToUnsignedNative<UINT32>(
                  (const CHAR *)buf + sizeof( UINT32 ), FALSE ) ) ;
      return rid ;
   }

   void _keyStringCoder::encodeLSN( UINT64 lsn, void *buf )
   {
      encodeUnsignedNative( lsn, TRUE, buf ) ;
   }

   UINT64 _keyStringCoder::decodeToLSN( const void *buf ) const
   {
      return decodeToUnsignedNative<UINT64>( buf, TRUE ) ;
   }

   void _keyStringCoder::encodeIndexID( const dmsIdxMetadataKey &id,
                                        BOOLEAN asUpperKey,
                                        void *buf )
   {
      CHAR *data = (CHAR *)buf ;
      encodeUnsignedNative( (UINT64)( id.getCLOrigUID() ), FALSE, data ) ;
      data += sizeof( UINT64 ) ;
      encodeUnsignedNative( (UINT32)( id.getCLOrigLID() ), FALSE, data ) ;
      data += sizeof( UINT32 ) ;
      encodeUnsignedNative( (UINT32)( ( asUpperKey ) ?
                                      ( id.getIdxInnerID() + 1 ) :
                                      ( id.getIdxInnerID() ) ),
                            FALSE, data ) ;
   }

   dmsIdxMetadataKey _keyStringCoder::decodeToIndexID( const void *buf ) const
   {
      return dmsIdxMetadataKey(
            (utilCLUniqueID)( decodeToUnsignedNative<UINT64>( (const CHAR *)buf, FALSE ) ),
            (UINT32)( decodeToUnsignedNative<UINT32>( (const CHAR *)buf + sizeof( UINT64 ), FALSE ) ),
            (utilIdxInnerID)( decodeToUnsignedNative<UINT32>( (const CHAR *)buf + sizeof( UINT64 ) + sizeof( UINT32 ), FALSE ) ) ) ;
   }

}
}
