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

   Source File Name = CommonDefine.java

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

public class CommonDefine {
    // request headers
    public static final String AUTHORIZATION = "Authorization";
    public static final String X_AMZ_CONTENT_SHA256 = "x-amz-content-sha256";

    /** request path */
    public static final String PATH_REGION = "/region/";

    /** request parameters */
    public static final String ACTION = "Action";
    public static final String REGION_NAME = "RegionName";

    /** values of parameter "Action" */
    public static final String CREATE_REGION = "CreateRegion";
    public static final String DELETE_REGION = "DeleteRegion";
    public static final String GET_REGION = "GetRegion";
    public static final String HEAD_REGION = "HeadRegion";
    public static final String LIST_REGION = "ListRegions";

    /** request method */
    public static final String HTTP_METHOD_POST = "POST";

    /** S3 service */
    public static final String SERVICE_NAME = "s3";
}
