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

   Source File Name = ScheduleServerError.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.exception;

public enum ScheduleServerError {
    NO_LEADER_NODE(-200, "no leader node"),
    LEADER_NOT_PREPARE(-201, "leader node not prepare"),
    RECORD_NOT_EXISTS(-202, "record not exists"),
    META_SOURCE_ERROR(-300, "meta source error"),
    DUPLICATE_RECORD(-301, "duplicate record"),
    INTERNAL_ERROR(-302, "internal error"),
    SYSTEM_ERROR(-303, "system error"),
    SITE_NOT_EXISTS(-304, "site not exists"),
    INVALID_ARG(-305, "invalid arg"),
    TRANSACTION_LOCK_TIMEOUT(-306, "transaction lock timeout"),
    UNKNOWN_ERROR(0, "unknown error");

    private final int errorCode;
    private final String desc;

    ScheduleServerError(int errorCode, String desc) {
        this.errorCode = errorCode;
        this.desc = desc;
    }

    public int getErrorCode() {
        return errorCode;
    }

    public String getDesc() {
        return desc;
    }

    @Override
    public String toString() {
        return name() + "(" + this.errorCode + ")" + ":" + this.desc;
    }

    public static ScheduleServerError getScheduleServerError(int errorCode) {
        for (ScheduleServerError value : ScheduleServerError.values()) {
            if (value.getErrorCode() == errorCode) {
                return value;
            }
        }

        return UNKNOWN_ERROR;
    }
}
