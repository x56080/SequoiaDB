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

   Source File Name = ScheduleJobInfo.java

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
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.apache.logging.log4j.util.Strings;
import org.bson.BSONObject;
import org.springframework.util.CollectionUtils;

import java.util.List;
import java.util.regex.Pattern;

@Data
@NoArgsConstructor
@AllArgsConstructor
public class ScheduleJobInfo {
    private String id;
    private String type;
    private String cron;

    protected void commonCheckArg(ScheduleServer server, BSONObject content) throws Exception {
        String sourceSite = BsonUtils.getStringChecked(content,
                FieldName.Schedule.FIELD_CONTENT_SOURCE_SITE);
        SiteInfo sourceSiteInfo = server.getSite(sourceSite);
        if (sourceSiteInfo == null) {
            throw new ScheduleServerException(ScheduleServerError.SITE_NOT_EXISTS,
                    "source site is not exist, sourceSite: " + sourceSite);
        }

        String targetSite = BsonUtils.getStringChecked(content,
                FieldName.Schedule.FIELD_CONTENT_TARGET_SITE);
        SiteInfo targetSiteInfo = server.getSite(targetSite);
        if (targetSiteInfo == null) {
            throw new ScheduleServerException(ScheduleServerError.SITE_NOT_EXISTS,
                    "target site is not exist, targetSite: " + targetSite);
        }

        String datasource = targetSiteInfo.getDatasource();
        if (Strings.isEmpty(datasource)) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                    "the target site is not a datasouce site, targetSite: " + targetSite);
        }

        List<String> csList = BsonUtils.getStringArray(content,
                FieldName.Schedule.FIELD_CONTENT_CS_LIST);
        List<String> csRegex = BsonUtils.getStringArray(content,
                FieldName.Schedule.FIELD_CONTENT_CS_REGEX);
        List<String> clList = BsonUtils.getStringArray(content,
                FieldName.Schedule.FIELD_CONTENT_CL_LIST);
        List<String> clRegex = BsonUtils.getStringArray(content,
                FieldName.Schedule.FIELD_CONTENT_CL_REGEX);
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

        int noWriteTimeThreshold = BsonUtils.getIntegerOrElse(content,
                FieldName.Schedule.FIELD_CONTENT_NO_WRITE_TIME_THRESHOLD, 30);
        if (noWriteTimeThreshold < 0) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                    "no write time threshold must not be less than 0, noWriteTimeThreshold: "
                            + noWriteTimeThreshold);
        }

        int clCreateTimeThreshold = BsonUtils.getIntegerOrElse(content,
                FieldName.Schedule.FIELD_CONTENT_CL_CREATE_TIME_THRESHOLD, 30);
        if (clCreateTimeThreshold < 0) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                    "cl create time threshold must not be less than 0, clCreateTimeThreshold: "
                            + clCreateTimeThreshold);
        }

        long maxExecTime = BsonUtils.getLongOrElse(content, FieldName.Schedule.FIELD_MAX_EXEC_TIME,
                0);
        if (maxExecTime < 0) {
            throw new ScheduleServerException(ScheduleServerError.INVALID_ARG,
                    "max exec time must not be less than 0, maxExecTime: " + maxExecTime);
        }

    }

    public boolean isRegexValid(String regex) {
        try {
            Pattern.compile(regex);
            return true;
        }
        catch (Exception e) {
            return false;
        }

    }
}
