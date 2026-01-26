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

   Source File Name = TaskController.java

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

import com.sequoiadb.schedule.service.TaskService;
import org.bson.BSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
public class TaskController {

    @Autowired
    private TaskService taskService;

    @PostMapping(value = "/api/v1/tasks/{task_id}/notify")
    public void notifyTask(@PathVariable("task_id") String taskId,
            @RequestParam("notify_type") int notifyType) throws Exception {
        taskService.notifyTask(taskId, notifyType);
    }

    @GetMapping(value = "/api/v1/tasks/{task_id}/progress")
    public List<BSONObject> listTaskProgress(@PathVariable("task_id") String taskId)
            throws Exception {
        return taskService.listTaskProgress(taskId);
    }

    @PostMapping(value = "/api/v1/tasks/{task_id}/stop")
    public void stopTask(@PathVariable("task_id") String taskId) throws Exception {
        taskService.stopTask(taskId);
    }
}
