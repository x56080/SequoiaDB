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

   Source File Name = GlobalConfEnum.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.model;

import com.sequoiadb.schedule.common.ScheduleDefine;

import java.util.HashSet;
import java.util.Set;

public enum GlobalConfEnum {
    TASK_RECORD_RETENTION_DAYS(ScheduleDefine.GlobalConfKey.KEY_TASK_RECORD_RETENTION_DAYS, String.valueOf(30),
            "任务历史运行记录最大保留时间，单位：天",
            new NumberConfValidator(ScheduleDefine.GlobalConfKey.KEY_TASK_RECORD_RETENTION_DAYS));


    private String keyName;
    private String defaultValue;
    private String desc;
    private GlobalConfValidator validator;

    GlobalConfEnum(String keyName, String defaultValue, String desc, GlobalConfValidator validator) {
        this.keyName = keyName;
        this.defaultValue = defaultValue;
        this.desc = desc;
        this.validator = validator;
    }

    public String getKeyName() {
        return keyName;
    }

    public String getDefaultValue() {
        return defaultValue;
    }

    public String getDesc() {
        return desc;
    }

    public GlobalConfValidator getValidator() {
        return validator;
    }

    public static GlobalConfEnum getType(String keyName) {
        for (GlobalConfEnum value : values()) {
            if (value.getKeyName().equals(keyName)) {
                return value;
            }
        }
        throw new IllegalArgumentException("unsupported conf key: " + keyName);
    }

    public static Set<String> getKeys() {
        Set<String> set = new HashSet<>();
        for (GlobalConfEnum value : values()) {
            set.add(value.getKeyName());
        }
        return set;
    }

}
