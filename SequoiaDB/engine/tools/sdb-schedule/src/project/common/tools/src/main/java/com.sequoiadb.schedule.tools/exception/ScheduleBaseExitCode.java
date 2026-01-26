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

   Source File Name = ScheduleBaseExitCode.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.tools.exception;

public class ScheduleBaseExitCode {
    public static int SUCCESS = 0;
    // empty std out
    public static int EMPTY_OUT = 1;

    // common sys error >=3 and <60
    public static int INVALID_ARG = 3;
    public static int FILE_DELETE_ERROR = 4;
    public static int FILE_NOT_FIND = 4;
    public static int FILE_ALREADY_EXIST = 5;
    public static int PERMISSION_ERROR = 6;
    public static int SHELL_EXEC_ERROR = 7;
    public static int SYSTEM_ERROR = 8;
    public static int SCHEDULE_ALREADY_EXIST_ERROR = 9;
    public static int ALREADY_EXIST_ERROR = 62;

    // reserved exit code >=100 and <126

    // sdb error >=170 and <180
    public static int SDB_ERROR = 170;

    // private exit code >=180 and <255

    // max 255
    public static int MAX_VALUE = 255;
}
