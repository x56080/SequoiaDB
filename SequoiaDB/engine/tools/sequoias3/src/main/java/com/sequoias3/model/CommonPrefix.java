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

   Source File Name = CommonPrefix.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

public class CommonPrefix {
    @JsonProperty("Prefix")
    private String prefix;

    public CommonPrefix(String prefix, String encodingType) throws S3ServerException{
        try {
            if (null != encodingType) {
                this.prefix = URLEncoder.encode(prefix, "UTF-8");
            }else {
                this.prefix = prefix;
            }
        }catch (UnsupportedEncodingException e){
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setPrefix(String prefix) {
        this.prefix = prefix;
    }

    public String getPrefix() {
        return prefix;
    }

    @Override
    public boolean equals(Object o){
        CommonPrefix inItem = (CommonPrefix) o;
        return prefix.equals(inItem.prefix);
    }

    @Override
    public int hashCode(){
        return prefix.hashCode();
    }
}
