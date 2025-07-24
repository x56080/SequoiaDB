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

   Source File Name = Upload.java

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
import com.sequoias3.core.UploadMeta;
import com.sequoias3.exception.S3Error;
import com.sequoias3.exception.S3ServerException;
import org.bson.BSONObject;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;

import static com.sequoias3.utils.DataFormatUtils.formatDate;

public class Upload {
    @JsonProperty("Key")
    private String key;

    @JsonProperty("UploadId")
    private long uploadId;

    @JsonProperty("Initiator")
    Owner initiator;

    @JsonProperty("Owner")
    Owner owner;

    @JsonProperty("Initiated")
    private String formatDate;

    public Upload(BSONObject record, String encodingtype, Owner owner) throws S3ServerException{
        try {
            if (record.get(UploadMeta.META_KEY_NAME) != null){
                if (encodingtype != null) {
                    this.key = URLEncoder.encode(record.get(UploadMeta.META_KEY_NAME).toString(), "UTF-8");
                }else {
                    this.key = record.get(UploadMeta.META_KEY_NAME).toString();
                }
            }

            if (record.get(UploadMeta.META_UPLOAD_ID) != null){
                this.uploadId = (long) record.get(UploadMeta.META_UPLOAD_ID);
            }

            if (record.get(UploadMeta.META_INIT_TIME) != null){
                this.formatDate = formatDate((long) record.get(UploadMeta.META_INIT_TIME));
            }

            this.owner     = owner;
            this.initiator = owner;
        }catch (UnsupportedEncodingException e) {
            throw new S3ServerException(S3Error.UNKNOWN_ERROR, "URL encode failed", e);
        }
    }

    public void setUploadId(long uploadId) {
        this.uploadId = uploadId;
    }

    public long getUploadId() {
        return uploadId;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getKey() {
        return key;
    }

    public void setOwner(Owner owner) {
        this.owner = owner;
    }

    public Owner getOwner() {
        return owner;
    }

    public void setFormatDate(String formatDate) {
        this.formatDate = formatDate;
    }

    public String getFormatDate() {
        return formatDate;
    }

    public void setInitiator(Owner initiator) {
        this.initiator = initiator;
    }

    public Owner getInitiator() {
        return initiator;
    }
}
