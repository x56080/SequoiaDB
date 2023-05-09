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
