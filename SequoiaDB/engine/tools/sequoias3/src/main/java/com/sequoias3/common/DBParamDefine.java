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

   Source File Name = DBParamDefine.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoias3.common;

public class DBParamDefine {
    public static final int DB_DUPLICATE_MAX_TIME = 10000;

    public static final String DB_AUTO_DELIMITER = "/";

    public static final String CS_S3   = "S3_";
    public static final String CS_META = "_MetaCS";
    public static final String CS_DATA = "_DataCS";

    public static final String MODIFY_SET   = "$set";
    public static final String MODIFY_UNSET = "$unset";
    public static final String GREATER      = "$gt";
    public static final String NOT_SMALL    = "$gte";
    public static final String LESS_THAN    = "$lt";
    public static final String INCREASE     = "$inc";
    public static final String IN           = "$in";
    public static final String OR           = "$or";
    public static final String NOT_EQUAL    = "$ne";
    public static final String IS_NULL      = "$isnull";

    public static final int    CREATE_OK     = 1;
    public static final int    CREATE_EXIST  = 2;
}
