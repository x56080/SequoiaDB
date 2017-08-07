/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:NodeWrapper.java
 * 
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.commlib;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;

public class NodeWrapper {
    public enum NodeStatus {
        STOP_SUCCESS,
        STOP_FAILURE,
        START_SUCCESS,
        START_FAILURE
    };

    private NodeStatus status;
    private Node node;
    private BasicBSONObject nodeInfo;

    public NodeWrapper(Node node, BasicBSONObject nodeInfo) {
        this.node = node;
        this.nodeInfo = nodeInfo;
    }

    private BasicBSONObject getDataBaseSnapshot(boolean printRes) throws ReliabilityException {
        Sequoiadb sdb = null;
        BasicBSONObject retObj = null;
        try {
            sdb = node.connect();
            BSONObject nullObj = null;
            DBCursor cursor = sdb.getSnapshot(Sequoiadb.SDB_SNAP_DATABASE, nullObj, nullObj,
                    nullObj);
            while (cursor.hasNext()) {
                retObj = (BasicBSONObject) cursor.getNext();
            }
            cursor.close();
        }
        catch (BaseException e) {
            if (printRes) {
                System.out.println(node.getNodeName() + " getSnapshot( "
                        + Sequoiadb.SDB_SNAP_DATABASE + ") failed " + e.getErrorCode());
            }
            throw new ReliabilityException(e);
        }
        finally {
            if (sdb != null) {
                sdb.close();
            }
        }
        return retObj;
    }

    public boolean start() throws ReliabilityException {
        try {
            node.start();
            status = NodeStatus.START_SUCCESS;

        }
        catch (BaseException e) {
            System.out.println("start " + node.getNodeName() + " failed " + e.getErrorCode());
            status = NodeStatus.START_FAILURE;
            throw new ReliabilityException(e);
        }
        return true;
    }

    public boolean stop() throws ReliabilityException {
        try {
            node.stop();
            status = NodeStatus.STOP_SUCCESS;

        }
        catch (BaseException e) {
            System.out.println("stop " + node.getNodeName() + " failed " + e.getErrorCode());
            status = NodeStatus.STOP_FAILURE;
            throw new ReliabilityException(e);
        }
        return true;
    }

    public boolean checkStop() {
        if (status == NodeStatus.STOP_FAILURE) {
            return false;
        }
        else {
            return true;
        }
    }

    public boolean checkStart() {
        if (status == NodeStatus.START_FAILURE) {
            return false;
        }
        else {
            return true;
        }
    }

    public boolean isNodeActive() {
        try {
            Sequoiadb db = node.connect();
            BSONObject nullObj = null;
            DBCursor cursor = db.getList(Sequoiadb.SDB_LIST_CONTEXTS_CURRENT, nullObj, nullObj,
                    nullObj);
            while (cursor.hasNext()) {
                cursor.getNext();
            }
            cursor.close();
        }
        catch (BaseException e) {
            return false;
        }
        return true;
    }

    public String hostName() {
        return nodeInfo.getString("HostName");
    }

    public int nodeID() {
        return nodeInfo.getInt("NodeID");
    }

    public String svcName() {
        return ((BasicBSONObject) ((BasicBSONList) nodeInfo.get("Service")).get(0))
                .getString("Name");
    }

    public String dbPath() {
        return nodeInfo.getString("dbpath");
    }

    public boolean isMaster() throws ReliabilityException {
        return isMaster(true);
    }

    public boolean isMaster(boolean printException) throws ReliabilityException {
        BasicBSONObject obj = getDataBaseSnapshot(printException);
        if (obj != null) {
            return obj.getBoolean("IsPrimary");
        }

        return false;
    }

    public NodeCheckResult checkBusiness(boolean printRes) {
        NodeCheckResult checkResult = new NodeCheckResult();
        checkResult.hostName = hostName();
        checkResult.nodeID = nodeID();
        checkResult.svcName = svcName();

        int svcPort = Integer.parseInt(checkResult.svcName);
        if (svcPort >= SdbTestBase.reservedPortBegin && svcPort <= SdbTestBase.reservedPortEnd) {
            checkResult.isInDeploy = false;
        }

        try {
            BasicBSONObject obj = getDataBaseSnapshot(printRes);
            checkResult.serviceStatus = obj.getBoolean("ServiceStatus");
            checkResult.connect = true;
            checkResult.LSN = ((BasicBSONObject) obj.get("CurrentLSN")).getLong("Offset");
            checkResult.isPrimary = obj.getBoolean("IsPrimary");
            checkResult.freeSpace = ((BasicBSONObject) obj.get("Disk")).getLong("FreeSpace");
        }
        catch (ReliabilityException e) {
            checkResult.connect = false;
        }

        return checkResult;
    }

    public void backupDiaglog(String testCaseName) throws ReliabilityException {

        Ssh remote = new Ssh(hostName(), SdbTestBase.remoteUser, SdbTestBase.remotePwd);
        remote.exec(String.format("cp -r %s/diaglog %s/backup_%s", dbPath(), SdbTestBase.workDir,
                testCaseName));
        if (0 != remote.getExitStatus()) {
            throw new ReliabilityException(
                    "stdout:" + remote.getStdout() + "\nstderr:" + remote.getStderr());
        }
    }

    public Sequoiadb connect() {
        Sequoiadb db = new Sequoiadb(hostName() + ":" + svcName(), "", "");
        return db;
    }
}
