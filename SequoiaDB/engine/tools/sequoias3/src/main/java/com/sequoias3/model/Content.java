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

   Source File Name = Content.java

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

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sequoias3.core.ObjectMeta;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import static com.sequoias3.utils.DataFormatUtils.formatDate;

@JsonInclude(value = JsonInclude.Include.NON_NULL)
public class Content {
    @JsonProperty("Key")
    private String key;
    @JsonProperty("LastModified")
    private String lastModified;
    @JsonProperty("ETag")
    private String eTag;
    @JsonProperty("Size")
    private long   size;
    @JsonProperty("Owner")
    private Owner owner;

    public Content(){}

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setLastModified(String lastModified) {
        this.lastModified = lastModified;
    }

    public String getLastModified() {
        return lastModified;
    }

    public void seteTag(String eTag) {
        this.eTag = eTag;
    }

    public String geteTag() {
        return eTag;
    }

    public void setSize(long size) {
        this.size = size;
    }

    public long getSize() {
        return size;
    }

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public Owner getOwner() {
        return owner;
    }

    public Content (BSONObject bsonObject, String encodingType)
            throws S3ServerException {
        try {
            if (null != encodingType) {
                this.key = URLEncoder.encode(bsonObject.get(ObjectMeta.META_KEY_NAME).toString(), "UTF-8");
            } else {
                this.key = bsonObject.get(ObjectMeta.META_KEY_NAME).toString();
            }
            this.lastModified = formatDate((long) bsonObject.get(ObjectMeta.META_LAST_MODIFIED));
            this.eTag         = bsonObject.get(ObjectMeta.META_ETAG).toString();
            this.size         = (long) bsonObject.get(ObjectMeta.META_SIZE);
        }catch (UnsupportedEncodingException e){
            //logger.error("Encode object name failed. e", e);
            throw new S3ServerException(S3Error.UNKNOWN_ERROR,
                    "encode object name failed."+e.getMessage(), e);
        }
    }
}
