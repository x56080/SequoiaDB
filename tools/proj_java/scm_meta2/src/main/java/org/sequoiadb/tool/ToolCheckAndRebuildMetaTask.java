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
import java.util.concurrent.ConcurrentLinkedQueue;

class MetaInfo {
    int site_id;
    long last_access_time;
    long create_time;

    public MetaInfo(int site_id, long last_access_time, long create_time) {
        this.site_id = site_id;
        this.last_access_time = last_access_time;
        this.create_time = create_time;
    }

    BSONObject toBSON() {
        BSONObject bson = new BasicBSONObject();

        bson.put("site_id", site_id);
        bson.put("last_access_time", last_access_time);
        bson.put("create_time", create_time);

        return bson;
    }

    @Override
    public String toString() {
        return "MetaInfo{" +
                "site_id=" + site_id +
                ", last_access_time=" + last_access_time +
                ", create_time=" + create_time +
                '}';
    }
}

class LobInfo {
    int siteId;
    long createTime;
    boolean available;

    public LobInfo(int siteId, long createTime, boolean available) {
        this.siteId = siteId;
        this.createTime = createTime;
        this.available = available;
    }

    MetaInfo toMetaInfo() {
        return available ? new MetaInfo(siteId, createTime, createTime) : null;
    }

    @Override
    public String toString() {
        return "LobInfo{" +
                "siteId=" + siteId +
                ", createTime=" + createTime +
                ", available=" + available +
                '}';
    }
}

class TaskWorker implements Runnable {
    private static final Logger logger = LoggerFactory.getLogger(TaskWorker.class);
    private ToolCheckAndRebuildMetaTask taskMgr;
    private Param param;
    private Common common;
    private ResultFile noLobFile;
    private ResultFile invalidLobFile;

    public TaskWorker(ToolCheckAndRebuildMetaTask taskMgr, Param param, Common common,
                      ResultFile noLobFile, ResultFile invalidLobFile) {
        this.taskMgr = taskMgr;
        this.param = param;
        this.common = common;
        this.noLobFile = noLobFile;
        this.invalidLobFile = invalidLobFile;
    }

    @Override
    public void run() {
        long startTM = System.currentTimeMillis();
        long lastTM = startTM;
        int totalFixedCount = 0;

        Sequoiadb sdb = common.getSdb();
        try {
            DBCollection metaCL = Common.getCL(sdb, taskMgr.getMetaCLName());
            DBCollection lobInfoCL = Common.getCL(sdb, taskMgr.getLobInfoCLName());
            DBCollection updateCL = Common.getCL(sdb, taskMgr.getUpdateCLName());

            // 循环拉取任务进行作业
            while (true) {
                if (!Controller.isRunning()) {
                    logger.info("Interrupted task");
                    return;
                }
                /// 获取任务批次
                ObjectId objectId = taskMgr.getObjectId();
                if (objectId == null) { // 说明已经没有任务需要处理，线程退出
                    logger.info("Finish task");
                    return;
                }
                /// 运行该任务批次
                try {
                    totalFixedCount += runBatch(metaCL, lobInfoCL, updateCL, objectId);
                } catch (Exception e) {
                    // 让其它线程正常退出
                    Controller.setHasErr(true);
                    Controller.stop();
                    throw e;
                }
                // 尝试打印统计信息。每隔 5 min 打印一次
                lastTM = Common.printStatistics("Repair", startTM, lastTM, totalFixedCount);
            }
        } finally {
            common.releaseSdb(sdb);
            Common.printStatistics("Repair", startTM, lastTM, totalFixedCount, 0);
        }
    }

    private int runBatch(DBCollection metaCL, DBCollection lobInfoCL, DBCollection updateCL, ObjectId objectId) {
        int recordCount = 0;
        int loopCount = 0;
        // 处理每批次的 meta 记录
        DBCursor cursor = queryMetaInfo(metaCL, objectId, taskMgr.getBatchSize());
        try {
            while (cursor.hasNext()) {
                loopCount++;
                if (loopCount % 200 == 0 && !Controller.isRunning()) {
                    /// 每循环 200 次，就检查一下是否需要终止退出。防止 kill -15 等操作后，线程长时间没有退出
                    return recordCount;
                }
                BSONObject metaRecord = cursor.getNext();
                recordCount++;
                /// 使用 meta 表记录的 data_id + lobinfo 表的 hint 构造查询条件到 lobinfo 表进行查询
                List<BSONObject> lobInfoList = queryLobInfo(lobInfoCL, metaRecord);

                /// 将返回的 lob info 与 meta 表的 site_list 信息进行比较，
                /// 把需要增加或者删除的信息更新回 site_list 数组
                BSONObject newMetaRecord = checkAndRebuildMeta(metaRecord, lobInfoList);

                /// 把需要更新的 meta 记录写入 更新表
                if (newMetaRecord != null) {
                    writeToUpdateTable(updateCL, newMetaRecord);
                    logger.debug("write meta record is: " + newMetaRecord.toString());
                }
            }
        } finally {
            cursor.close();
            logger.debug("Current batch handle " + recordCount + " meta records");
        }
        return recordCount;
    }

