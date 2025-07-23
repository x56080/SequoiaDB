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

   Source File Name = MD5Utils.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.utils;

import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.apache.commons.codec.binary.Hex;
import sun.misc.BASE64Decoder;

public class MD5Utils {
    public static Boolean isMd5EqualWithETag(String contentMd5, String eTag) throws S3ServerException {
        try {
            if(contentMd5.length() % 4 != 0){
                throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                        "decode md5 failed, contentMd5:"+contentMd5);
            }
            BASE64Decoder decoder = new BASE64Decoder();
            String textMD5 = new String(Hex.encodeHex(decoder.decodeBuffer(contentMd5)));
            if (textMD5.equals(eTag)){
                return true;
            }else {
                return false;
            }
        }catch (Exception e){
            throw new S3ServerException(S3Error.OBJECT_INVALID_DIGEST,
                    "decode md5 failed, contentMd5:"+contentMd5, e);
        }
    }
}
