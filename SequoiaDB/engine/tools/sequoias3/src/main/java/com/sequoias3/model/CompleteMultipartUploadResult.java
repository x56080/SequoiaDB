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

   Source File Name = CompleteMultipartUploadResult.java

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

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.dataformat.xml.annotation.JacksonXmlRootElement;

@JacksonXmlRootElement(localName = "CompleteMultipartUploadResult")
public class CompleteMultipartUploadResult {
    @JsonProperty("Location")
    private String location;

    @JsonProperty("Bucket")
    private String bucket;

    @JsonProperty("Key")
    private String Key;
    @JsonProperty("ETag")
    private String eTag;

    @JsonIgnore
    private Long versionId;

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public void setBucket(String bucket) {
        this.bucket = bucket;
    }

    public void setKey(String key) {
        Key = key;
    }

    public void setLocation(String location) {
        this.location = location;
    }

    public String geteTag() {
        return eTag;
    }

    public String getBucket() {
        return bucket;
    }

    public String getKey() {
        return Key;
    }

    public String getLocation() {
        return location;
    }

    public void setVersionId(Long versionId) {
        this.versionId = versionId;
    }

    public Long getVersionId() {
        return versionId;
    }
}