    private DBCursor queryMetaInfo(DBCollection cl, ObjectId objectId, int batchSize) {
        BSONObject matcher = new BasicBSONObject("_id", new BasicBSONObject("$gte", objectId));
        BSONObject sorter = new BasicBSONObject("_id", 1);
        BSONObject hint = new BasicBSONObject("", Common.IDX_DEFAULT_ID);
        return cl.query(matcher, null, sorter, hint, 0, batchSize, 0);
    }

    private List<BSONObject> queryLobInfo(DBCollection cl, BSONObject metaRecord) {
        ArrayList<BSONObject> recordList = new ArrayList<>();
        BSONObject matcher = new BasicBSONObject();
        BSONObject hint = new BasicBSONObject();

        // 构造查询条件 及 hint
        if (!metaRecord.containsField("data_id")) {
            String errMsg = "Meta record[" + metaRecord.toString() + "] does not contain data_id field";
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_SYS, errMsg);
        }
        String dataId = (String) metaRecord.get("data_id");
        ObjectId lobId = new ObjectId(dataId);
        matcher.put("Oid", lobId);
        hint.put("", Common.IDX_USER_DEF_OID); // 使用我们自己以 Oid 字段创建的 非唯一索引

        // 将所有命中的 lob info 返回。因为只有 21 个站点，理论上最多只有 21 条记录，所以可以通过一个 list 返回所有结果
        DBCursor cursor = cl.query(matcher, null, null, hint);
        try {
            while (cursor.hasNext()) {
                recordList.add(cursor.getNext());
            }
        } finally {
            cursor.close();
        }

