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

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

class JobInfo {
    private int JobId;
    private String CollectionName;
    private String GroupName;
    private ObjectId LastOid;
    // SKIP/DONE/TODO
    private String Status;
    // the first element is master
    private List<String> nodeList;

    public final static String STATUS_TODO = "TODO";
    public final static String STATUS_SKIP = "SKIP";
    public final static String STATUS_DONE = "DONE";

    public int getJobId() {
        return JobId;
    }

    public void setJobId(int jobId) {
        JobId = jobId;
    }

    public String getCollectionName() {
        return CollectionName;
    }

    public void setCollectionName(String collectionName) {
        CollectionName = collectionName;
    }

    public String getGroupName() {
        return GroupName;
    }

    public void setGroupName(String groupName) {
        GroupName = groupName;
    }

    public ObjectId getLastOid() {
        return LastOid;
    }

    public void setLastOid(ObjectId lastOid) {
        LastOid = lastOid;
    }

    public String getStatus() {
        return Status;
    }

    public void setStatus(String status) {
        Status = status;
    }

    public List<String> getNodeList() {
        return nodeList;
    }

    public void setNodeList(List<String> nodeList) {
        this.nodeList = nodeList;
    }

    public JobInfo() {
        JobId = -1;
        CollectionName = "";
        GroupName = "";
        LastOid = null;
        Status = STATUS_TODO;
        nodeList = null;
    }

    public JobInfo(int jobId, String collectionName, String groupName, ObjectId lastOid, String status) {
        JobId = jobId;
        CollectionName = collectionName;
        GroupName = groupName;
        LastOid = lastOid;
        Status = status;
        nodeList = null;
    }

    public JobInfo(BSONObject baseInfo) {
        this((Integer) baseInfo.get("JobId"),
                (String) baseInfo.get("CollectionName"),
                (String) baseInfo.get("GroupName"),
                (ObjectId) baseInfo.get("LastOid"),
                (String) baseInfo.get("Status"));
    }

    public String getBaseInfo() {
        return "{" +
                "JobId:" + JobId +
                ", CollectionName:'" + CollectionName + '\'' +
                ", GroupName:'" + GroupName + '\'' +
                (LastOid == null ? ", LastOid:" + LastOid : ", LastOid: { '$oid' : '" + LastOid + "' }") +
                ", Status:'" + Status + '\'' +
                '}';
    }

    @Override
    public String toString() {
        return "JobInfo{" +
                "JobId=" + JobId +
                ", CollectionName='" + CollectionName + '\'' +
                ", GroupName='" + GroupName + '\'' +
                (LastOid == null ? ", LastOid=" + LastOid : ", LastOid: { '$oid' : '" + LastOid + "' }") +
                ", Status='" + Status + '\'' +
                ", nodeList=" + nodeList +
                '}';
    }
}

public class JobMgr {
    private static final Logger logger = LoggerFactory.getLogger(JobMgr.class);
    private SequoiadbDatasource datasource;
    private String jobFileName;
    private TreeMap<Integer, JobInfo> allJobMap = new TreeMap<>();
    private TreeMap<Integer, JobInfo> todoJobMap = new TreeMap<>();

    public JobMgr(SequoiadbDatasource datasource, String jobFileName) {
        this.datasource = datasource;
        this.jobFileName = jobFileName;
    }

