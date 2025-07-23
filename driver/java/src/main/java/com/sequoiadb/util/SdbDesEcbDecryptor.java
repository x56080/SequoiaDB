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

   Source File Name = SdbDesEcbDecryptor.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.util;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

class SdbDesEcbDecryptor {
    private Cipher cipher;

    SdbDesEcbDecryptor(byte[] key) {
        try {
            int mode = Cipher.DECRYPT_MODE;
            SecretKeySpec keySpec = new SecretKeySpec(key, "DES");
            cipher = Cipher.getInstance("DES/ECB/NoPadding");
            cipher.init(mode, keySpec);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "init cipher failed", e);
        }
    }

    public byte[] desDecrypt(byte[] encryptedValue) {
        try {
            return cipher.doFinal(encryptedValue);
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "descrypt failed", e);
        }
    }
}
