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

   Source File Name = ScheduleController.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.controller;

import com.sequoiadb.schedule.common.BsonUtils;
import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import com.sequoiadb.schedule.service.ScheduleService;
import com.sequoiadb.schedule.service.TaskService;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import javax.servlet.http.HttpServletResponse;
import java.util.Collections;
import java.util.List;

@RestController
@RequestMapping("/api")
public class ScheduleController {

    private static final Logger logger = LoggerFactory.getLogger(ScheduleController.class);

    @Autowired
    private ScheduleService scheduleService;

    @Autowired
    private TaskService taskService;

    @PostMapping("/v1/schedules")
    public ScheduleFullEntity createSchedule(@RequestBody ScheduleUserEntity info)
            throws Exception {
        if (info == null) {
            throw new IllegalArgumentException("schedule info is null");
        }
        return scheduleService.createSchedule(info);
    }

    @PutMapping("/v1/schedules/{schedule_id}")
    public ScheduleFullEntity updateSchedule(@PathVariable("schedule_id") String scheduleId,
            @RequestBody ScheduleUserEntity info) throws Exception {
        if (info == null) {
            throw new IllegalArgumentException("new schedule info is null");
        }
        return scheduleService.updateSchedule(scheduleId, info);
    }

    @PutMapping(value = "/v1/schedules/{schedule_id}/switch")
    public ScheduleFullEntity switchSchedule(@PathVariable("schedule_id") String scheduleId,
            @RequestParam("enable") boolean enable) throws Exception {
        return scheduleService.switchSchedule(scheduleId, enable);
    }

    @DeleteMapping("/v1/schedules/{schedule_id}")
    public void deleteSchedule(@PathVariable("schedule_id") String scheduleId) throws Exception {
        if (scheduleId == null || scheduleId.isEmpty()) {
            throw new IllegalArgumentException("schedule id is null or empty");
        }
        scheduleService.deleteSchedule(scheduleId);
    }

    @GetMapping("/v1/schedules")
    public List<BSONObject> listSchedule(
            @RequestParam(value = "skip", required = false, defaultValue = "0") long skip,
            @RequestParam(value = "limit", required = false, defaultValue = "1000") long limit,
            @RequestParam(value = "filter", required = false) BSONObject filter,
            @RequestParam(value = "orderby", required = false) BSONObject orderBy,
            HttpServletResponse response) throws Exception {
        if (null == filter) {
            filter = new BasicBSONObject();
        }
        long scheduleCount = scheduleService.getScheduleCount(filter);
        response.setHeader("x-record-count", String.valueOf(scheduleCount));
        if (scheduleCount <= 0) {
            return Collections.emptyList();
        }
        return scheduleService.getScheduleList(filter, orderBy, skip, limit);
    }

    @GetMapping("/v1/schedules/{schedule_id}")
    public ScheduleFullEntity getSchedule(@PathVariable("schedule_id") String scheduleId)
            throws Exception {
        if (scheduleId == null || scheduleId.isEmpty()) {
            throw new IllegalArgumentException("schedule id is null or empty");
        }
        return scheduleService.getSchedule(scheduleId);
    }

    @GetMapping("/v1/schedules/tasks")
    public List<BSONObject> listTask(
            @RequestParam(value = "schedule_id", required = true) String scheduleId,
            @RequestParam(value = "filter", required = false) BSONObject filter,
            @RequestParam(value = "skip", required = false, defaultValue = "0") long skip,
            @RequestParam(value = "limit", required = false, defaultValue = "1000") long limit,
            @RequestParam(value = "orderby", required = false) BSONObject orderBy,
            HttpServletResponse response) throws Exception {
        if (null == filter) {
            filter = new BasicBSONObject();
        }
        BasicBSONList bsonList = new BasicBSONList();
        bsonList.add(new BasicBSONObject("schedule_id", scheduleId));
        bsonList.add(filter);
        BSONObject taskCountFilter = new BasicBSONObject("$and", bsonList);
        long taskCount = taskService.getTaskCount(taskCountFilter);
        response.setHeader("x-record-count", String.valueOf(taskCount));
        if (taskCount <= 0) {
            return Collections.emptyList();
        }
        return taskService.listTasks(taskCountFilter, orderBy, skip, limit);
    }

    @PostMapping("/v1/schedules/previewCSCLMatch")
    public BSONObject previewCSCLMatch(@RequestBody BSONObject filter) throws Exception{
        String site = BsonUtils.getString(filter, "site");
        if (site == null || site.isEmpty()) {
            throw new IllegalArgumentException("site is null or empty");
        }

        List<String> csRegexList = BsonUtils.getStringArray(filter, "cs_regex");
        List<String> clRegexList = BsonUtils.getStringArray(filter, "cl_regex");

        if ((csRegexList == null || csRegexList.isEmpty()) && (clRegexList == null || clRegexList.isEmpty())) {
            throw new IllegalArgumentException("cs_regex and cl_regex can not both be null or empty");
        }

        return scheduleService.previewCSCLMatch(site, csRegexList, clRegexList);
    }
}
