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

   Source File Name = TaskEntityTranslator.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.common;

import com.sequoiadb.schedule.model.TaskEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TaskEntityTranslator {
    private static final Logger logger = LoggerFactory.getLogger(TaskEntityTranslator.class);

    public static TaskEntity fromBSONObject(BSONObject obj) throws Exception {
        TaskEntity info = new TaskEntity();
        try {
            info.setId((String) obj.get(FieldName.Task.FIELD_ID));
            info.setType(BsonUtils.getInteger(obj, FieldName.Task.FIELD_TYPE));
            info.setContent((BSONObject) obj.get(FieldName.Task.FIELD_CONTENT));
            info.setRunServer(BsonUtils.getString(obj, FieldName.Task.FIELD_RUN_SERVER));
            info.setRunningFlag(BsonUtils.getInteger(obj, FieldName.Task.FIELD_RUNNING_FLAG));
            info.setStartTime(BsonUtils.getLong(obj, FieldName.Task.FIELD_START_TIME));
            info.setStopTime(
                    BsonUtils.getNumberOrElse(obj, FieldName.Task.FIELD_STOP_TIME, -1).longValue());

            Object scheduleId = obj.get(FieldName.Task.FIELD_SCHEDULE_ID);
            if (null != scheduleId) {
                info.setScheduleId((String) scheduleId);
            }
        }
        catch (Exception e) {
            logger.error("translate BSONObject to TaskInfo failed:obj={}", obj);
            throw e;
        }

        return info;
    }

    public static BSONObject toBSONObject(TaskEntity info) {
        BSONObject obj = new BasicBSONObject();
        obj.put(FieldName.Task.FIELD_ID, info.getId());
        obj.put(FieldName.Task.FIELD_TYPE, info.getType());
        obj.put(FieldName.Task.FIELD_CONTENT, info.getContent());
        obj.put(FieldName.Task.FIELD_RUN_SERVER, info.getRunServer());
        obj.put(FieldName.Task.FIELD_RUNNING_FLAG, info.getRunningFlag());
        obj.put(FieldName.Task.FIELD_START_TIME, info.getStartTime());
        obj.put(FieldName.Task.FIELD_STOP_TIME, info.getStopTime());
        obj.put(FieldName.Task.FIELD_CREATE_TIME, info.getCreateTime());
        String scheduleId = info.getScheduleId();
        if (null != scheduleId) {
            obj.put(FieldName.Task.FIELD_SCHEDULE_ID, scheduleId);
        }
        return obj;
    }
}
