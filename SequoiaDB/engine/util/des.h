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

   Source File Name = des.h

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_DES_H__
#define UTIL_DES_H__

#include "core.h"

SDB_EXTERN_C_START

SDB_EXPORT INT32 desEncryptSize( INT32 expressLen ) ;

SDB_EXPORT INT32 desEncrypt( BYTE *pDesKey, BYTE *pExpress, INT32 expressLen,
                             BYTE *pCiphertext ) ;

SDB_EXTERN_C_END

#endif
