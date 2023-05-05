/**
 * Copyright (C) 2023 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.sequoiadb.tool;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.options.UpdateOption;
import com.sequoiadb.base.result.UpdateResult;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class Worker implements Runnable {

    private final String FIELD_SITE_LIST = "site_list";
    private final String FIELD_CREATE_TIME = "create_time";
    private final String FIELD_LAST_ACCESS_TIME = "last_access_time";
    private final String FIELD_SITE_ID = "site_id";
    private final String IDX_ID = "$id";

    private static final Logger logger = LoggerFactory.getLogger(Worker.class);
    private Param param;
    private SequoiadbDatasource datasource;
    private JobMgr jobMgr;
    private FileMgr fileMgr;

    public Worker(Param param, SequoiadbDatasource datasource, JobMgr jobMgr, FileMgr fileMgr) {
        this.param = param;
        this.datasource = datasource;
        this.jobMgr = jobMgr;
        this.fileMgr = fileMgr;
    }

    @Override
    public void run() {
        while (Controller.isRunning()) {
            // 1. 从 JobMgr 拉取一个任务
            JobInfo jobInfo = jobMgr.getJobRecord();
            if (jobInfo == null) {
                logger.info("No jobs to do, stop current worker");
                return;
            }
            logger.info("Start job[" + jobInfo.getBaseInfo() + "] in worker: " + Thread.currentThread().getName());

            // 2. 进行作业
            // 分别获取 coord 连接 及 各数据节点连接
            Sequoiadb sdb = getSdb();
            List<Sequoiadb> connList = getConnections(jobInfo.getNodeList());
            try {
                doit(sdb, connList, jobInfo);
            } catch (Exception e) {
                // 让其它线程正常退出
                Controller.setHasErr(true);
                Controller.stop();
                throw e;
            } finally {
                releaseConnections(connList);
                releaseSdb(sdb);
            }

            logger.info("Stop job[" + jobInfo.getBaseInfo() + "] in worker: " + Thread.currentThread().getName());
        }
    }

    //    操作步骤：
//     1. 获取当前数据组所有节点的连接
//     2. 使用 LastOid + hint($oid) + limit 循环从主节点获取数据
//     3. 循环处理这批数据
//     3.1 使用 Oid 把所有备节点的记录查回来。如果备节点查询不到，或者主备节点记录没有 site_list 字段，将 Oid 写入 miss_record.list
//     3.2 把所有节点返回记录的 site_list 都提取出来
//     3.3 比较及处理这些 site_list
//         1) site_list 字段所有元素一致，且没有非法元素，跳过这条元数据记录
//         2) 否则，剔除这些记录的非法元素，并将剩余的元素合并、去重，得到一个新的 site_list
//         2.1）如果新的 site_list 为空，将 Oid 输出到 empty_site.list 文件
//         2.2）如果新的 site_list 不为空，
//         2.2.1) 将新的 site_list 中的 last_access_time 更新为当前时间
//         2.2.2) 使用 $set + 新的 site_list 来构建 updater
//         2.2.3) 使用 _id + $or + 原来所有副本的 site_list 来构建 matcher
//         2.2.4) 连接 coord 进行更新
//         2.2.4.1) 更新成功，将 Oid 写入 update_succ.list 文件
//         2.2.4.1) 更新失败，将 Oid 写入 update_fail.list 文件
//     3.4 用最后一次使用的 Oid 覆盖 jobInfo 中的 Oid
//     3.5 将 jobInfo 更新回 JobMgr
    private void doit(Sequoiadb sdb, List<Sequoiadb> connList, JobInfo jobInfo) {
        String fullName = jobInfo.getCollectionName();
        ObjectId lastOid = null;
        long startTM = System.currentTimeMillis();
        long lastTM = startTM;
        int totalFixedCount = 0;

        // 注册集合，初始化输出文件
        fileMgr.registerCL(fullName);

        // 分别从 coord 及 各数据节点获取集合对象
        DBCollection coll = getCollection(sdb, fullName);
        List<DBCollection> collList = getCollections(connList, fullName);

        while (true) {
            // 使用 LastOid + hint($oid) + limit 从主节点获取一批数据
            DBCursor cursor = queryData(collList.get(0), jobInfo.getLastOid(), param.getBatchSize());
            try {
                if (!cursor.hasNext()) {
                    // 数据集为空，说明所有数据已经处理完毕。将当前任务的状态标记为 "DONE"，并最后一次更新 jobInfo
                    jobInfo.setLastOid(lastOid);
                    jobInfo.setStatus(JobInfo.STATUS_DONE);
                    jobMgr.updateJobRecord(jobInfo);
                    logger.info("Finish job: " + jobInfo.getBaseInfo());
                    return;
                }
                // 循环处理这批数据
                while (cursor.hasNext()) {
                    totalFixedCount++;
                    if (totalFixedCount % 100 == 0 && !Controller.isRunning()) {
                        // 每循环 100 次，就检查一下是否需要终止退出。防止 kill -15 等操作后，线程长时间没有退出
                        jobInfo.setLastOid(lastOid);
                        jobMgr.updateJobRecord(jobInfo);
                        logger.info("Interrupted job: " + jobInfo.getBaseInfo());
                        return;
                    }
                    BSONObject masterRecord = cursor.getNext();
                    ObjectId oid = (ObjectId) masterRecord.get("_id");
                    List<BasicBSONList> siteLists = new ArrayList<>();
                    try {
                        /// 使用 Oid 把所有备节点记录查回来。如果备节点查询不到结果，或者主备节点记录没有 site_list 字段，
                        /// 便将 Oid 写入 miss_record.list
                        List<BasicBSONList> slaveSiteLists = getSlaveSiteList(collList, oid);
                        BasicBSONList siteList = (BasicBSONList) masterRecord.get("site_list");
                        /// 把所有节点返回的 site_list 都放在一起，供后续用于 比较一致性、合并去重、构造查询条件 等
                        siteLists.add(siteList);
                        siteLists.addAll(slaveSiteLists);
                    } catch (Exception e) {
                        logger.warn("Failed to extract site_list from record with oid: " + oid.toString()
                                + "error: " + e.getMessage());
                        // 检测连接是否还有效。如果有效，就继续循环。否则，进程异常退出
                        if (isConnectionValid(sdb, connList)) {
                            fileMgr.writeMissRecord(fullName, oid);
                            lastOid = oid;
                            continue;
                        } else {
                            jobInfo.setLastOid(lastOid);
                            jobMgr.updateJobRecord(jobInfo);
                            logger.error("Connection is broken for job: " + jobInfo.getBaseInfo());
                            throw e;
                        }
                    }
                    /// 比较及处理这些 site_list
                    boolean isOk = isConsistentAndLegal(siteLists);
                    if (isOk) {
                        //// site_list 字段所有元素一致，且没有非法元素，跳过这条元数据记录
                        lastOid = oid;
                        continue;
                    }
                    //// 否则，剔除记录中非法元素，将剩余的元素合并、去重，得到一个新的 site_list （last_access_time 已更新为当前时间）
                    BasicBSONList newSiteList = rebuildSiteList(siteLists);
                    if (newSiteList.isEmpty()) {
                        //// 如果新的 site_list 为空，将 Oid 输出到 empty_site.list 文件
                        fileMgr.writeEmptySite(fullName, oid);
                    } else {
                        boolean isSucc = false;
                        try {
                            //// 如果这个 site_list 不为空
                            //// 使用 $set + 新的 site_list 来构建 updater
                            //// 使用 _id + $or + 原来所有副本的 site_list 来构建 matcher
                            //// 连接 coord 进行更新
                            isSucc = updateSiteList(coll, oid, newSiteList, siteLists);
                        } catch (Exception e) {
                            logger.warn("Failed to update in collection: " + fullName + ", e: " + e.getMessage());
                            isSucc = false;
                        }
                        if (isSucc) {
                            //// 更新成功，将结果 Oid 写入 update_succ.list 文件
                            fileMgr.writeUpdateSucc(fullName, oid);
                        } else {
                            if (isConnectionValid(sdb, connList)) {
                                //// 更新失败且连接还有效，将结果 Oid 写入 update_fail.list 文件
                                fileMgr.writeUpdateFail(fullName, oid);
                            } else {
                                //// 否则，让进程异常退出
                                jobInfo.setLastOid(lastOid);
                                jobMgr.updateJobRecord(jobInfo);
                                String errMsg = "Connection is broken for job: " + jobInfo.getBaseInfo();
                                logger.error(errMsg);
                                throw new BaseException(SDBError.SDB_SYS, errMsg);
                            }
                        }
                    }
                    lastOid = oid;
                }
            } finally {
                cursor.close();
            }
            // 尝试打印统计信息。每隔 5 min 打印一次
            lastTM = printStatistics(jobInfo, startTM, lastTM, totalFixedCount);
            // 用最后一个更新记录的 Oid 覆盖 jobInfo 中的 Oid，并将 jobInfo 更新回 JobMgr
            jobInfo.setLastOid(lastOid);
            jobMgr.updateJobRecord(jobInfo);
            fileMgr.flush(fullName);
        }
    }

    private boolean isConnectionValid(Sequoiadb sdb, List<Sequoiadb> connList) {
        if (!sdb.isValid()) {
            return false;
        }
        for (Sequoiadb db : connList) {
            if (!db.isValid()) {
                return false;
            }
        }
        return true;
    }

    private Sequoiadb getSdb() {
        Sequoiadb sdb;
        try {
            sdb = datasource.getConnection();
        } catch (InterruptedException e) {
            String errMsg = "Failed to get connection";
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
        return sdb;
    }

    private void releaseSdb(Sequoiadb sdb) {
        datasource.releaseConnection(sdb);
    }

    private List<Sequoiadb> getConnections(List<String> nodeNameList) {
        List<Sequoiadb> sdbList = new ArrayList<>();
        for (String nodeName : nodeNameList) {
            Sequoiadb sdb = new Sequoiadb(nodeName, param.getUserName(), param.getPassword());
            sdbList.add(sdb);
        }
        return sdbList;
    }

    private void releaseConnections(List<Sequoiadb> sdbList) {
        for (Sequoiadb sdb : sdbList) {
            sdb.close();
        }
    }

    private DBCollection getCollection(Sequoiadb sdb, String fullName) {
        return sdb.getCollectionSpace(fullName.split("\\.")[0]).getCollection(fullName.split("\\.")[1]);
    }

    private List<DBCollection> getCollections(List<Sequoiadb> sdbList, String fullName) {
        List<DBCollection> collList = new ArrayList<>();
        String csName = fullName.split("\\.")[0];
        String clName = fullName.split("\\.")[1];
        for (Sequoiadb sdb : sdbList) {
            DBCollection coll = sdb.getCollectionSpace(csName).getCollection(clName);
            collList.add(coll);
        }
        return collList;
    }

    private DBCursor queryData(DBCollection cl, ObjectId oid, int limit) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        ObjectId newOid = oid != null ? oid : new ObjectId(0, 0, 0);

        matcher.put("_id", new BasicBSONObject("$gt", newOid));
        hint.put("", IDX_ID);

        logger.debug("cl: " + cl.getFullName() + " query, matcher: "
                + matcher.toString() + ", hint: " + hint.toString() + ", limit: " + limit);
        DBCursor cursor = cl.query(matcher, null, null, hint, 0, limit, 0);
        return cursor;
    }

    private BSONObject queryData(DBCollection cl, ObjectId oid) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();
        matcher.put("_id", oid);
        hint.put("", IDX_ID);
        return cl.queryOne(matcher, null, null, hint, 0);
    }

    private List<BasicBSONList> getSlaveSiteList(List<DBCollection> collList, ObjectId oid) {
        List<BasicBSONList> siteLists = new ArrayList<>();

        // start from index 1, skip the master
        for (int i = 1; i < collList.size(); i++) {
            DBCollection coll = collList.get(i);
            // query record
            BSONObject record = queryData(coll, oid);
            if (record == null) {
                String errMsg = "Failed to query record from cl: " + coll.getFullName() +
                        ", with oid: " + oid.toString();
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            // get fields
            BasicBSONList siteList = (BasicBSONList) record.get("site_list");
            if (siteList == null) {
                String errMsg = "Failed to get site_list from record in cl: " + coll.getFullName() +
                        ", with oid: " + oid.toString();
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            } else {
                siteLists.add(siteList);
            }
        }
        return siteLists;
    }

    /**
     * 1. 所有元素都一样
     * 2. 不包含非法元素
     *
     * @param siteLists
     * @return
     */
    private boolean isConsistentAndLegal(List<BasicBSONList> siteLists) {
        if (siteLists.isEmpty()) {
            return false;
        }

        BasicBSONList baseList = siteLists.get(0);
        if (!isLegal(baseList)) {
            return false;
        }

        for (int i = 1; i < siteLists.size(); i++) {
            BasicBSONList list = siteLists.get(i);
            // 检查一致性
            // ["aa", "bb"] 和 ["bb", "aa"] 会被判断为不相等
            if (!list.equals(baseList)) {
                return false;
            }
            // 检查正确性
            if (!isLegal(list)) {
                return false;
            }
        }

        return true;
    }

    /**
     * 包含如下三个字段：
     * create_time
     * last_access_time
     * site_id
     *
     * @param bsonList
     * @return
     */
    private boolean isLegal(BasicBSONList bsonList) {
        Object[] objs = (Object[]) bsonList.toArray();
        for (int i = 0; i < objs.length; i++) {
            BSONObject obj = (BSONObject) objs[i];
            if (!isLegal(obj)) {
                return false;
            }
        }
        return true;
    }

    private boolean isLegal(BSONObject obj) {
        if (!obj.containsField(FIELD_CREATE_TIME) ||
                !obj.containsField(FIELD_LAST_ACCESS_TIME) ||
                !obj.containsField(FIELD_SITE_ID)) {
            return false;
        }
        return true;
    }

    private BasicBSONList rebuildSiteList(List<BasicBSONList> siteLists) {
        int index = 0;
        BasicBSONList newBsonList = new BasicBSONList();
        Set<Integer> siteIdSet = new HashSet<>();

        for (BasicBSONList bsonList : siteLists) {
            Object[] objs = (Object[]) bsonList.toArray();
            for (int i = 0; i < objs.length; i++) {
                BSONObject obj = (BSONObject) objs[i];
                // 如果是非法的元素，直接丢弃
                if (!isLegal(obj)) {
                    continue;
                }
                // 判断 site_id 是否重复，如果重复，直接丢弃
                int siteId = (int) obj.get(FIELD_SITE_ID);
                if (siteIdSet.contains(siteId)) {
                    continue;
                } else {
                    siteIdSet.add(siteId);
                }
                // 更新 last_access_time
                BSONObject newObj = new BasicBSONObject();
                newObj.putAll(obj);
                newObj.put(FIELD_LAST_ACCESS_TIME, Helper.timestamp);
                // 把元素放到新的 bson 数组里面
                newBsonList.put(index++, newObj);
            }
        }

        return newBsonList;
    }

    private boolean updateSiteList(DBCollection coll, ObjectId oid,
                                   BasicBSONList newSiteList, List<BasicBSONList> siteLists) {
        BSONObject matcher = new BasicBSONObject();
        BSONObject updater = new BasicBSONObject();
        UpdateOption option = new UpdateOption().setHint(new BasicBSONObject("", IDX_ID));

        // matcher is: { $and:[ {"_id" : oid}, {$or: [ {"site_list": bsonList1}, {"site_list": bsonList2} ] } ] }
        // updater is: { $set: {"site_list": newBsonList} }

        // build $or:[]
        int orIdx = 0;
        BasicBSONList orList = new BasicBSONList();
        for (BasicBSONList bsonList : siteLists) {
            orList.put(orIdx++, new BasicBSONObject(FIELD_SITE_LIST, bsonList));
        }
        // build $and:[]
        BasicBSONList andList = new BasicBSONList();
        andList.put(0, new BasicBSONObject("_id", oid));
        andList.put(1, new BasicBSONObject("$or", orList));
        // build matcher
        matcher.put("$and", andList);
        // build updater
        updater.put("$set", new BasicBSONObject(FIELD_SITE_LIST, newSiteList));

        logger.debug("Update record, matcher: " + matcher.toString() + ", updater: "
                + updater.toString() + ", option: " + option.toString() + " in cl: " + coll.getFullName());
        try {
            UpdateResult updateResult = coll.updateRecords(matcher, updater, option);
            if (updateResult.getModifiedNum() != 1) {
                logger.error("Failed to update record, matcher: " + matcher.toString() + ", updater: "
                        + updater.toString() + ", option: " + option.toString() + " in cl: " + coll.getFullName());
                return false;
            }
        } catch (Exception e) {
            logger.error("Failed to update record, matcher: " + matcher.toString() + ", updater: "
                    + updater.toString() + ", option: " + option.toString() + " in cl: " + coll.getFullName()
                    + ", error: " + e.getMessage());
            return false;
        }

        return true;
    }

    private long printStatistics(JobInfo jobInfo, long startTM, long lastTM, int fixedCount) {
        long currentTM = System.currentTimeMillis();
        if ((currentTM - lastTM) >= 300000) { // 大于 5min 打印一次统计信息
            long deltaTM = currentTM - startTM;
            long sec = deltaTM / 1000;
            long millSec = deltaTM % 1000;
            logger.info("Job[" + jobInfo.getBaseInfo() + "] in worker [" + Thread.currentThread().getName() +
                    "] " + "has run: " + sec + "." + millSec + "(secs), and has fixed: " + fixedCount + " records");
            return currentTM;
        } else {
            return lastTM;
        }
    }

}
