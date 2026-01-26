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

   Source File Name = LeaderElect.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.core.elect;

import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.config.ElectionConfig;
import com.sequoiadb.schedule.config.HostInfoConfig;
import com.sequoiadb.schedule.dao.LeaderElectionDao;
import com.sequoiadb.schedule.exception.ScheduleServerError;
import com.sequoiadb.schedule.exception.ScheduleServerException;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.model.ScheduleServerLeaderInfo;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.DependsOn;
import org.springframework.stereotype.Component;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.util.concurrent.TimeUnit;

@Component
@DependsOn("scheduleCoreAutoConfig")
public class LeaderElect {
    private static final Logger logger = LoggerFactory.getLogger(LeaderElect.class);
    private static final String ROLE_LEADER = "Leader";
    private static final String ROLE_FOLLOWER = "Follower";
    @Autowired
    private ElectionConfig electionConfig;

    @Autowired
    private LeaderElectionDao leaderElectionDao;

    private volatile String leaderNodeUrl;
    private volatile long lastLeaderLeaseNum;
    private volatile Long lastLeaderActiveNanoTime;
    private final String localNodeUrl;
    private String nodeRole;
    private volatile boolean isWorking = false;
    private LeaderAction leaderAction;
    private NotLeaderAction notLeaderAction;
    private Timer leaderRenewTimer;
    private Timer reElectTimer;
    private volatile long leaderExpireNanoTime;
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;

    public LeaderElect(HostInfoConfig hostInfo) {
        this.localNodeUrl = hostInfo.getHostAndPort();
        this.leaderAction = new ScheduleLeaderAction();
        this.notLeaderAction = new ScheduleNotLeaderAction();
    }

    @PostConstruct
    public void init() {
        try {
            startElect();
        }
        catch (Exception e) {
            logger.error("failed to start elect", e);
            asyncRetryStartElect();
        }
    }

