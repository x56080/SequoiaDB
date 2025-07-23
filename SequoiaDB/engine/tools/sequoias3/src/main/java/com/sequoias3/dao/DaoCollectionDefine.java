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

   Source File Name = DaoCollectionDefine.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.dao;

public class DaoCollectionDefine {
    public static final String USER_LIST_COLLECTION      = "S3_User";
    public static final String BUCKET_LIST_COLLECTION    = "S3_Bucket";
    public static final String REGION_LIST_COLLECTION    = "S3_Region";
    public static final String REGION_SPACE_LIST         = "S3_RegionSpace";
    public static final String ID_GENERATOR              = "S3_IDGenerator";
    public static final String TASK_COLLECTION           = "S3_Task";
    public static final String UPLOAD_LIST               = "S3_UploadMeta";
    public static final String PART_LIST                 = "S3_Part";
    public static final String ACL_LIST                  = "S3_ACL";

    public static final String OBJECT_META_LIST          = "S3_ObjectMeta";
    public static final String OBJECT_META_LIST_HISTORY  = "S3_ObjectMetaHistory";
    public static final String OBJECT_DIR                = "S3_ObjectDir";

    public static final String OBJECT_DATA_LIST          = "S3_ObjectData";
}
