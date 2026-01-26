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

   Source File Name = SdbScheduleNodeDao.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/01/2026  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.schedule.dao.sdb;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.schedule.common.FieldName;
import com.sequoiadb.schedule.common.timer.Timer;
import com.sequoiadb.schedule.common.timer.TimerFactory;
import com.sequoiadb.schedule.common.timer.TimerTask;
import com.sequoiadb.schedule.dao.ScheduleNodeDao;
import com.sequoiadb.schedule.exception.ScheduleServerMetaSourceException;
import com.sequoiadb.schedule.metasource.MetaCursor;
import com.sequoiadb.schedule.metasource.config.SequoiadbConfig;
import com.sequoiadb.schedule.metasource.template.DataSourceWrapper;
import com.sequoiadb.schedule.metasource.template.SequoiadbTemplate;
import com.sequoiadb.schedule.model.ServerNodeEntity;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Repository;

import javax.annotation.PostConstruct;
import java.util.ArrayList;
import java.util.List;

@Repository
public class SdbScheduleNodeDao implements ScheduleNodeDao {
    private static final Logger logger = LoggerFactory.getLogger(SdbScheduleNodeDao.class);
    private SequoiadbTemplate template;
    @Autowired
    private SequoiadbConfig sequoiadbConfig;
    private static final String CL_SERVER_NODE = "SERVER_NODE";
    private static final String INDEX_NAME = "idx_ip_port";
    private Timer retryTimer;
    private final static int RETRY_INTERVAL = 1000 * 3;
    private DataSourceWrapper dataSourceWrapper;

    @Autowired
    public SdbScheduleNodeDao(DataSourceWrapper dataSourceWrapper) {
        this.dataSourceWrapper = dataSourceWrapper;
        this.template = new SequoiadbTemplate(dataSourceWrapper);
    }

    @PostConstruct
    public void init() {
        try {
            ensureTableAndIndex();
        }
        catch (Exception e) {
            logger.error("failed to create table or index", e);
            asyncRetryEnsureTableAndIndex();
        }
    }

    private void asyncRetryEnsureTableAndIndex() {
        if (retryTimer == null) {
            retryTimer = TimerFactory.createTimer("retryEnsureTableAndIndexTimer");
        }
        retryTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                try {
                    logger.info("retry to create table and index");
                    ensureTableAndIndex();
                    cancel();
                }
                catch (Exception e) {
                    logger.error("failed to create table or index", e);
                }
            }
        }, RETRY_INTERVAL, RETRY_INTERVAL);
    }

    private void ensureTableAndIndex() throws ScheduleServerMetaSourceException {
        ensureTable();
        ensureIndex();
    }

    private void ensureTable() throws ScheduleServerMetaSourceException {
        try {
            template.collectionSpace()
                    .createCollection(CL_SERVER_NODE);
        }
        catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_DMS_EXIST.getErrorCode()) {
                throw new ScheduleServerMetaSourceException("failed to create collection:"
                        + template.getSystemCSName() + "." + CL_SERVER_NODE, e);
            }
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException("failed to create collection:"
                    + template.getSystemCSName() + "." + CL_SERVER_NODE, e);
        }
    }

    private void ensureIndex() throws ScheduleServerMetaSourceException {
        BasicBSONObject indexBson = new BasicBSONObject();
        indexBson.put(FieldName.ServerNode.FIELD_IPADDR, 1);
        indexBson.put(FieldName.ServerNode.FIELD_PORT, 1);
        try {
            template.collection(CL_SERVER_NODE)
                    .ensureIndex(INDEX_NAME, indexBson, true);
        }
        catch (Exception e) {
            throw new ScheduleServerMetaSourceException(
                    "failed to create index:csName=" + template.getSystemCSName() + ",clName="
                            + CL_SERVER_NODE + ",index=" + INDEX_NAME,
                    e);
        }
    }

    @Override
    public void upsert(ServerNodeEntity info) throws Exception {
        String ipAddress = info.getIpAddress();
        int port = info.getPort();
        BSONObject match = new BasicBSONObject();
        match.put(FieldName.ServerNode.FIELD_IPADDR, ipAddress);
        match.put(FieldName.ServerNode.FIELD_PORT, port);
        template.collection(CL_SERVER_NODE).upsert(match,
                new BasicBSONObject("$set", info.toBSONObject()));

    }

    @Override
    public void renew(ServerNodeEntity info) throws Exception {
        String ipAddress = info.getIpAddress();
        int port = info.getPort();
        BSONObject match = new BasicBSONObject();
        match.put(FieldName.ServerNode.FIELD_IPADDR, ipAddress);
        match.put(FieldName.ServerNode.FIELD_PORT, port);
        BSONObject modifier = new BasicBSONObject();
        modifier.put("$set", new BasicBSONObject(FieldName.ServerNode.FIELD_LAST_HEART_TIME,
                System.currentTimeMillis()));
        modifier.put("$inc", new BasicBSONObject(FieldName.ServerNode.FIELD_LEASE_NUM, 1));
        template.collection(CL_SERVER_NODE).update(match,
                modifier);
    }

    @Override
    public void down(ServerNodeEntity info) throws Exception {
        String ipAddress = info.getIpAddress();
        int port = info.getPort();
        BSONObject match = new BasicBSONObject();
        match.put(FieldName.ServerNode.FIELD_IPADDR, ipAddress);
        match.put(FieldName.ServerNode.FIELD_PORT, port);
        BSONObject modifier = new BasicBSONObject();
        modifier.put("$set", new BasicBSONObject(FieldName.ServerNode.FIELD_STATUS, "DOWN"));
        template.collection(CL_SERVER_NODE).update(match,
                modifier);
    }

    @Override
    public void useTempConnection() {
        this.template = new TempSequoiadbTemplate(dataSourceWrapper, sequoiadbConfig);
    }

    @Override
    public List<ServerNodeEntity> list() throws Exception {
        List<ServerNodeEntity> serverNodeEntityList = new ArrayList<>();
        MetaCursor metaCursor = null;
        try {
            metaCursor = template.collection(CL_SERVER_NODE)
                    .find(new BasicBSONObject());
            while (metaCursor.hasNext()) {
                BSONObject next = metaCursor.getNext();
                serverNodeEntityList.add(ServerNodeEntity.fromBSONObject(next));
            }
            return serverNodeEntityList;
        }
        finally {
            if (metaCursor != null) {
                metaCursor.close();
            }
        }
    }

    @Override
    public long countNode(BSONObject condition) throws Exception {
        return template.collection(CL_SERVER_NODE)
                .count(condition);
    }

    @Override
    public List<ServerNodeEntity> listNode(BSONObject condition, BSONObject orderBy, long skip,
            long limit) throws Exception {
        List<ServerNodeEntity> serverNodeEntityList = new ArrayList<>();
        MetaCursor metaCursor = null;
        try {
            metaCursor = template.collection(CL_SERVER_NODE)
                    .find(condition, null, orderBy, skip, limit);
            while (metaCursor.hasNext()) {
                BSONObject next = metaCursor.getNext();
                serverNodeEntityList.add(ServerNodeEntity.fromBSONObject(next));
            }
            return serverNodeEntityList;
        }
        finally {
            if (metaCursor != null) {
                metaCursor.close();
            }
        }
    }
}
