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

   Source File Name = ScheduleNodeRequiredParam.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.tools.element;

public class ScheduleNodeRequiredParam {
    private String key;
    private String example;

    // isPrefixKey 为 true 表示字段 key 是一个前缀，
    // 他需要与 bindingKey 的 value 结合起来组成一个完整 key
    // 如 key = k1.， bindingKey = k2 ， -Dk2=v2， 那么 k1 完整
    // 的表示是 -Dk1.v2
    private boolean isPrefixKey;
    private String bindingKey;

    private ScheduleNodeRequiredParam(String key, String example, boolean isPrefixKey,
            String bindingKey) {
        this.key = key;
        this.example = example;
        this.isPrefixKey = isPrefixKey;
        this.bindingKey = bindingKey;
    }

    public static ScheduleNodeRequiredParam keyParamInstance(String key, String example) {
        return new ScheduleNodeRequiredParam(key, example, false, null);
    }

    public static ScheduleNodeRequiredParam preKeyParamInstance(String preKey, String example,
            String bindingKey) {
        return new ScheduleNodeRequiredParam(preKey, example, true, bindingKey);
    }

    public String getKey() {
        return key;
    }

    public String getExample() {
        return example;
    }

    public boolean isPrefixKey() {
        return isPrefixKey;
    }

    public String getBindingKey() {
        return bindingKey;
    }
}
