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

   Source File Name = CryptoHelper.java

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

public class CryptoHelper {
    public static String bytesToHexStr(byte[] b) {
        if (null == b) {
            return "";
        }

        StringBuilder buf = new StringBuilder();
        for (int i = 0; i < b.length; i++) {
            int x = b[i] & 0xFF;
            String s = Integer.toHexString(x).toUpperCase();
            if (s.length() == 1) {
                buf.append("0");
            }

            buf.append(s);
        }

        return buf.toString();
    }

    public static byte[] hexStrToBytes(String s) {
        final int len = s.length();
        if (len % 2 != 0) {
            throw new IllegalArgumentException("input is not a avaliable hex string:s=" + s);
        }

        int length = s.length() / 2;
        byte b[] = new byte[length];
        for (int i = 0; i < b.length; i++) {
            String tmp = s.substring(i * 2, i * 2 + 2);
            if (!isHexStr(tmp)) {
                throw new IllegalArgumentException("input is not a avaliable hex string:s=" + s);
            }

            b[i] = (byte) Integer.parseInt(tmp, 16);
        }

        return b;
    }

    private static boolean isHexStr(String s) {
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c >= '0' && c <= '9') {
                continue;
            }

            if (c >= 'a' && c <= 'f') {
                continue;
            }

            if (c >= 'A' && c <= 'F') {
                continue;
            }

            return false;
        }

        return true;
    }
}
