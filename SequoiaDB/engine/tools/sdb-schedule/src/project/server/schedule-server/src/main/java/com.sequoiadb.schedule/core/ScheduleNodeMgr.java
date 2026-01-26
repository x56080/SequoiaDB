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

   Source File Name = ScheduleNodeMgr.java

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

import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.config.HostInfoConfig;
import com.sequoiadb.schedule.dao.ScheduleNodeDao;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ThreadLocalRandom;

@Component
public class ScheduleNodeMgr {
    private static final Logger logger = LoggerFactory.getLogger(ScheduleNodeMgr.class);
    private Timer renewTimer;

    @Autowired
    private HostInfoConfig hostInfo;

    @Autowired
    private ScheduleNodeDao scheduleNodeDao;

    private int renewInterval = 15; // seconds

    private final static int RETRY_INTERVAL = 1000 * 3;

    private Timer retryTimer;

    private ServerNodeEntity localServer;

    private Timer refreshTimer;

    // key: ip:port
    private final Map<String, ServerNodeEntity> ipAliveNodeMap = new ConcurrentHashMap<>();
    // key: hostname:port
    private final Map<String, ServerNodeEntity> hostnameAliveNodeMap = new ConcurrentHashMap<>();
    private final Map<String, ServerNodeEntity> mayAbnormalNodeMap = new ConcurrentHashMap<>();

    @PostConstruct
    public void init() {
        try {
            register();
        }
        catch (Exception e) {
            logger.error("failed to register", e);
            asyncRetryRegister();
        }
    }

    private void asyncRetryRegister() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryRegisterTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to start register");
                    register();
                    cancel();
                }
                catch (Exception e) {
                    logger.warn("retry start register failed", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    /** 注册节点信息到数据库 */
    private void register() throws Exception {
        String ipAddress = hostInfo.getIpAddress();
        String hostname = hostInfo.getHostname();
        int serverPort = hostInfo.getServerPort();

        ServerNodeEntity server = new ServerNodeEntity();
        server.setIpAddress(ipAddress);
        server.setHostName(hostname);
        server.setPort(serverPort);
        server.setStatus("UP");
        server.setLeaseNum(1); // 初始续约号
        server.setLastHeartTime(System.currentTimeMillis());
        this.localServer = server;
        scheduleNodeDao.upsert(server);
        logger.info("Node registered: {}:{}", hostname, hostname);

        // 注册成功后启动续约任务
        startRenewTask();

        // 启动刷新在线节点任务
        startRefreshAliveNodesTask();
    }

    private void startRefreshAliveNodesTask() {
        if (refreshTimer != null) {
            refreshTimer.cancel();
            refreshTimer = null;
        }
        refreshTimer = TimerFactory.createTimer("refreshAliveNodesTimer");
        long interval = renewInterval * 1000L;

        refreshTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    List<ServerNodeEntity> serverNodeEntityList = scheduleNodeDao.list();
                    for (ServerNodeEntity server : serverNodeEntityList) {
                        String status = server.getStatus();
                        String ipPortKey = server.getIpAddress() + ":" + server.getPort();
                        String hostnamePortKey = server.getHostName() + ":" + server.getPort();
                        if (status.equals("DOWN")) {
                            ipAliveNodeMap.remove(ipPortKey);
                            hostnameAliveNodeMap.remove(hostnamePortKey);
                        }
                        else {
                            if (ipAliveNodeMap.get(ipPortKey) == null
                                    || hostnameAliveNodeMap.get(hostnamePortKey) == null) {
                                ServerNodeEntity abnormalNode = mayAbnormalNodeMap.get(ipPortKey);
                                if (abnormalNode == null) {
                                    ipAliveNodeMap.put(ipPortKey, server);
                                    hostnameAliveNodeMap.put(hostnamePortKey, server);
                                }
                                else {
                                    if (abnormalNode.getLeaseNum() != server.getLeaseNum()) {
                                        // 续约号变化，说明节点恢复正常
                                        ipAliveNodeMap.put(ipPortKey, server);
                                        hostnameAliveNodeMap.put(hostnamePortKey, server);
                                        mayAbnormalNodeMap.remove(ipPortKey);
                                    } // else 续约号没有变化，继续保持异常状态
                                }
                            }
                            else {
                                ServerNodeEntity entity = ipAliveNodeMap.get(ipPortKey) == null
                                        ? hostnameAliveNodeMap.get(hostnamePortKey)
                                        : ipAliveNodeMap.get(ipPortKey);
                                if (entity.getLeaseNum() != server.getLeaseNum()) {
                                    ipAliveNodeMap.put(ipPortKey, server);
                                    hostnameAliveNodeMap.put(hostnamePortKey, server);
                                }
                                else {
                                    // 续约号相同，说明节点没有更新，有可能节点异常掉线
                                    ipAliveNodeMap.remove(ipPortKey);
                                    hostnameAliveNodeMap.remove(hostnamePortKey);
                                    mayAbnormalNodeMap.put(ipPortKey, server);
                                }
                            }
                        }
                    }
                }
                catch (Exception e) {
                    logger.error("refresh alive nodes failed", e);
                }
            }
        }, 0, interval + 3000);
    }

    /** 启动定时续约号自增任务 */
    private void startRenewTask() {
        if (renewTimer != null) {
            renewTimer.cancel();
            renewTimer = null;
        }
        renewTimer = TimerFactory.createTimer("scheduleNodeRenewTimer");
        long interval = renewInterval * 1000L;
        renewTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    renew(localServer);
                    localServer.incLeaseNum();
                }
                catch (Exception e) {
                    logger.warn("renew failed", e);
                }
            }
        }, interval, interval);
    }

    private void renew(ServerNodeEntity server) throws Exception {
        scheduleNodeDao.renew(server);
    }

    public synchronized ServerNodeEntity getRandomAliveNode() {
        if (ipAliveNodeMap.isEmpty()) {
            return localServer;
        }
        int size = ipAliveNodeMap.size();
        if (size == 1) {
            return ipAliveNodeMap.values().iterator().next();
        }
        int index = ThreadLocalRandom.current().nextInt(size);
        return ipAliveNodeMap.values().stream().skip(index).findFirst().orElse(null);
    }

    public ServerNodeEntity getServerNode(String key) {
        ServerNodeEntity server = ipAliveNodeMap.get(key);
        if (server == null) {
            server = hostnameAliveNodeMap.get(key);
        }
        return server;
    }

    public ServerNodeEntity getLocalServer() {
        return localServer;
    }

    @PreDestroy
    public void destroy() {
        if (renewTimer != null) {
            renewTimer.cancel();
        }
        if (retryTimer != null) {
            retryTimer.cancel();
        }
        if (refreshTimer != null) {
            refreshTimer.cancel();
        }
        ipAliveNodeMap.clear();
        hostnameAliveNodeMap.clear();
        if (localServer == null) {
            return;
        }
        try {
            logger.info("schedule node destroy, update status to down");
            // 节点停止时，sequoiadb 的数据源会自动关闭，因此需要使用临时数据连接进行操作
            scheduleNodeDao.useTempConnection();
            scheduleNodeDao.down(localServer);
        }
        catch (Exception e) {
            logger.warn("update status info failed", e);
        }
    }
}