    private void asyncRetryStartElect() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryStartElectTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to start elect");
                    startElect();
                    cancel();
                }
                catch (Exception e) {
                    logger.warn("retry start elect failed", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    public boolean startElect() throws ScheduleServerMetaSourceException {
        logger.debug("start elect");
        ScheduleServerLeaderInfo beforeLeader = leaderElectionDao.queryOne();
        // 1. 表记录不存在，说明现在没有主节点
        if (beforeLeader == null) {
            return tryToBecomeLeader(null);
        }
        // 2. 当前有主节点，需要检测主节点是否过期
        else {
            // 如果主节点正常停止，则认为主节点已经过期，可以正常抢主
            if (beforeLeader.getLeaseNum() == -1) {
                return tryToBecomeLeader(beforeLeader);
            }

            // 检测主节点是否过期
            if (lastLeaderActiveNanoTime != null) {
                updateLeaderInfo(beforeLeader);
                long duration = System.nanoTime() - lastLeaderActiveNanoTime;
                if (duration > TimeUnit.SECONDS.toNanos(electionConfig.getLeaderDuration())
                        && beforeLeader.getNodeUrl().equals(leaderNodeUrl)) {
                    return tryToBecomeLeader(beforeLeader);
                }
                else {
                    return false;
                }
            }
            else {
                // 节点第一次启动，没有记录上次主节点活跃时间
                // a.如果表里记录的主节点是当前节点，直接抢主
                // b.如果是其它节点，则先做为备节点，等待主节点过期
                if (beforeLeader.getNodeUrl().equals(localNodeUrl)) {
                    return tryToBecomeLeader(beforeLeader);
                }
                else {
                    onBecomeFollower(beforeLeader);
                    return false;
                }
            }
        }
    }

    private boolean tryToBecomeLeader(ScheduleServerLeaderInfo oldLeaderInfo)
            throws ScheduleServerMetaSourceException {
        updateLeaderInfo(null);
        if (oldLeaderInfo == null) {
            try {
                ScheduleServerLeaderInfo leaderInfo = new ScheduleServerLeaderInfo(localNodeUrl, 1,
                        System.currentTimeMillis());
                leaderElectionDao.insert(leaderInfo);
                // 插入成功，当前节点作为主节点
                onBecomeLeader(leaderInfo);
                return true;
            }
            catch (ScheduleServerMetaSourceException e) {
                if (e.getError() == ScheduleServerError.DUPLICATE_RECORD) {
                    onBecomeFollower(leaderElectionDao.queryOne());
                    return false;
                }
                throw e;
            }
        }
        else {
            ScheduleServerLeaderInfo newLeaderInfo = new ScheduleServerLeaderInfo(localNodeUrl, 1,
                    System.currentTimeMillis());
            ScheduleServerLeaderInfo returnNew = leaderElectionDao.updateAndReturnNew(newLeaderInfo,
                    oldLeaderInfo);
            if (newLeaderInfo.equals(returnNew)) {
                // 抢主成功，当前节点是主节点
                onBecomeLeader(newLeaderInfo);
                return true;
            }
            else {
                // 当前节点作为备节点
                onBecomeFollower(leaderElectionDao.queryOne());
                return false;
            }
        }
    }

    private void onBecomeLeader(ScheduleServerLeaderInfo leaderInfo) {
        if (ROLE_LEADER.equals(nodeRole)) {
            // 理论上不会出现这种情况,如果出现则主动切换为备节点
            logger.warn(
                    "current node is already leader, switch to follower, leaderNodeUrl={}, localNodeUrl={}",
                    leaderInfo.getNodeUrl(), localNodeUrl);
            onBecomeFollower(leaderInfo);
            return;
        }
        logger.info("current node become leader role: {}", leaderInfo.getNodeUrl());
        logger.info("clear lock info and wait all lock expired");
        isWorking = false;
        nodeRole = ROLE_LEADER;
        leaderExpireNanoTime = System.nanoTime()
                + TimeUnit.SECONDS.toNanos(electionConfig.getLeaderDuration());
        updateLeaderInfo(leaderInfo);
        leaderAction.run();
        isWorking = true;
        // 主节点启动一个续约任务
        setupRenewTask();
    }

    private void setupRenewTask() {
        if (leaderRenewTimer != null) {
            leaderRenewTimer.cancel();
            leaderRenewTimer = null;
        }
        if (reElectTimer != null) {
            reElectTimer.cancel();
            reElectTimer = null;
        }
        leaderRenewTimer = TimerFactory.createTimer("leaderRenewTimer");
        long interval = electionConfig.getRenewInterval() * 1000L;
        leaderRenewTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    renewLeader();
                }
                catch (Exception e) {
                    logger.warn("leader renew failed", e);
                }
            }
        }, interval, interval);
    }

    private void renewLeader() throws ScheduleServerMetaSourceException {
        long startNanoTime = System.nanoTime();
        boolean success = doRenew();
        if (success) {
            // 当主节点长期续约不上时，需要主动退主，因此需要记录这个过期时间
            this.leaderExpireNanoTime = startNanoTime
                    + TimeUnit.SECONDS.toNanos(electionConfig.getLeaderDuration());
        }
    }

    private boolean doRenew() throws ScheduleServerMetaSourceException {
        if (System.nanoTime() > leaderExpireNanoTime) {
            logger.warn("leader expired");
            onBecomeFollower(null);
            return false;
        }
        ScheduleServerLeaderInfo beforeLeaderInfo = leaderElectionDao.queryOne();
        if (beforeLeaderInfo == null) {
            // should not happen
            logger.warn("leader info not found");
            onBecomeFollower(null);
            return false;
        }
        if (!beforeLeaderInfo.getNodeUrl().equals(localNodeUrl)) {
            logger.warn("leader change, leaderNodeUrl={}, localNodeUrl={}",
                    beforeLeaderInfo.getNodeUrl(), localNodeUrl);
            onBecomeFollower(beforeLeaderInfo);
            return false;
        }

        ScheduleServerLeaderInfo returnNew = leaderElectionDao.renew(beforeLeaderInfo);
        if (returnNew != null && returnNew.getLeaseNum() == beforeLeaderInfo.getLeaseNum() + 1) {
            logger.debug("leader renew success, leaseNum={}, updateTime={}",
                    returnNew.getLeaseNum(), returnNew.getUpdateTime());
            updateLeaderInfo(returnNew);
            return true;
        }
        else {
            logger.warn("renew failed, returnNewLeaderInfo={}", returnNew);
            onBecomeFollower(null);
            return false;
        }
    }

    // 作为备节点时会调用这个函数
    private void onBecomeFollower(ScheduleServerLeaderInfo leaderInfo) {
        logger.info("current node as follower role: leaderNodeUrl={}, localNodeUrl={}",
                leaderInfo != null ? leaderInfo.getNodeUrl() : null, localNodeUrl);
        this.nodeRole = ROLE_FOLLOWER;
        this.isWorking = false;
        notLeaderAction.run();
        updateLeaderInfo(leaderInfo);
        setupReElectTask();
    }

    private void setupReElectTask() {
        if (leaderRenewTimer != null) {
            leaderRenewTimer.cancel();
            leaderRenewTimer = null;
        }
        if (reElectTimer != null) {
            reElectTimer.cancel();
            reElectTimer = null;
        }
        reElectTimer = TimerFactory.createTimer("reElectTimer");
        long interval = electionConfig.getRenewInterval() * 1000L;
        reElectTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    startElect();
                }
                catch (Exception e) {
                    logger.warn("reElect failed", e);
                }
            }
        }, interval, interval);
    }

    private void updateLeaderInfo(ScheduleServerLeaderInfo leaderInfo) {
        if (leaderInfo == null) {
            leaderNodeUrl = null;
            lastLeaderLeaseNum = 0;
            lastLeaderActiveNanoTime = null;
            return;
        }
        if (lastLeaderLeaseNum != leaderInfo.getLeaseNum()) {
            lastLeaderLeaseNum = leaderInfo.getLeaseNum();
            lastLeaderActiveNanoTime = System.nanoTime();
        }
        leaderNodeUrl = leaderInfo.getNodeUrl();
    }

    @PreDestroy
    public void destroy() {
        if (leaderRenewTimer != null) {
            leaderRenewTimer.cancel();
        }
        if (reElectTimer != null) {
            reElectTimer.cancel();
        }
        if (retryTimer != null) {
            retryTimer.cancel();
        }
        if (isLeader()) {
            try {
                logger.info("leader node destroy, update leaseNum to -1");
                // 节点停止时，sequoiadb 的数据源会自动关闭，因此需要使用临时数据连接进行操作
                leaderElectionDao.useTempConnection();
                ScheduleServerLeaderInfo leaderInfo = leaderElectionDao.queryOne();
                if (leaderInfo != null && leaderInfo.getNodeUrl().equals(localNodeUrl)) {
                    ScheduleServerLeaderInfo newLeaderInfo = new ScheduleServerLeaderInfo(
                            localNodeUrl, -1, System.currentTimeMillis());
                    ScheduleServerLeaderInfo returnNew = leaderElectionDao
                            .updateAndReturnNew(newLeaderInfo, leaderInfo);
                    if (newLeaderInfo.equals(returnNew)) {
                        logger.info("update leaseNum to -1 success");
                    }
                }
            }
            catch (Exception e) {
                logger.warn("update leader info failed", e);
            }
        }
    }

    // 主动退主：外部可调用
    public synchronized void resignLeader() {
        if (!isLeader()) {
            logger.info("resignLeader called but node is not leader: {}", localNodeUrl);
            return;
        }
        logger.info("Leader {} voluntarily resigns", localNodeUrl);
        try {
            doResignLeader();
        }
        catch (Exception e) {
            throw new RuntimeException("resignLeader failed", e);
        }
    }

    // 实际执行退主和降级
    private void doResignLeader() throws ScheduleServerMetaSourceException {
        // 1. 停止续约任务
        if (leaderRenewTimer != null) {
            leaderRenewTimer.cancel();
            leaderRenewTimer = null;
        }

        // 2. 更新数据库，leaseNum = -1 表示主动退出
        ScheduleServerLeaderInfo leaderInfo = leaderElectionDao.queryOne();
        if (leaderInfo != null && leaderInfo.getNodeUrl().equals(localNodeUrl)) {
            ScheduleServerLeaderInfo newLeaderInfo = new ScheduleServerLeaderInfo(localNodeUrl, -1,
                    System.currentTimeMillis());
            leaderElectionDao.updateAndReturnNew(newLeaderInfo, leaderInfo);
        }

        // 3. 本地角色降为 follower，并启动重新选举
        onBecomeFollower(null);
        logger.info("Node {} has resigned and is now follower", localNodeUrl);
    }

    public void checkLeaderState() throws ScheduleServerException {
        if (leaderNodeUrl == null) {
            throw new ScheduleServerException(ScheduleServerError.NO_LEADER_NODE,
                    "leader node not exist");
        }
        if (isLeader() && !isWorking) {
            throw new ScheduleServerException(ScheduleServerError.LEADER_NOT_PREPARE,
                    "leader node not prepare: leaderNodeUrl=" + leaderNodeUrl);
        }
    }

    public boolean isLeader() {
        return ROLE_LEADER.equals(nodeRole);
    }

    public String getLeaderNodeUrl() throws ScheduleServerException {
        if (leaderNodeUrl == null) {
            throw new ScheduleServerException(ScheduleServerError.NO_LEADER_NODE,
                    "leader node not exist");
        }
        return leaderNodeUrl;
    }

    public String getLocalNodeUrl() {
        return localNodeUrl;
    }
}
