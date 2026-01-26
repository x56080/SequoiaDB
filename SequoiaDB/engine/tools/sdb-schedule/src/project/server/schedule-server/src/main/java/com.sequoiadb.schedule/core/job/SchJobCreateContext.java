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

   Source File Name = SchJobCreateContext.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

package com.sequoiadb.schedule.core.job;

import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;

import java.util.HashMap;
import java.util.Map;

public class SchJobCreateContext {
    private ScheduleJobInfo info;
    private Map<String, Object> contextData;

    public SchJobCreateContext(ScheduleJobInfo info) {
        this.info = info;
        this.contextData = new HashMap<>();
    }

    public ScheduleJobInfo getJobInfo() {
        return info;
    }

    public void set(String key, Object data) {
        contextData.put(key, data);
    }

    @SuppressWarnings("unchecked")
    public <T> T get(String key, Class<T> dataType) throws ScheduleServerException {
        T data = (T) contextData.get(key);
        if (data == null) {
            throw new ScheduleServerException(ScheduleServerError.INTERNAL_ERROR,
                    "no such context data:key=" + key + ", contextData=" + contextData);
        }
        return data;
    }
}
