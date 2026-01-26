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

   Source File Name = PasswordMgr.java

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

import java.util.Random;

public class PasswordMgr {

    public final static int CRYPT_TYPE_NONE = 0;
    public final static int CRYPT_TYPE_DES = 1;

    private final int BASEKEY_LENGTH = 2;
    private final int KEY_LENGTH = 8;

    private static PasswordMgr instance = new PasswordMgr();

    public static void main(String[] args) {
        System.out.println(instance.decrypt(1, "04C6EF29C00D2482028689FC0B4E9FAB2774011579B10144199A0269D162"));
    }

    private PasswordMgr() {
    }

    public static PasswordMgr getInstance() {
        return instance;
    }

    public String encrypt(int cryptType, String orignal) {
        if (cryptType == CRYPT_TYPE_NONE) {
            return orignal;
        }

        Crypto crypto = null;
        if (cryptType == CRYPT_TYPE_DES) {
            crypto = new DesCrypto();

            byte[] baseKey = generateBaseKey();
            byte[] key = generateKey(baseKey);

            byte[] encrypted = crypto.encrypt(orignal.getBytes(), key);

            StringBuilder sb = new StringBuilder();
            sb.append(CryptoHelper.bytesToHexStr(baseKey));
            sb.append(CryptoHelper.bytesToHexStr(encrypted));
            return sb.toString();
        }

        throw new IllegalArgumentException("invalid crypt type:type=" + cryptType);
    }

    public String decrypt(int cryptType, String encrypted) {
        if (cryptType == CRYPT_TYPE_NONE) {
            return encrypted;
        }

        int length = encrypted.length();
        Crypto crypto = null;
        if (cryptType == CRYPT_TYPE_DES) {
            int baseKeyStrLen = BASEKEY_LENGTH * 2;
            if (length < baseKeyStrLen) {
                throw new IllegalArgumentException("encrypted str is invalid:str=" + encrypted);
            }

            crypto = new DesCrypto();
            byte[] baseKey = CryptoHelper.hexStrToBytes(encrypted.substring(0, baseKeyStrLen));
            byte[] key = generateKey(baseKey);

            byte[] orignalEncrypted = CryptoHelper
                    .hexStrToBytes(encrypted.substring(baseKeyStrLen));
            byte[] result = crypto.decrypt(orignalEncrypted, key);

            return new String(result);
        }

        throw new IllegalArgumentException("invalid crypt type:type=" + cryptType);
    }

    private byte[] generateKey(byte[] baseKey) {
        byte[] key = new byte[KEY_LENGTH];

        int j = 0;
        for (int i = 0; i < key.length; i++) {
            if (j >= baseKey.length) {
                j = 0;
            }

            key[i] = baseKey[j];
            j++;
        }

        return key;
    }

    private byte[] generateBaseKey() {
        Random r = new Random();
        byte[] b = new byte[BASEKEY_LENGTH];
        r.nextBytes(b);

        return b;
    }
}
