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

   Source File Name = ScheduleEntityTranslator.java

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

import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Date;

public class ScheduleEntityTranslator {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleEntityTranslator.class);

    public static class FullInfo {
        public static ScheduleFullEntity fromUserInfo(ScheduleUserEntity userInfo, String id,
                long createTime) {
            ScheduleFullEntity fullEntity = new ScheduleFullEntity();
            fullEntity.setId(id);
            fullEntity.setName(userInfo.getName());
            fullEntity.setDesc(userInfo.getDesc());
            fullEntity.setType(userInfo.getType());
            fullEntity.setContent(userInfo.getContent());
            fullEntity.setCron(userInfo.getCron());
            fullEntity.setEnable(userInfo.isEnable());
            fullEntity.setCreateTime(createTime);
            fullEntity.setUpdateTime(createTime);
            return fullEntity;
        }

        public static ScheduleFullEntity updateInfo(ScheduleUserEntity userInfo, String id,
                long createTime) {
            ScheduleFullEntity fullEntity = new ScheduleFullEntity();
            fullEntity.setId(id);
            fullEntity.setName(userInfo.getName());
            fullEntity.setDesc(userInfo.getDesc());
            fullEntity.setType(userInfo.getType());
            fullEntity.setContent(userInfo.getContent());
            fullEntity.setCron(userInfo.getCron());
            fullEntity.setEnable(userInfo.isEnable());
            fullEntity.setCreateTime(createTime);
            fullEntity.setUpdateTime(new Date().getTime());
            return fullEntity;
        }

        public static BSONObject toBSONObject(ScheduleFullEntity info) {
            BSONObject obj = new BasicBSONObject();
            obj.put(FieldName.Schedule.FIELD_ID, info.getId());
            obj.put(FieldName.Schedule.FIELD_NAME, info.getName());
            obj.put(FieldName.Schedule.FIELD_DESC, info.getDesc());
            obj.put(FieldName.Schedule.FIELD_TYPE, info.getType());
            obj.put(FieldName.Schedule.FIELD_CONTENT, info.getContent());
            obj.put(FieldName.Schedule.FIELD_CRON, info.getCron());
            obj.put(FieldName.Schedule.FIELD_CREATE_TIME, info.getCreateTime());
            obj.put(FieldName.Schedule.FIELD_UPDATE_TIME, info.getUpdateTime());
            obj.put(FieldName.Schedule.FIELD_ENABLE, info.isEnable());
            return obj;
        }

        public static ScheduleFullEntity fromBSONObject(BSONObject obj) throws Exception {
            ScheduleFullEntity info = new ScheduleFullEntity();
            try {
                info.setId((String) obj.get(FieldName.Schedule.FIELD_ID));
                info.setName((String) obj.get(FieldName.Schedule.FIELD_NAME));
                info.setDesc((String) obj.get(FieldName.Schedule.FIELD_DESC));
                info.setType((String) obj.get(FieldName.Schedule.FIELD_TYPE));
                info.setContent((BSONObject) obj.get(FieldName.Schedule.FIELD_CONTENT));
                info.setCron((String) obj.get(FieldName.Schedule.FIELD_CRON));
                info.setCreateTime(BsonUtils.getLong(obj, FieldName.Schedule.FIELD_CREATE_TIME));
                info.setUpdateTime(BsonUtils.getLong(obj, FieldName.Schedule.FIELD_UPDATE_TIME));
                Object enable = obj.get(FieldName.Schedule.FIELD_ENABLE);
                if (enable != null) {
                    info.setEnable((boolean) enable);
                }
            }
            catch (Exception e) {
                logger.error("translate BSONObject to ScheduleFullInfo failed:obj={}", obj);
                throw e;
            }

            return info;
        }
    }
}
