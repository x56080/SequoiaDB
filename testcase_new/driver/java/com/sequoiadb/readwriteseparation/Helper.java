package com.sequoiadb.readwriteseparation;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by laojingtang on 18-1-19.
 */
class Helper {
    /**
     * get node
     *
     * @param db
     * @param groupName
     * @return
     */
    public static List<NodeWarrper> getNodeList(Sequoiadb db, String groupName) {
        List<NodeWarrper> nodeList = new ArrayList<>(5);
        ReplicaGroup rg = db.getReplicaGroup(groupName);
        BSONObject detail = rg.getDetail();
        BasicBSONList group = (BasicBSONList) detail.get("Group");
        for (Object node : group) {
            BSONObject temp = (BSONObject) node;
            String hostName = (String) temp.get("HostName");
            String port = null;
            BasicBSONList service = (BasicBSONList) temp.get("Service");
            for (Object o2 : service) {
                BSONObject o2Tmp = (BSONObject) o2;
                if (o2Tmp.get("Type").equals(0)) {
                    port = (String) o2Tmp.get("Name");
                }
            }
            String nodeName = hostName + ":" + port;
            Integer instanceid = (Integer) temp.get("instanceid");

            NodeWarrper nodeInfo = new NodeWarrper().setNodeName(nodeName);
            if (instanceid != null) {
                nodeInfo.setInstenceid(instanceid);
            }
            if (rg.getMaster().getNodeName().trim().equals(nodeInfo.getNodeName().trim())) {
                nodeInfo.setMaster(true);
            } else {
                nodeInfo.setMaster(false);
            }
            nodeList.add(nodeInfo);
        }
        return nodeList;
    }

    /**
     * 通过explain找实际访问的数据节点名称
     *
     * @param dbcl
     * @return
     */
    public static String getActualDataNodeName(DBCollection dbcl) {
        final int retryTimes = 10;
        for (int i = 0; i < retryTimes; i++) {
            try {
                DBCursor cur = dbcl.explain(null, null, null, null, 0, 10, 0, new BasicBSONObject("Run", true));
                BSONObject o = cur.getNext();
                String nodeName = (String) o.get("NodeName");
                cur.close();
                return nodeName;
            } catch (BaseException e) {
                if (i < retryTimes - 1) {
                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException e1) {
                        e1.printStackTrace();
                    }
                    //e.printStackTrace();
                } else {
                    throw e;
                }
            }
        }
        //should never come here
        return "";
    }

}

