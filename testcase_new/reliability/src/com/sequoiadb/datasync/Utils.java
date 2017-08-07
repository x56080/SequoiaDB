package com.sequoiadb.datasync;

import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupCheckResult;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

public class Utils {
    /**
     * 当同步日志未写满，依然能找到第一条同步日志时，新增节点依然会增量同步，
     * 为了构造全量同步，需用此方法确保组上的同步日志已经写了一圈。
     * @param groupName
     */
    public static void makeReplicaLogFull(String groupName) {
        try (Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")) {
            ReplicaGroup group = db.getReplicaGroup(groupName);
            try (Sequoiadb dataDB = group.getMaster().connect()) {
                long fullLSN = 20 * 64 * 1024 * 1024; // 默认20份同步日志，1份64M
                long currentLSN = 0;
                
                if (getCurrentLSN(dataDB) > fullLSN) {
                    return ;
                }
                
                System.out.println("fullLSN: " + fullLSN);
                String tmpCSName = "csToMakeRgLogFull";
                String tmpCLName = "clToMakeRgLogFull";
                DBCollection tmpCL = db.createCollectionSpace(tmpCSName).createCollection(tmpCLName, new BasicBSONObject("Group", groupName));
                byte[] lobBytes = new byte[64 * 1024 * 1024];
                while (currentLSN <= fullLSN) {
                    DBLob lob = tmpCL.createLob();
                    lob.write(lobBytes);
                    lob.close();
                    currentLSN = getCurrentLSN(dataDB);
                    System.out.println("currentLSN: " + currentLSN);
                }
                db.dropCollectionSpace(tmpCSName);
            }
        }
    }
    
    private static long getCurrentLSN(Sequoiadb dataDB) {
        DBCursor cursor = dataDB.getSnapshot(Sequoiadb.SDB_SNAP_DATABASE, "{}", "{}", "{}");
        BSONObject CurrentLSN = (BSONObject) cursor.getNext().get("CurrentLSN");
        long currentLSN = (long) CurrentLSN.get("Offset");
        cursor.close();
        return currentLSN;
    }
    
    /**
     * 框架提供的checkBusiness接口会排斥新建节点，认为是组部署异常。
     * 因此要重新定制一个检查集群的方法。
     * @param timeOutSecond
     * @throws ReliabilityException 
     */
    public static boolean checkBusinessWithExNode(GroupMgr groupMgr, int timeOutSecond) throws ReliabilityException {
        for (int i = 0; i < timeOutSecond; i++) {
            List<String> groupNames = groupMgr.getAllDataGroupName();
            groupNames.remove("SYSCoord");
            boolean ok = true;
            
            try {
                for (String groupName : groupNames) {
                    GroupWrapper group = groupMgr.getGroupByName(groupName);
                    GroupCheckResult grpRes = group.checkBusiness(false);
                    if (!(grpRes.connCheck 
                            && grpRes.primaryCheck 
                            && grpRes.serviceCheck
                            && grpRes.LSNCheck)) {
                        ok = false;
                    }
                }
            } catch (Exception e) {
                ok = false;
            }
            
            if (ok) { return true; }
            
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
            }
        }
        return false;
    }
    
    public static void testLob(Sequoiadb db, String clName) throws ReliabilityException {
        DBCollection cl = db.getCollectionSpace(SdbTestBase.csName).getCollection(clName);
        int lobSize = 1 * 1024;
        byte[] lobBytes = new byte[lobSize];
        new Random().nextBytes(lobBytes);
        
        DBLob wLob = cl.createLob();
        wLob.write(lobBytes);
        ObjectId oid = wLob.getID();
        wLob.close();
        
        DBLob rLob = cl.openLob(oid);
        byte[] rLobBytes = new byte[lobSize];
        rLob.read(rLobBytes);
        rLob.close();
        
        if (!Arrays.equals(rLobBytes, lobBytes)) {
            throw new ReliabilityException("lob is different");
        }
    }
    
    public static String getKeyStack(Exception e, Object classObj) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for (int i = 0; i < stackElements.length; i++) {
            if (stackElements[i].toString().contains(classObj.getClass().getName())) {
                stackBuffer.append(stackElements[i].toString()).append("\r\n");
            }
        }
        String str = stackBuffer.toString();
        if (str.length() >= 2) {
            return str.substring(0, str.length() - 2);
        } else {
            return str;
        }
    }
}
