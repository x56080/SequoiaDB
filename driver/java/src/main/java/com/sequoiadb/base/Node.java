/**
 * Copyright (C) 2018 SequoiaDB Inc.
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
package com.sequoiadb.base;

import com.sequoiadb.base.SequoiadbConstants.Operation;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.util.SDBMessageHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.nio.ByteBuffer;

/**
 * @class Node
 * @brief Database operation interfaces of node.This class takes the place of class "replicaNode".
 * @note We use concept "node" instead of "replica node",
 * and change the class name "ReplicaNode" to "Node".
 */
public class Node {
    private String hostName;
    private int port;
    private String nodeName;
    private int id;
    private ReplicaGroup rg;
    private Sequoiadb ddb;
    private NodeStatus status;

    Node(String hostName, int port, int nodeId, ReplicaGroup rg) {
        this.rg = rg;
        this.hostName = hostName;
        this.port = port;
        this.nodeName = hostName + SequoiadbConstants.NODE_NAME_SEP + port;
        this.id = nodeId;
    }

    /*!
     * enum Node::NodeStatus
     */
    public static enum NodeStatus {
        SDB_NODE_ALL(1),
        SDB_NODE_ACTIVE(2),
        SDB_NODE_INACTIVE(3),
        SDB_NODE_UNKNOWN(4);
        private final int key;

        private NodeStatus(int key) {
            this.key = key;
        }

        public int getKey() {
            return key;
        }

        public static NodeStatus getByKey(int key) {
            NodeStatus nodeStatus = NodeStatus.SDB_NODE_ALL;
            for (NodeStatus status : NodeStatus.values()) {
                if (status.getKey() == key) {
                    nodeStatus = status;
                    break;
                }
            }
            return nodeStatus;
        }
    }

    /**
     * @return Current node's id.
     * @fn int getNodeId()
     * @brief Get current node's id.
     */
    public int getNodeId() {
        return id;
    }

    /**
     * @return Current node's parent replica group.
     * @fn ReplicaGroup getReplicaGroup()
     * @brief Get current node's parent replica group.
     */
    public ReplicaGroup getReplicaGroup() {
        return rg;
    }

    /**
     * @return void
     * @throws com.sequoiadb.exception.BaseException
     * @fn void disconnect()
     * @brief Disconnect from current node.
     */
    public void disconnect() throws BaseException {
        ddb.disconnect();
    }

    /**
     * @return The Sequoiadb object of current node.
     * @throws com.sequoiadb.exception.BaseException
     * @fn Sequoiadb connect ()
     * @brief Connect to current node with the same username and password of coordination node.
     */
    public Sequoiadb connect() throws BaseException {
        ddb = new Sequoiadb(hostName, port, rg.getSequoiadb().getUserName(),
                rg.getSequoiadb().getPassword());
        return ddb;
    }

    /**
     * @param username user name
     * @param password pass word
     * @return The Sequoiadb object of current node.
     * @throws com.sequoiadb.exception.BaseException
     * @fn Sequoiadb connect(String username, String password)
     * @brief Connect to current node with username and password.
     */
    public Sequoiadb connect(String username, String password) throws BaseException {
        ddb = new Sequoiadb(hostName, port, username, password);
        return ddb;
    }

    /**
     * @return The Sequoiadb object of current node or null for having not
     * connected to the current node yet.
     * @fn Sequoiadb getSdb()
     * @brief Get the Sequoiadb of current node.
     * @see Node#connect()
     * @see Node#connect(String, String)
     */
    public Sequoiadb getSdb() {
        return ddb;
    }

    /**
     * @return Hostname of current node.
     * @fn String getHostName()
     * @brief Get the hostname of current node.
     */
    public String getHostName() {
        return hostName;
    }

    /**
     * @return The port of current node.
     * @fn int getPort()
     * @brief Get the port of current node.
     */
    public int getPort() {
        return port;
    }

    /**
     * @return The name of current node.
     * @fn String getNodeName()
     * @brief Get the name of current node.
     */
    public String getNodeName() {
        return nodeName;
    }

    /**
     * @return The status of current node.
     * @throws com.sequoiadb.exception.BaseException
     * @fn NodeStatus getStatus()
     * @brief Get the status of current node.
     */
    public NodeStatus getStatus() throws BaseException {
        BSONObject obj = new BasicBSONObject();
        obj.put(SequoiadbConstants.FIELD_NAME_GROUPID, rg.getId());
        obj.put(SequoiadbConstants.FIELD_NAME_NODEID, id);
        String commandString = SequoiadbConstants.SNAP_CMD + " "
                + SequoiadbConstants.DATABASE;
        SDBMessage rtn = adminCommand(commandString, obj);
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SDBError.SDB_NET_CANNOT_CONNECT.getErrorCode()) {
                status = NodeStatus.SDB_NODE_INACTIVE;
                return status;
            } else {
                throw new BaseException(flags);
            }
        }
        status = NodeStatus.SDB_NODE_ACTIVE;
        return status;
    }

    /**
     * @return void
     * @throws com.sequoiadb.exception.BaseException
     * @fn void start()
     * @brief Start current node in database.
     */
    public void start() throws BaseException {
        startStop(true);
    }

    /**
     * @return void
     * @throws com.sequoiadb.exception.BaseException
     * @fn void stop()
     * @brief Stop current node in database.
     */
    public void stop() throws BaseException {
        startStop(false);
    }

    private void startStop(boolean status) {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        SDBMessage rtn = adminCommand(
                status ? SequoiadbConstants.CMD_NAME_STARTUP_NODE
                        : SequoiadbConstants.CMD_NAME_SHUTDOWN_NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            String msg = "node = " + hostName + ":" + port;
            throw new BaseException(flags, msg);
        }
    }

    private SDBMessage adminCommand(String commandString, BSONObject obj)
            throws BaseException {
        // Admin command request
        // int reqId = 0;
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage sdbMessage = new SDBMessage();
        sdbMessage.setMatcher(obj);
        sdbMessage.setCollectionFullName(SequoiadbConstants.ADMIN_PROMPT + commandString);

        sdbMessage.setVersion(1);
        sdbMessage.setW((short) 0);
        sdbMessage.setPadding((short) 0);
        sdbMessage.setFlags(0);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        // sdbMessage.setResponseTo(reqId);
        // reqId++;
        sdbMessage.setRequestID(rg.getSequoiadb().getNextRequstID());
        sdbMessage.setSkipRowsCount(-1);
        sdbMessage.setReturnRowsCount(-1);
        sdbMessage.setSelector(dummyObj);
        sdbMessage.setOrderBy(dummyObj);
        sdbMessage.setHint(dummyObj);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        boolean endianConver = this.rg.getSequoiadb().endianConvert;
        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage,
                endianConver);
        this.rg.getSequoiadb().getConnection().sendMessage(request);

        ByteBuffer byteBuffer = this.rg.getSequoiadb().getConnection().receiveMessage(endianConver);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);

        return rtnSDBMessage;
    }

}