    /**
     * Generating job infos, and insert them into the job file for the input collections.
     * job info is like:
     * {JobId:xxx, CollectionName:"xxx", GroupName:"xxx", LastOid:"xxx", Status:"xxx"}
     */
    public void initJobFile(List<String> fullNameList) {
        // create a new job file
        File file = new File(jobFileName);
        if (file.exists()) {
            if (file.delete()) {
                logger.info("Deleted the existed job file: " + jobFileName);
            } else {
                String errMsg = "Failed to delete the existed job file: " + jobFileName;
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }
        }
        try {
            file.createNewFile();
        } catch (Exception e) {
            String errMsg = "Failed to create a new job file: " + jobFileName;
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
        logger.info("Success to create a new job file: " + jobFileName);

        // generating job infos by the collections
        allJobMap.clear();
        int jobID = 1;
        for (String fullName : fullNameList) {
            List<JobInfo> jobInfos = buildJobInfo(fullName);
            if (jobInfos != null && jobInfos.size() > 0) {
                for (JobInfo jobInfo : jobInfos) {
                    jobInfo.setJobId(jobID);
                    allJobMap.put(jobID, jobInfo);
                    jobID++;
                }
            }
        }
        if (allJobMap.isEmpty()) {
            throw new BaseException(SDBError.SDB_SYS, "No job record for job file: " + jobFileName);
        }

        // flush to job file
        flushJobRecords();
    }

    private List<JobInfo> buildJobInfo(String fullName) {
        List<JobInfo> jobInfoList = new ArrayList<>();

        Sequoiadb sdb = getSdb();

        boolean isMainCL = false;
        DBCursor cursor = null;
        try {
            // get snapshot 8
            cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_CATALOG,
                    new BasicBSONObject("Name", fullName), null, null, null, 0, -1);
            BSONObject snapObj = cursor.getNext();
            if (snapObj == null) {
                String errMsg = "Failed to get snapshot of collection: " + fullName;
                logger.error(errMsg);
                throw new BaseException(SDBError.SDB_SYS, errMsg);
            }

            // build job info
            if (snapObj.containsField("IsMainCL")) {
                isMainCL = (boolean) snapObj.get("IsMainCL");
            }
            if (isMainCL) { // handle main cl
                List<String> subCLNames = new ArrayList<>();
                // when it is an empty main cl, let's skip it
                if (!snapObj.containsField("CataInfo")) {
                    logger.warn("No sub cl info for main collection: " + fullName +
                            ", ignore it, snapObj is: " + snapObj.toString());
                    return jobInfoList;
                }
                // when no elements in array, let's skip it
                BasicBSONList bsonList = (BasicBSONList) snapObj.get("CataInfo");
                if (bsonList.isEmpty()) {
                    logger.warn("No sub cl info for main collection: " + fullName +
                            ", ignore it, snapObj is: " + snapObj.toString());
                    return jobInfoList;
                }
                // loop to get all the sub cl name
                for (Object obj : bsonList) {
                    BSONObject o = (BSONObject) obj;
                    if (!o.containsField("SubCLName")) {
                        throw new BaseException(SDBError.SDB_SYS, "Failed to get sub cl name for collection: " +
                                fullName + ", snapObj is: " + snapObj.toString());
                    }
                    String subCLName = (String) o.get("SubCLName");
                    if (!subCLNames.contains(subCLName)) {
                        subCLNames.add(subCLName);
                    }
                }
                // loop to build all the job info of the sub cl
                for (String name : subCLNames) {
                    List<JobInfo> list = buildJobInfo(name);
                    if (list.size() > 0) {
                        jobInfoList.addAll(list);
                    }
                }
            } else { // handle cl (not main cl)
                // when has no group info, let's stop
                if (!snapObj.containsField("CataInfo")) {
                    throw new BaseException(SDBError.SDB_SYS, "No group info for collection: " + fullName +
                            ", snapObj is: " + snapObj.toString());
                }
                // when no elements in array, let's stop
                BasicBSONList bsonList = (BasicBSONList) snapObj.get("CataInfo");
                if (bsonList.isEmpty()) {
                    throw new BaseException(SDBError.SDB_SYS, "No group info for collection: " + fullName +
                            ", snapObj is: " + snapObj.toString());
                }
                // loop to get groups of current collection and build the job info
                for (Object obj : bsonList) {
                    BSONObject o = (BSONObject) obj;
                    if (!o.containsField("GroupName")) {
                        throw new BaseException(SDBError.SDB_SYS, "Failed to get group name for collection: " + fullName
                                + ", snapObj is: " + snapObj.toString());
                    }
                    String groupName = (String) o.get("GroupName");
                    // build job info
                    JobInfo jobInfo = new JobInfo();
                    jobInfo.setCollectionName(fullName);
                    jobInfo.setGroupName(groupName);
                    jobInfoList.add(jobInfo);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (sdb != null) {
                releaseSdb(sdb);
            }
        }
        return jobInfoList;
    }

    public synchronized void loadJobRecords() {
        // get connection
        Sequoiadb sdb = getSdb();

        // load job records
        try {
            BufferedReader reader = new BufferedReader(new FileReader(jobFileName));
            String line;
            while ((line = reader.readLine()) != null) {
                // 跳过空行
                if (line.isEmpty()) {
                    continue;
                }
                BSONObject obj = (BSONObject) JSON.parse(line);
                JobInfo jobInfo = new JobInfo(obj);
                allJobMap.put(jobInfo.getJobId(), jobInfo);
                // add job info to todoMap for workers
                if (jobInfo.getStatus().equals(JobInfo.STATUS_TODO)) {
                    // get nodes of group
                    List<String> nodeList = Helper.getAllNode(sdb, jobInfo.getGroupName());
                    jobInfo.setNodeList(nodeList);
                    todoJobMap.put(jobInfo.getJobId(), jobInfo);
                }
            }
            reader.close();
        } catch (FileNotFoundException e) {
            String errMsg = "Job File " + jobFileName + " does not exist";
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_INVALIDARG, errMsg, e);
        } catch (Exception e) {
            String errMsg = "Failed to load job file: " + jobFileName;
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        } finally {
            if (sdb != null) {
                releaseSdb(sdb);
            }
        }
    }

    private synchronized void flushJobRecords() {
        try {
            // 新的内容会覆盖文件原来的内容。哪怕新的内容比原来的少，也不会在文件末尾遗留垃圾信息
            BufferedWriter writer = new BufferedWriter(new FileWriter(jobFileName));
            for (JobInfo jobInfo : allJobMap.values()) {
                writer.write(jobInfo.getBaseInfo());
                writer.newLine();
            }
            writer.flush();
            writer.close();
        } catch (FileNotFoundException e) {
            String errMsg = "Job File " + jobFileName + " does not exist";
            logger.error(errMsg);
            throw new BaseException(SDBError.SDB_INVALIDARG, errMsg, e);
        } catch (Exception e) {
            String errMsg = "Failed to flush job records to file: " + jobFileName;
            logger.error(errMsg + ", " + e.getMessage());
            throw new BaseException(SDBError.SDB_SYS, errMsg, e);
        }
    }

    public synchronized JobInfo getJobRecord() {
        if (todoJobMap.isEmpty()) {
            return null;
        } else {
            // 弹出键值(JobId) 最小的元素
            Map.Entry<Integer, JobInfo> firstEntry = todoJobMap.pollFirstEntry();
            return firstEntry.getValue();
        }
    }

    public synchronized void updateJobRecord(JobInfo jobInfo) {
        allJobMap.put(jobInfo.getJobId(), jobInfo);
        flushJobRecords();
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
}
