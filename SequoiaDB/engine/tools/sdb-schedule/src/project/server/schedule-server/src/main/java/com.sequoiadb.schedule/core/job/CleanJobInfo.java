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

   Source File Name = CleanJobInfo.java

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

import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.model.SiteInfo;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.springframework.util.CollectionUtils;

import java.util.List;

public class CleanJobInfo extends ScheduleJobInfo{
    private BSONObject content;

    public CleanJobInfo(String id, String type, String cron, BSONObject content) throws Exception {
        super(id, type, cron);
        this.content = content;
        checkAndParse(ScheduleServer.getInstance());
    }

    private void checkAndParse(ScheduleServer server) throws Exception {
        BasicBSONList cleanRangeList = BsonUtils.getArray(this.content, FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE);
        if (cleanRangeList == null || cleanRangeList.isEmpty()) {
            throw new IllegalArgumentException("clean_range can not be null or empty");
        }

        for (Object o : cleanRangeList) {
            BSONObject range = (BSONObject) o;
            String cleanSite = BsonUtils.getString(range, FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE);
            String cleanSiteRegex = BsonUtils.getString(range, FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CLEAN_SITE_REGEX);
            List<String> clList = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CL_LIST);
            List<String> csRegex = BsonUtils.getStringArray(range, FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CS_REGEX);
            List<String> csList = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CS_LIST);
            List<String> clRegex = BsonUtils.getStringArray(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_CL_REGEX);
            Integer maxRetentionDays = BsonUtils.getIntegerOrElse(range,
                    FieldName.Schedule.FIELD_CONTENT_CLEAN_RANGE_MAX_RETENTIONS_DAYS, 7);

            if (cleanSite == null && cleanSiteRegex == null) {
                throw new IllegalArgumentException("cleanSite and cleanSiteRegex can not both be null");
            }

            if (cleanSite != null) {
                if (cleanSite.isEmpty()) {
                    throw new IllegalArgumentException("cleanSite can not be empty");
                }
                else {
                    SiteInfo site = server.getSite(cleanSite);
                    if (site == null) {
                        throw new ScheduleServerException(ScheduleServerError.SITE_NOT_EXISTS,
                                "clean site is not exist, cleanSite: " + cleanSite);
                    }
                }
            }

            if (cleanSiteRegex != null) {
                if (cleanSiteRegex.isEmpty()) {
                    throw new IllegalArgumentException("cleanSiteRegex can not be empty");
                }
                else {
                    if (!isRegexValid(cleanSiteRegex)) {
                        throw new IllegalArgumentException("the cleanSiteRegex is not a valid regex, cleanSiteRegex: " + cleanSiteRegex);
                    }
                }
            }

            if (CollectionUtils.isEmpty(csList) && CollectionUtils.isEmpty(csRegex)
                    && CollectionUtils.isEmpty(clList) && CollectionUtils.isEmpty(clRegex)) {
                throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                        "the cs_list, cs_regex, cl_list, cl_regex can not be both empty");
            }

            if (!CollectionUtils.isEmpty(csRegex)) {
                for (String regex : csRegex) {
                    if (!isRegexValid(regex)) {
                        throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                                "have a invalid regex value in the cs_regex, value: " + regex);
                    }
                }
            }

            if (!CollectionUtils.isEmpty(clRegex)) {
                for (String regex : clRegex) {
                    if (!isRegexValid(regex)) {
                        throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                                "have a invalid regex value in the cl_regex, value: " + regex);
                    }
                }
            }

            if (maxRetentionDays < 0) {
                throw new IllegalArgumentException("maxRetentionDays must not be less than 0, maxRetentionDays: " + maxRetentionDays);
            }
        }

        long maxExecTime = BsonUtils.getLongOrElse(content, FieldName.Schedule.FIELD_MAX_EXEC_TIME,
                0);
        if (maxExecTime < 0) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                    "max exec time must not be less than 0, maxExecTime: " + maxExecTime);
        }
    }

    public BSONObject getContent() {
        return content;
    }
}
