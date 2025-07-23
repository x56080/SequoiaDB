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

   Source File Name = FilterRecord.java

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

import com.sequoias3.model.*;

public class FilterRecord {
    public static final int FILTER_DIR          = 1;
    public static final int FILTER_DELIMITER    = 2;
    public static final int FILTER_NO_DELIMITER = 3;

    public static final int CONTENT      = 1;
    public static final int VERSION      = 2;
    public static final int DELETEMARKER = 3;
    public static final int COMMONPREFIX = 4;
    public static final int UPLOAD       = 5;

    private int recordType;
    private CommonPrefix commonPrefix;
    private Version version;
    private RawVersion deleteMarker;
    private Content content;
    private Upload upload;

    public void setRecordType(int recordType) {
        this.recordType = recordType;
    }

    public int getRecordType() {
        return recordType;
    }

    public void setDeleteMarker(RawVersion deleteMarker) {
        this.deleteMarker = deleteMarker;
    }

    public RawVersion getDeleteMarker() {
        return deleteMarker;
    }

    public void setCommonPrefix(CommonPrefix commonPrefix) {
        this.commonPrefix = commonPrefix;
    }

    public CommonPrefix getCommonPrefix() {
        return commonPrefix;
    }

    public void setVersion(Version version) {
        this.version = version;
    }

    public Version getVersion() {
        return version;
    }

    public void setContent(Content content) {
        this.content = content;
    }

    public Content getContent() {
        return content;
    }

    public void setUpload(Upload upload) {
        this.upload = upload;
    }

    public Upload getUpload() {
        return upload;
    }
}
