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

   Source File Name = ScheduleService.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.service;

import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import org.bson.BSONObject;

import java.util.List;

public interface ScheduleService {

    ScheduleFullEntity createSchedule(ScheduleUserEntity info) throws Exception;

    ScheduleFullEntity updateSchedule(String scheduleId, ScheduleUserEntity newInfo) throws Exception;

    void deleteSchedule(String scheduleId) throws Exception;

    long getScheduleCount(BSONObject filter) throws Exception;

    List<BSONObject> getScheduleList(BSONObject condition, BSONObject orderby,
            long skip, long limit) throws Exception;

    ScheduleFullEntity getSchedule(String scheduleId) throws Exception;

    ScheduleFullEntity switchSchedule(String scheduleId, boolean enable) throws Exception;

    BSONObject previewCSCLMatch(String site, List<String> csRegexList, List<String> clRegexList) throws ScheduleSystemException;
}
