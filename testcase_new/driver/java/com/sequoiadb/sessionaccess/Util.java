package com.sequoiadb.sessionaccess;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;

public class Util extends SdbTestBase {
	
    public static  BasicBSONList createRG(Sequoiadb db, String rgName) {
    	int reservedPortBegin = SdbTestBase.reservedPortBegin;
        ReplicaGroup rg = db.createReplicaGroup(rgName);
        int[][] param = {{reservedPortBegin+10, 7}, {reservedPortBegin+20, 8}, {reservedPortBegin+30, 9},};
        String hostName=db.getReplicaGroup("SYSCatalogGroup").getMaster().getHostName();
        
        BasicBSONList nodes = new BasicBSONList();
        for (int[] ints : param) {
            BasicBSONObject config = new BasicBSONObject();
            config.append("instanceid", ints[1]);
            rg.createNode(hostName, ints[0], SdbTestBase.reservedDir + "/data/" + ints[0], config);
            
            BSONObject node = new BasicBSONObject();
            node.put("nodeName", hostName + ":" + String.valueOf(ints[0]));
            node.put("instanceid", ints[1]);
            nodes.add(node);
        }
        rg.start();
        return nodes;
    }
    
    public static boolean isMaster(Sequoiadb db, String groupName, String nodeName){
    	ReplicaGroup rg = db.getReplicaGroup(groupName);
    	if (rg.getMaster().getNodeName().trim().equals(nodeName)) {
            return true;
        } else {
            return false;
        }
    }
    
    
    public static int getInstanceidByNodeName(BasicBSONList nodes, String nodeName){
    	for(int i = 0 ; i < nodes.size() ; i++){
    		BasicBSONObject node = (BasicBSONObject)nodes.get(i);
    		if(node.getString("nodeName").equals(nodeName)){
    			return Integer.parseInt(node.getString("instanceid"));
    		}
    	}
    	return -1;
    }
    
    public static String getNodeNameByInstanceId(BasicBSONList nodes, String instanceId){
    	for(int i = 0 ; i < nodes.size() ; i++){
    		BasicBSONObject node = (BasicBSONObject)nodes.get(i);
    		if(node.getString("instanceid").equals(instanceId)){
    			return node.getString("nodeName");
    		}
    	}
    	return "";
    }

    public static List<Integer> getInstanceidList(BasicBSONList nodes){
    	List<Integer> instanceidList = new ArrayList<Integer>();
        for (int i=0 ; i< nodes.size() ; i++) {
        	BasicBSONObject node = (BasicBSONObject)nodes.get(i);
        	instanceidList.add(Integer.parseInt(node.getString("instanceid")));
        }
    	return instanceidList;
    }
    
    public static String getActualDataNodeName(DBCollection dbcl) {
        DBCursor cur = dbcl.explain(null, null, null, null, 0, 10, 0, new BasicBSONObject("Run", true));
        BSONObject o = cur.getNext();
        String nodeName = (String) o.get("NodeName");
        cur.close();
        return nodeName;
    }
    
    public static void insertRecords(DBCollection dbcl){
    	for(int i=0 ; i < 1000 ; i ++){
        	BSONObject obj = new BasicBSONObject();
        	obj.put("a", i);
        	dbcl.insert(obj);	
        }
    }
}
