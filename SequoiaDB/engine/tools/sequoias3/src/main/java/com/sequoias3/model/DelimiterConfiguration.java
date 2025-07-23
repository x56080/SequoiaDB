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

   Source File Name = DelimiterConfiguration.java

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
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

@JacksonXmlRootElement(localName ="DelimiterConfiguration")
public class DelimiterConfiguration {
    @JsonProperty("Delimiter")
    private String delimiter;
    @JsonProperty("Status")
    private String status;

    public DelimiterConfiguration(){}

    public DelimiterConfiguration(String delimiter, String status, String encodingType) throws S3ServerException{
        try {
            if (encodingType != null) {
                this.delimiter = URLEncoder.encode(delimiter, "UTF-8");
            }else {
                this.delimiter = delimiter;
            }
            this.status = status;
        }catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.BUCKET_DELIMITER_GET_FAILED, "encode delimiter failed", e);
        }
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public String getStatus() {
        return status;
    }
}
