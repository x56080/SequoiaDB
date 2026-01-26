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

   Source File Name = ScheduleCoreAutoConfig.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core;

import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.dao.CollectionDataSwitchEventDao;
import com.sequoiadb.schedule.dao.CollectionSnapshotRecordDao;
import com.sequoiadb.schedule.dao.CollectionTransferRecordStatusDao;
import com.sequoiadb.schedule.dao.ScheduleDao;
import com.sequoiadb.schedule.dao.TaskDao;
import com.sequoiadb.schedule.dao.TaskProgressDao;
import com.sequoiadb.schedule.dao.TransactionFactory;
import com.sequoiadb.schedule.dao.TransactionLockDao;
import com.sequoiadb.schedule.lock.TransactionLockFactory;
import com.sequoiadb.schedule.remote.FeignClientFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Lazy;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;

@Component
public class ScheduleCoreAutoConfig {
    @Autowired
    private TaskDao taskDao;

    @Autowired
    private FeignClientFactory feignClientFactory;

    @Autowired
    private ScheduleNodeMgr scheduleNodeMgr;

    @Autowired
    private SiteMgr siteMgr;

    @Autowired
    private ScheduleDao scheduleDao;

    @Autowired
    @Lazy
    private LeaderElect leaderElect;

    @Autowired
    private SiteDataserviceMgr siteDataserviceMgr;

    @Autowired
    private TransactionFactory transactionFactory;

    @Autowired
    private TransactionLockDao transactionLockDao;

    @Autowired
    private TransactionLockFactory transactionLockFactory;

    @Autowired
    private CollectionSnapshotRecordDao collectionSnapshotRecordDao;

    @Autowired
    private CollectionDataSwitchEventDao collectionDataSwitchEventDao;

    @Autowired
    private CollectionTransferRecordStatusDao collectionTransferRecordStatusDao;

    @Autowired
    private TaskProgressDao taskProgressDao;

    @PostConstruct
    public void init() {
        ScheduleMgrWrapper.getInstance().init(scheduleDao, leaderElect);
        ScheduleServer.getInstance().init(taskDao, feignClientFactory, scheduleNodeMgr, siteMgr,
                siteDataserviceMgr, transactionFactory, transactionLockDao,
                collectionSnapshotRecordDao, collectionDataSwitchEventDao, transactionLockFactory,
                collectionTransferRecordStatusDao, taskProgressDao);
        TaskStatusMgr.getInstance().init(taskDao, transactionLockFactory);
    }
}
