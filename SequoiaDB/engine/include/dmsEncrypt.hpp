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

   Source File Name = dmsEncrypt.hpp 

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
#ifndef DMSENCRYPT_HPP__
#define DMSENCRYPT_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "dms.hpp"
#include "ossGMCrypto.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   // Initialize dms encryption vector by RID
   void dmsInitEncVecByRID( const dmsRecordID * rid, ossSM4Vec vec ) ;

   // The first 4 Bytes of a BSONObj reprsents its size, and function
   // dmsRecord::getDataLength() peeks these 4 bytes to get the length of
   // this BSON obj. To avoid breaking getDataLength() and the hassle of
   // having to decrypt every time in order to get the length of an encrypted
   // BSON obj, instead of encyrpting the entire BSON obj, we simply skip off
   // the first 4 bytes from beginning, i.e., starting from pInputData + 4.
   INT32 dmsBSONEncrypt( _pmdEDUCB *cb, ossSM4Context * ctx, 
                         const CHAR *pInputData, INT32 inputSize,
                         const CHAR **ppData, INT32 *pDataSize = NULL ) ;

   // caller shall make sure the key and vector are same as they
   // were used while encrypting.
   //
   // As mentioned above, when encrypt we skip off the first 4 bytes, so when
   // decrypt we start from pInputData + sizeof(int). 
   INT32 dmsBSONDecrypt( _pmdEDUCB *cb, ossSM4Context * ctx,
                         const CHAR *pInputData, INT32 inputSize,
                         const CHAR **ppData, INT32 *pDataSize = NULL ) ;

   // get unencrypted DEK 
   INT32 dmsSecGetDEK( const CHAR * dbPath, UINT8 * pDEK ) ;

   INT32 dmsBinEncrypt( _pmdEDUCB *cb,
                        ossSM4Context *ctx,
                        const CHAR *pInputData,
                        INT32 inputSize,
                        const CHAR **ppData,
                        INT32 *pDataSize = NULL ) ;
   
   INT32 dmsBinDecrypt( _pmdEDUCB *cb,
                        ossSM4Context *ctx,
                        const CHAR *pInputData,
                        INT32 inputSize,
                        const CHAR **ppData,
                        INT32 *pDataSize = NULL ) ;
}
#endif // DMSENCRYPT_HPP__
