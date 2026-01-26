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

   Source File Name = NodeServiceImpl.java

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

import com.sequoiadb.schedule.core.ScheduleNodeMgr;
import com.sequoiadb.schedule.core.elect.LeaderElect;
import com.sequoiadb.schedule.dao.ScheduleNodeDao;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import com.sequoiadb.schedule.service.NodeService;
import org.bson.BSONObject;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;

@Service
public class NodeServiceImpl implements NodeService {

    @Autowired
    private ScheduleNodeDao scheduleNodeDao;

    @Autowired
    private ScheduleNodeMgr scheduleNodeMgr;

    @Autowired
    private LeaderElect leaderElect;

    @Override
    public List<BSONObject> getNodeList(BSONObject condition, BSONObject orderby, long skip,
            long limit) throws Exception {
        List<ServerNodeEntity> list = scheduleNodeDao.listNode(condition, orderby, skip, limit);
        List<BSONObject> res = new ArrayList<>();
        for (ServerNodeEntity entity : list) {
            BSONObject object = entity.toBSONObject();
            String leaderNodeUrl = leaderElect.getLeaderNodeUrl();
            String nodeIpUrl = entity.getIpAddress() + ":" + entity.getPort();
            String nodeHostUrl = entity.getHostName() + ":" + entity.getPort();
            if (nodeIpUrl.equals(leaderNodeUrl) || nodeHostUrl.equals(leaderNodeUrl)) {
                object.put("is_leader", true);
            }
            else {
                object.put("is_leader", false);
            }

            if (entity.getStatus().equals("UP")) {
                ServerNodeEntity serverNode = scheduleNodeMgr.getServerNode(nodeIpUrl);
                if (serverNode == null) {
                    serverNode = scheduleNodeMgr.getServerNode(nodeHostUrl);
                }
                if (serverNode == null) {
                    object.put("status", "ABNORMAL");
                }
            }
            res.add(object);
        }
        return res;
    }

    @Override
    public long getNodeCount(BSONObject filter) throws Exception {
        return scheduleNodeDao.countNode(filter);
    }
}
