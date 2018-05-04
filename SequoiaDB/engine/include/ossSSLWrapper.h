/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ossSSLWrapper.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/1/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SSL_WRAPPER_H_
#define OSS_SSL_WRAPPER_H_

#ifdef SDB_SSL

#include "core.h"

#define SSL_OK       (0)
#define SSL_ERROR    (-1)
#define SSL_AGAIN    (-2)
#define SSL_TIMEOUT  (-3)

#ifdef _DEBUG
#include <assert.h>
#define SSL_ASSERT(e) assert(e)
#else
#define SSL_ASSERT(e) 
#endif

SDB_EXTERN_C_START

typedef struct evp_pkey_st SSLKey;
typedef struct x509_st     SSLCertificate;
typedef struct ssl_ctx_st  SSLContext;
typedef struct SSLHandle   SSLHandle;

/* ossSSLCertificate.c */
INT32 ossSSLNewKey(SSLKey** key);
void  ossSSLFreeKey(SSLKey** key);
INT32 ossSSLNewCertificate(SSLCertificate** cert, SSLKey* key);
void  ossSSLFreeCertificate(SSLCertificate** cert);

/* ossSSLWrapper.c */
INT32 ossSSLInit();
void  ossSSLFinalize();
INT32 ossSSLNewContext(SSLContext** context, SSLCertificate* cert, SSLKey* key);
void  ossSSLFreeContext(SSLContext** context);
INT32 ossSSLNewHandle(SSLHandle** handle, SSLContext* ctx, SOCKET sock, const char* initialBytes, INT32 len);
void  ossSSLFreeHandle(SSLHandle** handle);
INT32 ossSSLConnect(SSLHandle* handle);
INT32 ossSSLAccept(SSLHandle* handle);
INT32 ossSSLRead(SSLHandle* handle, void* buf, INT32 num);
INT32 ossSSLPeek(SSLHandle* handle, void* buf, INT32 num);
INT32 ossSSLWrite(SSLHandle* handle, const void* buf, INT32 num);
INT32 ossSSLShutdown(SSLHandle* handle);
INT32 ossSSLGetError(SSLHandle* handle);
char* ossSSLGetErrorMessage(INT32 error);
INT32 ossSSLERRGetError();
char* ossSSLERRGetErrorMessage(INT32 error);

/* ossSSLContext.c */
SSLContext* ossSSLGetContext();

SDB_EXTERN_C_END

#else
#error "include \"ossSSLWrapper.h\" without SDB_SSL"
#endif /* SDB_SSL */

#endif /* OSS_SSL_WRAPPER_H_ */

