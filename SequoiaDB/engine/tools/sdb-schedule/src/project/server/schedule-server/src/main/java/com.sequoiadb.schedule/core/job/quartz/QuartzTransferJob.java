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

   Source File Name = QuartzTransferJob.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.job.quartz;

import com.sequoiadb.schedule.common.ScheduleDefine;
import com.sequoiadb.schedule.core.job.ScheduleJobInfo;
import com.sequoiadb.schedule.core.job.TransferJobInfo;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import com.sequoiadb.schedule.model.TaskEntity;

import java.util.Date;
import java.util.UUID;

public class QuartzTransferJob extends QuartzSdbScheduleJob {

    @Override
    protected TaskEntity createTaskEntity(ServerNodeEntity runTaskServer, ScheduleJobInfo info)
            throws ScheduleServerException {
        TransferJobInfo tInfo = (TransferJobInfo) info;
        Date d = new Date();
        String taskId = UUID.randomUUID().toString();
        return QuartzScheduleTools.createTask(ScheduleDefine.TaskType.TRANSFER, taskId,
                tInfo.getContent(), runTaskServer.getUrl(), info.getId(), d.getTime());
    }
}
