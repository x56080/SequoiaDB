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

   Source File Name = utilCipher.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHER_H_
#define UTILCIPHER_H_

#include "ossTypes.h"

void   utilCipherGenerateRandomArray( CHAR* array, UINT32 arrayLen ) ;
INT32  utilCipherExtractRandomArray( CHAR *cipherText, UINT32 cipherTextLen,
                                     CHAR *array, UINT32 *cipherTextNewLen,
                                     UINT32 *arrayNewLen ) ;
void   utilCipherInsertRandomArray( CHAR *destArray, UINT32 destArrayLen,
                                    CHAR *randArray, UINT32 randArrayLen,
                                    UINT32 *destArrayNewLen ) ;
INT32  utilCipherEncrypt( const CHAR *clearText, const CHAR *token,
                          CHAR *cipherText, UINT32 cipherTextLen ) ;
INT32  utilCipherDecrypt( const CHAR *cipherText, const CHAR *token,
                          CHAR *clearText ) ;
INT32  utilDecryptUserCipher( const CHAR *user, const CHAR *token,
                              const CHAR *path, CHAR *connectionUser,
                              CHAR *clearText ) ;

#endif /* UTILCIPHER_H_ */