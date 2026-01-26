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

   Source File Name = ScheduleNodeType.java

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

import java.util.Objects;

public class ScheduleNodeType {
    private ScheduleNodeTypeEnum typeEnum;
    private String jarNamePrefix;
    private String confTemplateNamePrefix = "spring-app";
    // 该属性用于提醒用户，新添加一个工具时，要向 ScheduleServerScriptEnum 类中添加新工具的脚本信息
    private ScheduleServerScriptEnum serverScriptEnum;

    // 创建该节点时，是否需要生成Hystrix配置
    private boolean isNeedHystrixConf = true;

    public ScheduleNodeType(ScheduleNodeTypeEnum scheduleNodeTypeEnum, ScheduleServerScriptEnum serverScriptEnum) {
        this(scheduleNodeTypeEnum, serverScriptEnum, true);
    }

    public ScheduleNodeType(ScheduleNodeTypeEnum scheduleNodeTypeEnum, ScheduleServerScriptEnum serverScriptEnum,
            boolean isNeedHystrixConf) {
        this.typeEnum = scheduleNodeTypeEnum;
        this.jarNamePrefix = scheduleNodeTypeEnum.getJarNamePrefix();
        this.serverScriptEnum = serverScriptEnum;
        this.isNeedHystrixConf = isNeedHystrixConf;
    }

    public ScheduleNodeType(ScheduleNodeTypeEnum scheduleNodeTypeEnum, ScheduleServerScriptEnum serverScriptEnum,
            String confTemplateNamePrefix) {
        this(scheduleNodeTypeEnum, serverScriptEnum);
        this.confTemplateNamePrefix = confTemplateNamePrefix;
    }

    public String getType() {
        return typeEnum.getTypeNum();
    }

    public String getName() {
        return typeEnum.getName();
    }

    public ScheduleNodeTypeEnum getTypeEnum() {
        return typeEnum;
    }

    public String getJarNamePrefix() {
        return jarNamePrefix;
    }

    public String getConfTemplateNamePrefix() {
        return confTemplateNamePrefix;
    }

    public String getUpperName() {
        return getName().toUpperCase();
    }

    public boolean isNeedHystrixConf() {
        return isNeedHystrixConf;
    }

    public String getServiceDirName() {
        return serverScriptEnum.getDirName();
    }

    @Override
    public String toString() {
        return this.getType();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o == null || getClass() != o.getClass())
            return false;
        ScheduleNodeType that = (ScheduleNodeType) o;
        return isNeedHystrixConf == that.isNeedHystrixConf && typeEnum == that.typeEnum
                && Objects.equals(jarNamePrefix, that.jarNamePrefix)
                && Objects.equals(confTemplateNamePrefix, that.confTemplateNamePrefix)
                && serverScriptEnum == that.serverScriptEnum;
    }

    @Override
    public int hashCode() {
        return Objects.hash(typeEnum, jarNamePrefix, confTemplateNamePrefix, serverScriptEnum,
                isNeedHystrixConf);
    }
}
