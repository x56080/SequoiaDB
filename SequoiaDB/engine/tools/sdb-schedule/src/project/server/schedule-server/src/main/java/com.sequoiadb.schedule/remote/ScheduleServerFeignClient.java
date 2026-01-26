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

   Source File Name = ScheduleServerFeignClient.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.remote;

import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestParam;

public interface ScheduleServerFeignClient {

    @PostMapping("/api/v1/schedules")
    public ScheduleFullEntity createSchedule(@RequestBody ScheduleUserEntity info)
            throws RemoteServiceException;

    @PutMapping("/api/v1/schedules/{schedule_id}")
    public ScheduleFullEntity updateSchedule(@PathVariable("schedule_id") String scheduleId,
            @RequestBody ScheduleUserEntity info) throws RemoteServiceException;

    @PutMapping(value = "/api/v1/schedules/{schedule_id}/switch")
    public ScheduleFullEntity switchSchedule(@PathVariable("schedule_id") String scheduleId,
            @RequestParam("enable") boolean enable) throws RemoteServiceException;

    @DeleteMapping("/api/v1/schedules/{schedule_id}")
    public void deleteSchedule(@PathVariable("schedule_id") String scheduleId)
            throws RemoteServiceException;

    @PostMapping(value = "/api/v1/tasks/{taskId}/notify")
    public void notifyTask(@PathVariable("taskId") String taskId,
            @RequestParam("notify_type") int notifyType);

}
