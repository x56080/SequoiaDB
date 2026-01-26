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

   Source File Name = ScheduleNodeRequiredParamGroup.java

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
import com.sequoiadb.schedule.tools.exception.ScheduleBaseExitCode;
import com.sequoiadb.schedule.tools.exception.ScheduleToolsException;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

public class ScheduleNodeRequiredParamGroup {
    private List<ScheduleNodeRequiredParam> requiredParams = new ArrayList<>();

    ScheduleNodeRequiredParamGroup() {
    }

    public void addParam(ScheduleNodeRequiredParam param) {
        requiredParams.add(param);
    }

    public void check(Properties clientKeyValue) throws ScheduleToolsException {
        Set<String> clientKeys = clientKeyValue.stringPropertyNames();
        Set<String> lostKeys = new HashSet<>(); // createnode时缺少的参数集合

        for (ScheduleNodeRequiredParam param : requiredParams) {
            if (!param.isPrefixKey()) {
                // param 是一个完整 key，直接查询命令行是否携带该 key
                if (!clientKeys.contains(param.getKey())) {
                    lostKeys.add(param.getKey());
                }
                continue;
            }

            if (param.getBindingKey() != null) {
                // param 是一个 preKey（如 -Dk1）, 拿到它关联 key 的 value （如 v1）
                // 然后再校验命令行是否携带 -Dk1.v1
                String bindingKeyValue = clientKeyValue.getProperty(param.getBindingKey());
                if (bindingKeyValue == null) {
                    lostKeys.add(param.getBindingKey());
                    continue;
                }
                String completedKey = param.getKey() + bindingKeyValue;
                if (!clientKeys.contains(completedKey)) {
                    lostKeys.add(completedKey);
                }
            } else {
                boolean found = false;
                for (String clientKey : clientKeys) {
                    if (clientKey.startsWith(param.getKey())) {
                        // 如果命令行参数中有以 param.getKey() 开头的 key，则不需要检查
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    lostKeys.add(param.getKey());
                }
            }
        }

        if (lostKeys.size() > 0) {
            StringBuilder sb = new StringBuilder();
            for (String lostKey : lostKeys) {
                sb.append("-D" + lostKey).append(", ");
            }
            throw new ScheduleToolsException(
                    "missing properties:key=" + sb.delete(sb.length() - 2, sb.length()).toString(),
                    ScheduleBaseExitCode.INVALID_ARG);
        }
    }

    public void check(Map<String, String> clientKeyValue) throws ScheduleToolsException {
        Properties nodeConf = new Properties();
        for (Map.Entry<String, String> entry : clientKeyValue.entrySet()) {
            nodeConf.setProperty(entry.getKey(), entry.getValue());
        }
        check(nodeConf);
    }

    public List<String> getExample() {
        List<String> example = new ArrayList<>();
        for (ScheduleNodeRequiredParam param : requiredParams) {
            example.add(param.getExample());
        }
        return example;
    }

    public static Builder newBuilder() {
        return new Builder();
    }

    public static class Builder {

        private final ScheduleNodeRequiredParamGroup paramGroup;

        Builder() {
            this.paramGroup = new ScheduleNodeRequiredParamGroup();
        }

        public ScheduleNodeRequiredParamGroup get() {
            return paramGroup;
        }
    }
}