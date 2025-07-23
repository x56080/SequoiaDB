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

   Source File Name = Dir.java

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

public class Dir {
    public static final String DIR_BUCKETID    = "BucketId";
    public static final String DIR_DELIMITER   = "Delimiter";
    public static final String DIR_NAME        = "Name";
    public static final String DIR_ID          = "ID";

    public static final String DIR_INDEX       = "dirIndex";

    private Long bucketId;
    private String delimiter;
    private String name;
    private Long ID;
//    private Boolean deleteMarker;

    public Dir(Long bucketId, String delimiter, String name, Long ID){
        this.bucketId     = bucketId;
        this.delimiter    = delimiter;
        this.name         = name;
        this.ID           = ID;
//        this.deleteMarker = deleteMarker;
    }

    public void setBucketId(Long bucketId) {
        this.bucketId = bucketId;
    }

    public Long getBucketId() {
        return bucketId;
    }

    public void setDelimiter(String delimiter) {
        this.delimiter = delimiter;
    }

    public String getDelimiter() {
        return delimiter;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public void setID(Long ID) {
        this.ID = ID;
    }

    public Long getID() {
        return ID;
    }

//    public void setDeleteMarker(Boolean deleteMarker) {
//        this.deleteMarker = deleteMarker;
//    }

//    public Boolean getDeleteMarker() {
//        return deleteMarker;
//    }
}
