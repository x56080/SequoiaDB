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

   Source File Name = ScheduleServiceImpl.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.service.impl;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.schedule.common.SdbHelper;
import com.sequoiadb.schedule.core.DataServiceWrapper;
import com.sequoiadb.schedule.core.ScheduleMgrWrapper;
import com.sequoiadb.schedule.core.ScheduleServer;
import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.dao.ScheduleDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleSystemException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.ScheduleFullEntity;
import com.sequoiadb.schedule.model.ScheduleUserEntity;
import com.sequoiadb.schedule.remote.FeignClientFactory;
import com.sequoiadb.schedule.remote.ScheduleServerFeignClient;
import com.sequoiadb.schedule.service.ScheduleService;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.util.CollectionUtils;
import org.springframework.util.StringUtils;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

@Service
public class ScheduleServiceImpl implements ScheduleService {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleServiceImpl.class);
    @Autowired
    private LeaderElect leaderElect;

    @Autowired
    private FeignClientFactory feignClientFactory;

    @Autowired
    private ScheduleDao scheduleDao;

    @Override
    public ScheduleFullEntity createSchedule(ScheduleUserEntity info) throws Exception {
        leaderElect.checkLeaderState();
        if (!leaderElect.isLeader()) {
            return getLeaderFeignClient().createSchedule(info);
        }
        return ScheduleMgrWrapper.getInstance().createSchedule(info);
    }

    @Override
    public ScheduleFullEntity updateSchedule(String scheduleId, ScheduleUserEntity newInfo)
            throws Exception {
        leaderElect.checkLeaderState();
        if (!leaderElect.isLeader()) {
            return getLeaderFeignClient().updateSchedule(scheduleId, newInfo);
        }
        return ScheduleMgrWrapper.getInstance().updateSchedule(scheduleId, newInfo);
    }

    @Override
    public ScheduleFullEntity switchSchedule(String scheduleId, boolean enable) throws Exception {
        leaderElect.checkLeaderState();
        if (!leaderElect.isLeader()) {
            return getLeaderFeignClient().switchSchedule(scheduleId, enable);
        }
        return ScheduleMgrWrapper.getInstance().switchSchedule(scheduleId, enable);
    }

    @Override
    public BSONObject previewCSCLMatch(String site, List<String> csRegexList, List<String> clRegexList) throws ScheduleSystemException {
        try {
            DataServiceWrapper dataService = ScheduleServer.getInstance().getDataServiceWrapper(site);
            Sequoiadb sdb = null;
            try {
                sdb = dataService.getConnection();
                Set<String> csSet = null;
                if (!CollectionUtils.isEmpty(csRegexList)) {
                    csSet = new HashSet<>(SdbHelper.getCSList(sdb, csRegexList));
                }
                Set<String> clSet = null;
                if (!CollectionUtils.isEmpty(clRegexList)) {
                    clSet = new HashSet<>(SdbHelper.getCLList(sdb, clRegexList));
                }

                BSONObject res = new BasicBSONObject();
                res.put("cs", csSet);
                res.put("cl", clSet);
                return res;
            }
            finally {
                if (sdb != null) {
                    dataService.releaseConnection(sdb);
                }
            }
        }
        catch (Exception e) {
            throw new ScheduleSystemException("failed to preview cs cl match, site: " + site + ", csRegex: " + csRegexList + ", clRegex: " + clRegexList, e);
        }
    }

    @Override
    public void deleteSchedule(String scheduleId) throws Exception {
        leaderElect.checkLeaderState();
        if (!leaderElect.isLeader()) {
            getLeaderFeignClient().deleteSchedule(scheduleId);
        }
        else {
            ScheduleMgrWrapper.getInstance().deleteSchedule(scheduleId);
        }
    }

    @Override
    public long getScheduleCount(BSONObject filter) throws Exception {
        return scheduleDao.countSchedule(filter);
    }

    @Override
    public List<BSONObject> getScheduleList(BSONObject condition, BSONObject orderby, long skip,
            long limit) throws Exception {
        List<BSONObject> result = new ArrayList<>();
        MetaCursor cursor = null;
        try {
            cursor = scheduleDao.listSchedule(condition, orderby, skip, limit);
            while (cursor.hasNext()) {
                BSONObject obj = cursor.getNext();
                result.add(obj);
            }
            return result;
        }
        finally {
            if (null != cursor) {
                cursor.close();
            }
        }
    }

    @Override
    public ScheduleFullEntity getSchedule(String scheduleId) throws Exception {
        return ScheduleMgrWrapper.getInstance().getSchedule(scheduleId);
    }

    private ScheduleServerFeignClient getLeaderFeignClient() throws ScheduleServerException {
        String leaderNodeUrl = leaderElect.getLeaderNodeUrl();
        if (leaderElect.getLocalNodeUrl().equals(leaderNodeUrl)) {
            throw new ScheduleServerException(ScheduleServerError.LEADER_NOT_PREPARE,
                    "leader node not prepare, leaderNodeUrl=" + leaderNodeUrl);
        }
        logger.debug("redirect to leader node, leaderNodeUrl=" + leaderNodeUrl);
        ScheduleServerFeignClient client = feignClientFactory
                .getClient(ScheduleServerFeignClient.class, leaderNodeUrl);
        return client;
    }
}
