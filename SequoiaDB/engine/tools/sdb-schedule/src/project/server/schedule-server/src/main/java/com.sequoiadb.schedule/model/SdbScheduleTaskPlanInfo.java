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

   Source File Name = SdbScheduleTaskPlanInfo.java

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

import com.sequoiadb.schedule.common.FieldName;
import lombok.Data;
import org.bson.BSONObject;

import java.lang.reflect.Field;
import java.util.Set;

@Data
public class SdbScheduleTaskPlanInfo {

    private Set<String> csSet;
    private Set<String> clSet;

    public BSONObject toBSONObject() {
        BSONObject obj = new org.bson.BasicBSONObject();
        obj.put(FieldName.Task.FIELD_PLAN_CS, csSet);
        obj.put(FieldName.Task.FIELD_PLAN_CL, clSet);
        return obj;
    }
}
