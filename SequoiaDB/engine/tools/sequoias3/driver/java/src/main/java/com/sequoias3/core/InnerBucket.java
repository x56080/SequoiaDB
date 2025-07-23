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

   Source File Name = InnerBucket.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.core;

import com.fasterxml.jackson.annotation.JsonProperty;

public class InnerBucket {
    public static final String BUCKET_NAME                 = "Name";
    public static final String BUCKET_CREATETIME           = "CreationDate";

    @JsonProperty(BUCKET_NAME)
    private String bucketName;

    @JsonProperty(BUCKET_CREATETIME)
    private String formatDate;

    public void setBucketName(String bucketName){
        this.bucketName = bucketName;
    }

    public String getBucketName(){
        return this.bucketName;
    }

    public void setFormatDate(String creationDate){
        this.formatDate = creationDate;
    }

    public String getFormatDate(){
        return this.formatDate;
    }
}
