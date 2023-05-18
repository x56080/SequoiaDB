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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

public class Helper {
    private static final Logger logger = LoggerFactory.getLogger(Helper.class);
    public static final long timestamp = System.currentTimeMillis();

    public static List<String> getAllNode(Sequoiadb sdb, String groupName) {
        List<String> nodeList1 = new ArrayList<>();
        List<String> nodeList2 = new ArrayList<>();

        DBCursor cursor = null;
        try {
            // get group list
            cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS,
                    new BasicBSONObject("GroupName", groupName), null, null);
            BSONObject snapObj = cursor.getNext();
            if (snapObj == null) {
                throw new BaseException(SDBError.SDB_SYS, "Failed to get list of group: " + groupName);
            }

            // get PrimaryNode Id
            int primaryNodeId = (int) snapObj.get("PrimaryNode");
            // get nodes in this group
            BasicBSONList bsonList = (BasicBSONList) snapObj.get("Group");
            if (bsonList.isEmpty()) {
                throw new BaseException(SDBError.SDB_SYS, "No nodes info for group: " + groupName);
            }
            // loop to get all the node name of this group
            for (Object obj : bsonList) {
                // build NodeName, like "ubuntu-dev1:2000"
                BSONObject o = (BSONObject) obj;
                String hostName = (String) o.get("HostName");
                String svcName = "";
                int nodeId = (int) o.get("NodeID");
                BasicBSONList svcList = (BasicBSONList) o.get("Service");
                for (Object svc : svcList) {
                    // get local svc name
                    BSONObject svcObj = (BSONObject) svc;
                    int type = (int) svcObj.get("Type");
                    if (type == 0) {
                        svcName = (String) svcObj.get("Name");
                        break;
                    }
                }
                if (svcName.isEmpty()) {
                    throw new BaseException(SDBError.SDB_SYS, "Invalid group info for: " + groupName);
                }
                String nodeName = hostName + ":" + svcName;
                if (primaryNodeId == nodeId) {
                    // let primary node put at the first place
                    nodeList1.add(nodeName);
                } else {
                    nodeList2.add(nodeName);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (nodeList1.isEmpty()) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to get primary node for group: " + groupName);
        }
        if (!nodeList2.isEmpty()) {
            nodeList1.addAll(nodeList2);
        }
        return nodeList1;
    }

    public static long printStatistics(String taskInfo, long startTM, long lastTM, int fixedCount) {
        return printStatistics(taskInfo, startTM, lastTM, fixedCount, 300); // 大于 5min 打印一次统计信息
    }

    public static long printStatistics(String taskInfo, long startTM, long lastTM, int fixedCount, int intervalSec) {
        long currentTM = System.currentTimeMillis();
        if ((currentTM - lastTM) >= (intervalSec * 1000L)) {
            long deltaTM = currentTM - startTM;
            long sec = deltaTM / 1000L;
            long millSec = deltaTM % 1000L;
            logger.info("Job[" + taskInfo + "] worker[" + Thread.currentThread().getName() +
                    "] " + "has run: " + sec + "." + millSec + "(secs), and has handled: " + fixedCount + " records");
            return currentTM;
        } else {
            return lastTM;
        }
    }

}
