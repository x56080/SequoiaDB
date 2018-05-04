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

   Source File Name = ossSSLContext.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/2/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifdef SDB_SSL

#include "ossSSLWrapper.h"
#include "oss.h"

SDB_EXTERN_C_START

typedef struct SSLGlobalContext
{
   SSLContext*       ctx;
   SSLCertificate*   cert;
   SSLKey*           key;
} SSLGlobalContext;

static SSLGlobalContext sslGlobalContext = {NULL};

static void _ossSSLInitGlobalContext()
{
   SSLKey* key = NULL;
   SSLCertificate* cert = NULL;
   SSLContext* ctx = NULL;
   INT32 ret;

   static BOOLEAN inited = FALSE;
   SSL_ASSERT(!inited);

   ret = ossSSLInit();
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewKey(&key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewCertificate(&cert, key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = ossSSLNewContext(&ctx, cert, key);
   if (SSL_OK != ret)
   {
      goto error;
   }

   sslGlobalContext.ctx = ctx;
   sslGlobalContext.cert = cert;
   sslGlobalContext.key = key;

   inited = TRUE;

done:
   return;
error:
   if (NULL != ctx)
   {
      ossSSLFreeContext(&ctx);
   }
   if (NULL != cert)
   {
      ossSSLFreeCertificate(&cert);
   }
   if (NULL != key)
   {
      ossSSLFreeKey(&key);
   }
   goto done;
}

SSLContext* ossSSLGetContext()
{
   static ossOnce initOnce = OSS_ONCE_INIT;

   ossOnceRun(&initOnce, _ossSSLInitGlobalContext);

   return sslGlobalContext.ctx;
}

SDB_EXTERN_C_END

#endif /* SDB_SSL */
