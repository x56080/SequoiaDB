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

   Source File Name = DesCrypto.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common.crypto;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;
import javax.crypto.spec.IvParameterSpec;

public class DesCrypto implements Crypto {

    @Override
    public byte[] encrypt(byte[] orignal, byte[] key)  {
        return encryptOrDecrypt(orignal, key, true);
    }

    @Override
    public byte[] decrypt(byte[] encrypted, byte[] key)  {
        return encryptOrDecrypt(encrypted, key, false);
    }

    private byte[] encryptOrDecrypt(byte[] contents, byte[] key, boolean isEncrypt) {
        try {
            int mode = Cipher.ENCRYPT_MODE;
            if (!isEncrypt) {
                mode = Cipher.DECRYPT_MODE;
            }

            DESKeySpec keySpec = new DESKeySpec(key);
            SecretKeyFactory factory = SecretKeyFactory.getInstance("DES");
            SecretKey secretKey = factory.generateSecret(keySpec);

            Cipher cipher = Cipher.getInstance("DES/CBC/PKCS5Padding");
            cipher.init(mode, secretKey, new IvParameterSpec(keySpec.getKey()));
            return cipher.doFinal(contents);
        }
        catch (Exception e) {
            throw new RuntimeException(isEncrypt ? "encrypt"
                    : "decrypt" + " failed:key=" + CryptoHelper.bytesToHexStr(key), e);
        }
    }
}