        return recordList;
    }

    private BSONObject checkAndRebuildMeta(BSONObject metaRecord, List<BSONObject> lobInfoList) {
        BSONObject newMetaRecord = null;

        if (!metaRecord.containsField("orig_site_list")) {
            String errMsg = "No orig_site_list field in meta record: " + metaRecord.toString();
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_SYS, errMsg);
        }

        List<MetaInfo> metaInfos = getMetaInfos(metaRecord);
        List<LobInfo> lobInfos = getLobInfos(lobInfoList);

        // 比较 SCM元数据信息 与 SCM Lob 信息是否匹配，存在以下情况说明不匹配
        // 1) metaInfos 为空 或者 lobInfos 为空
        // 2) metaInfos 有元素，但是 lobInfos 没有
        // 3) lobInfos 有元素，但是 metaInfos 没有
        boolean isOk = checkMetaInfo(metaInfos, lobInfos);

        if (!isOk) {
            if (lobInfos.isEmpty()) {
                // 如果 lobInfos 为空，说明通过 meta 表的 data_id 不能在任何站点找到该 lob,
                // 需要把该 meta 记录的 oid 写入 no_lob 文件
                noLobFile.write((ObjectId) metaRecord.get("_id"));
            } else {
                // 合并 metaInfos 与 lobInfos 的元素，规则如下：
                // 1) 先把存在于 metaInfos 并且存在于 lobInfos 的合法 lob 信息，写入 BasicBSONList
                // 2) 再把存在于 lobInfos，但是不存在于 metaInfos 的合法 lob 信息写入 BasicBSONList
                // 3) 将最后没有任何合法 lob 信息的 元数据 的 oid 写入 invalid_lob 文件
                // “合法 lob 信息” 是指 cl.listLobs() 拿到的信息中，“available” 字段为 true
                newMetaRecord = rebuildMeta(metaRecord, metaInfos, lobInfos);
            }
        }

        return newMetaRecord;
    }

    private List<MetaInfo> getMetaInfos(BSONObject metaRecord) {
        List<MetaInfo> metaInfos = new ArrayList<>();
        Set<Integer> siteIdSet = new HashSet<>();
        BasicBSONList bsonList = (BasicBSONList) metaRecord.get("orig_site_list");

        Object[] objs = (Object[]) bsonList.toArray();
        for (int i = 0; i < objs.length; i++) {
            BSONObject obj = (BSONObject) objs[i];
            if (obj == null) {
                continue;
            }
            // check fields
            if (!obj.containsField("site_id")) {
                String errMsg = "No site_id field in orig_site_list array element in meta record: " + metaRecord.toString();
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            if (!obj.containsField("create_time")) {
                String errMsg = "No create_time field in orig_site_list array element in meta record: " + metaRecord.toString();
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            if (!obj.containsField("last_access_time")) {
                String errMsg = "No last_access_time field in orig_site_list array element in meta record: " + metaRecord.toString();
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            // get fields
            int siteId = (Integer) obj.get("site_id");
            long createTime = (Long) obj.get("create_time");
            long lastAccessTime = (Long) obj.get("last_access_time");
            if (!siteIdSet.contains(siteId)) {
                MetaInfo metaInfo = new MetaInfo(siteId, lastAccessTime, createTime);
                metaInfos.add(metaInfo);
                siteIdSet.add(siteId);
            }
        }

        return metaInfos;
    }

    private List<LobInfo> getLobInfos(List<BSONObject> lobInfoList) {
        List<LobInfo> lobInfos = new ArrayList<>();
        Set<Integer> siteIdSet = new HashSet<>();

        for (BSONObject obj : lobInfoList) {
            int siteId = (Integer) obj.get("SiteId");
            long createTime = (Long) obj.get("CreateTime");
            boolean available = (Boolean) obj.get("Available");
            if (!siteIdSet.contains(siteId)) {
                LobInfo lobInfo = new LobInfo(siteId, createTime, available);
                lobInfos.add(lobInfo);
                siteIdSet.add(siteId);
            }
        }

        return lobInfos;
    }

    private boolean checkMetaInfo(List<MetaInfo> metaInfos, List<LobInfo> lobInfos) {
        if (metaInfos.isEmpty() || lobInfos.isEmpty()) {
            // 如果任意一方为空，说明需特别处理，校验不通过
            return false;
        }

        // 检测 meta 中所有的 site_id 是否在 lob info 中都存在。如果有不存在的，说明两边不匹配，校验不通过
        for (MetaInfo metaInfo : metaInfos) {
            boolean exist = existInLobInfos(metaInfo, lobInfos);
            if (!exist) {
                return false;
            }
        }

        // 检测 lob info 中所有的 SiteId 是否在 meta 中都存在。如果有不存在的，说明两边不匹配，校验不通过
        for (LobInfo lobInfo : lobInfos) {
            boolean exist = existInMetaInfos(lobInfo, metaInfos);
            if (!exist) {
                return false;
            }
        }

        return true;
    }

    private boolean existInLobInfos(MetaInfo metaInfo, List<LobInfo> lobInfos) {
        int siteId = metaInfo.site_id;
        for (LobInfo lobInfo : lobInfos) {
            if (siteId == lobInfo.siteId) {
                return true;
            }
        }
        return false;
    }

    private boolean existInMetaInfos(LobInfo lobInfo, List<MetaInfo> metaInfos) {
        int siteId = lobInfo.siteId;
        for (MetaInfo metaInfo : metaInfos) {
            if (siteId == metaInfo.site_id) {
                return true;
            }
        }
        return false;
    }

    private BSONObject rebuildMeta(BSONObject metaRecord, List<MetaInfo> metaInfos, List<LobInfo> lobInfos) {
        BSONObject newMetaRecord = new BasicBSONObject();
        BasicBSONList bsonList = new BasicBSONList();

        // 如果 metaInfos 为空，说明 meta 记录是从自 empty_site 查询出来的，
        // 这时，只需要把 lobInfos 的信息填写回 元数据的 site_list 数组即可
        if (metaInfos.isEmpty()) {
            buildSiteList(bsonList, 0, lobInfos, null);
        } else {
            int arrayIdx = 0;
            /// 把 metaInfos 中，存在于 lobInfos 的 lob info 写入 bsonList
            for (MetaInfo metaInfo : metaInfos) {
                //// 获取当前 metaInfo.site_id 在 lobInfos 对应的 lob info,
                //// 如果找不到，getLobInfoBySiteId 会返回 null
                //// 找到之后，判断 lob 状态是否可用，在可用的情况下，才能把 metaInfo 写入 bsonList
                LobInfo lobInfo = getLobInfoBySiteId(metaInfo.site_id, lobInfos);
                if (lobInfo != null && lobInfo.available == true) {
                    bsonList.put(arrayIdx++, metaInfo.toBSON());
                }
            }
            /// 把 lobInfos 中，不存在于 metaInfos 的 lob info 写入 bsonList
            buildSiteList(bsonList, arrayIdx, lobInfos, metaInfos);
        }

        // 检测 bsonList 是否为空，如果为空，说明没有有效的 lob 信息可以回写到 元数据 的 site_list 中，
        // 这时，需要把元数据的 oid 写入到 invalid_lob 文件
        if (bsonList.isEmpty()) {
            invalidLobFile.write((ObjectId) metaRecord.get("_id"));
        } else {
            newMetaRecord.putAll(metaRecord);
            newMetaRecord.put("new_site_list", bsonList);
        }

        logger.debug("New meta record info is: " + newMetaRecord.toString());

        return newMetaRecord;
    }

    private void buildSiteList(BasicBSONList outputBSONList, int startIndex,
                               List<LobInfo> lobInfos, List<MetaInfo> excludedMetaInfos) {
        int arrayStartIndex = startIndex;
        for (LobInfo lobInfo : lobInfos) {
            if (excludedMetaInfos != null) {
                boolean exists = existInMetaInfos(lobInfo, excludedMetaInfos);
                if (!exists) {
                    MetaInfo metaInfo = lobInfo.toMetaInfo(); // 当 lob 状态不可用时，toMetaInfo() 将返回 null
                    if (metaInfo != null) {
                        outputBSONList.put(arrayStartIndex++, metaInfo.toBSON());
                    } else {
                        logger.warn("Lob status is unavailable: " + lobInfo.toString());
                    }
                }
            } else {
                MetaInfo metaInfo = lobInfo.toMetaInfo(); // 当 lob 状态不可用时，toMetaInfo() 将返回 null
                if (metaInfo != null) {
                    outputBSONList.put(arrayStartIndex++, metaInfo);
                } else {
                    logger.warn("Lob status is unavailable: " + lobInfo.toString());
                }
            }
        }
    }

    private LobInfo getLobInfoBySiteId(int siteId, List<LobInfo> lobInfos) {
        for (LobInfo lobInfo : lobInfos) {
            if (siteId == lobInfo.siteId) {
                return lobInfo;
            }
        }
        return null;
    }

    private void writeToUpdateTable(DBCollection cl, BSONObject newMetaRecord) {
        cl.insertRecord(newMetaRecord);
    }

}

/**
 * 在 meta 表按 _id 排序之后，每隔 1w（默认值） 条，取一下 _id，存放在队列中。
 * 然后起多个线程不断从队列中获取 _id， 配合 limit 来并发处理任务
 */
public class ToolCheckAndRebuildMetaTask implements TaskBase {
    private static final Logger logger = LoggerFactory.getLogger(ToolCheckAndRebuildMetaTask.class);
    private static final String CSNAME_SUFFIX = "_test";
    private static final String META_TABLE_SUFFIX = "_meta";
    private static final String LOBINFO_TABLE_SUFFIX = "_lobinfo";
    private static final String UPDATE_TABLE_SUFFIX = "_update";
    private static final String NO_LOB_OUTPUT_FILE_SUFFIX = ".no_lob";
    private static final String INVALID_LOB_OUTPUT_FILE_SUFFIX = ".invalid_lob";

    private Param param;
    private Common common;
    private String metaCLName;
    private String lobInfoCLName;
    private String updateCLName;
    private String noLobOutputPath;
    private String invalidLobOutputPath;
    // 线程安全的队列，用于存放 oid，供工作线程并发访问
    private ConcurrentLinkedQueue<ObjectId> queue = new ConcurrentLinkedQueue<>();

    public ToolCheckAndRebuildMetaTask(Param param, Common common) {
        this.param = param;
        this.common = common;
        String[] names = Common.getNames(param.getCollectionName());
        // meta table
        this.metaCLName = names[0] + CSNAME_SUFFIX + "." + names[1] + META_TABLE_SUFFIX;
        // lobinfo table
        this.lobInfoCLName = names[0] + CSNAME_SUFFIX + "." + names[1] + LOBINFO_TABLE_SUFFIX;
        // update table
        this.updateCLName = names[0] + CSNAME_SUFFIX + "." + names[1] + UPDATE_TABLE_SUFFIX;
    }

    @Override
    public void init() {
        // 初始化输出文件
        this.noLobOutputPath = Common.createOutputFile(param.getOutputDir(),
                param.getCollectionName() + NO_LOB_OUTPUT_FILE_SUFFIX);
        this.invalidLobOutputPath = Common.createOutputFile(param.getOutputDir(),
                param.getCollectionName() + INVALID_LOB_OUTPUT_FILE_SUFFIX);

        Sequoiadb sdb = common.getSdb();
        try {
            // 检测 表 及 索引 是否存在
            DBCollection metaCL = null;
            DBCollection lobInfoCL = null;
            DBCollection updateCL = null;
            try {
                metaCL = Common.getCL(sdb, metaCLName);
                lobInfoCL = Common.getCL(sdb, lobInfoCLName);
                updateCL = Common.getCL(sdb, updateCLName);
            } catch (BaseException e) {
                String errMsg = "Failed to get collection in: " + sdb;
                logger.error(errMsg + ", e: " + e.getMessage());
                throw e;
            }
            try {
                lobInfoCL.getIndexInfo(Common.IDX_USER_DEF_OID);
            } catch (BaseException e) {
                if (e.getErrorType().equals(SDBError.SDB_IXM_NOTEXIST.getErrorType())) { // -47, index not exist
                    logger.error("No index[" + Common.IDX_USER_DEF_OID + "] in collection: " + lobInfoCLName);
                } else {
                    logger.error("Failed to get index[" + Common.IDX_USER_DEF_OID + "] in collection: " + lobInfoCLName);
                }
                throw e;
            }
            // 检测 meta 表 及 lobinfo 表 是否为空，预期不为空
            if (metaCL.getCount() == 0) {
                String errMsg = "Collection[" + metaCLName + "] is empty before running, please check";
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            if (lobInfoCL.getCount() == 0) {
                String errMsg = "Collection[" + lobInfoCLName + "] is empty before running, please check";
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
            // 检测 更新表 的记录数是否为 0，预期需要为 0
            if (updateCL.getCount() != 0) {
                String errMsg = "Collection[" + updateCLName + "] is not empty before running, please check";
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }

            // 构建 Lob 表 oid 分段信息
            buildOidQueue(metaCL);
        } finally {
            common.releaseSdb(sdb);
        }
    }

    private void buildOidQueue(DBCollection cl) {
        int batchSize = param.getBatchSize();
        ObjectId objectId = new ObjectId(0, 0, 0);
        BSONObject matcher = new BasicBSONObject("_id", new BasicBSONObject("$gte", objectId));
        BSONObject sorter = new BasicBSONObject("_id", 1); // 走 coord 节点，需要加排序
        BSONObject hint = new BasicBSONObject("", Common.IDX_DEFAULT_ID);

        // 走 coord 全量排序，把然后轮询一次，把分段的 oid 拿回来，保存到 队列 里面
        DBCursor cursor = cl.query(matcher, null, sorter, hint, 0, -1, 0);
        try {
            int counter = 0;
            while (cursor.hasNext()) {
                BSONObject record = cursor.getNext();
                if (counter % batchSize == 0) {
                    // 把 oid 保存
                    ObjectId currentOid = (ObjectId) record.get("_id");
                    queue.add(currentOid);
                }
                counter++;
            }
        } finally {
            cursor.close();
        }
    }

    @Override
    public void doit() {
        ResultFile noLobFile = new ResultFile(this.noLobOutputPath);
        ResultFile invalidLobFile = new ResultFile(this.invalidLobOutputPath);
        int threadNum = param.getThreadNum();
        Thread[] threads = new Thread[threadNum];

        try {
            // 启动 workers 工作
            for (int i = 0; i < threadNum; i++) {
                threads[i] = new Thread(
                        new TaskWorker(this, param, common, noLobFile, invalidLobFile), "Thread" + i);
            }
            for (int i = 0; i < threadNum; i++) {
                threads[i].start();
            }
            for (int i = 0; i < threadNum; i++) {
                try {
                    threads[i].join();
                } catch (Exception e) {
                    String errMsg = "Main thread join error";
                    logger.error(errMsg + ", " + e.getMessage());
                    throw new BaseException(SDBError.SDB_SYS, errMsg, e);
                }
            }
        } finally {
            noLobFile.close();
            invalidLobFile.close();
        }
    }

    /**
     * @return 获取并删除队列头的元素。返回 null 时，表示队列已经没有元素
     */
    ObjectId getObjectId() {
        ObjectId objectId = queue.poll();
        return objectId;
    }

    int getBatchSize() {
        return param.getBatchSize();
    }

    public String getMetaCLName() {
        return metaCLName;
    }

    public String getLobInfoCLName() {
        return lobInfoCLName;
    }

    public String getUpdateCLName() {
        return updateCLName;
    }
}