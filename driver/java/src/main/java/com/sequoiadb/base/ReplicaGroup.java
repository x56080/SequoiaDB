/**
 * Copyright (C) 2012 SequoiaDB Inc.
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
 *
 * @package com.sequoiadb.base;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
package com.sequoiadb.base;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.SequoiadbConstants.Operation;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.util.SDBMessageHelper;

/**
 * @class ReplicaGroup
 * @brief Database operation interfaces of replica group.
 */
public class ReplicaGroup {
    private String name;
    private int id;
    private Sequoiadb sequoiadb;
    private boolean isCataRG;


    /**
     * @fn Sequoiadb getSequoiadb()
     * @brief Get current replica group's Sequoiadb.
     * @return the current replica group's Sequoiadb
     */
    public Sequoiadb getSequoiadb() {
        return sequoiadb;
    }

    /**
     * @fn int getId()
     * @brief Get current replica group's id.
     * @return the current replica group's id
     */
    public int getId() {
        return id;
    }

    /**
     * @fn String getGroupName()
     * @brief Get current replica group's name.
     * @return the current replica group's name
     */
    public String getGroupName() {
        return name;
    }

    ReplicaGroup(Sequoiadb sdb, int id) {
        this.sequoiadb = sdb;
        this.id = id;
        BSONObject group = sdb.getDetailById(id);
        this.name = group.get(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                .toString();
        this.isCataRG = name.equals(Sequoiadb.CATALOG_GROUP_NAME);
    }

    ReplicaGroup(Sequoiadb sdb, String name) {
        this.sequoiadb = sdb;
        this.name = name;
        BSONObject group = sdb.getDetailByName(name);
        this.isCataRG = (name == Sequoiadb.CATALOG_GROUP_NAME);
        this.id = Integer.parseInt(group.get(
                SequoiadbConstants.FIELD_NAME_GROUPID).toString());
    }

    /**
     * @fn int getNodeNum(Node.NodeStatus status)
     * @brief Get the amount of the nodes with the specified status.
     * @param status
     * 			Node.NodeStatus
     * @return the amount of the nodes with the specified status
     * @exception com.sequoiadb.exception.BaseException
     */
    public int getNodeNum(Node.NodeStatus status) throws BaseException {
        BSONObject group = sequoiadb.getDetailById(id);
        try {
            Object obj = group.get(SequoiadbConstants.FIELD_NAME_GROUP);
            if (obj == null)
                return 0;
            BasicBSONList list = (BasicBSONList) obj;
            return list.size();
        } catch (BaseException e) {
            throw e;
        } catch (Exception e) {
            throw new BaseException("SDB_SYS", e);
        }
    }

    /**
     * @fn BSONObject getDetail()
     * @brief Get detail info of current replicaGoup
     * @return the detail info
     * @exception com.sequoiadb.exception.BaseException
     */
    public BSONObject getDetail() throws BaseException {
        return sequoiadb.getDetailById(id);
    }

    /**
     * @fn Node getMaster()
     * @brief Get the master node of current replica group.
     * @return the master node
     * @exception com.sequoiadb.exception.BaseException
     */
    public Node getMaster() throws BaseException {
        // get information of nodes from catalog
        BSONObject groupInfoObj = sequoiadb.getDetailById(id);
        if (groupInfoObj == null) {
            throw new BaseException("SDB_CLS_GRP_NOT_EXIST",
                    String.format("no information of group id[%d]", id));
        }
        // check the nodes in current group
        Object nodesInfoArr = groupInfoObj.get(SequoiadbConstants.FIELD_NAME_GROUP);
        if (nodesInfoArr == null || !(nodesInfoArr instanceof BasicBSONList)) {
            throw new BaseException("SDB_SYS",
                    String.format("invalid content[%s] of field[%s]",
                            nodesInfoArr == null ? "null" : nodesInfoArr.toString(), SequoiadbConstants.FIELD_NAME_GROUP));
        }
        BasicBSONList nodesInfoList = (BasicBSONList) nodesInfoArr;
        if (nodesInfoList.isEmpty()) {
            throw new BaseException("SDB_CLS_EMPTY_GROUP");
        }
        // check and extract the information of primary node
        Object primaryNodeObj = groupInfoObj.get(SequoiadbConstants.FIELD_NAME_PRIMARY);
        if (primaryNodeObj == null) {
            throw new BaseException("SDB_RTN_NO_PRIMARY_FOUND");
        } else if (!(primaryNodeObj instanceof Number)) {
            throw new BaseException("SDB_SYS", "invalid primary node's information: " + primaryNodeObj.toString());
        } else if (primaryNodeObj.equals(Integer.valueOf(-1))) {
            throw new BaseException("SDB_RTN_NO_PRIMARY_FOUND");
        }
        BSONObject primaryData = null;
        Object nodeId;
        for (Object nodeInfoObj : nodesInfoList) {
            BSONObject nodeInfo = (BSONObject) nodeInfoObj;
            nodeId = nodeInfo.get(SequoiadbConstants.FIELD_NAME_NODEID);
            if (nodeId == null) {
                throw new BaseException("SDB_SYS", "node id can not be null");
            }
            if (nodeId.equals(primaryNodeObj)) {
                primaryData = nodeInfo;
                break;
            }
        }
        // try to get the meta information of primary node.
        if (primaryData == null) {
            throw new BaseException("SDB_SYS", "no information about the primary node in node array");
        }
        nodeId = primaryData.get(SequoiadbConstants.FIELD_NAME_NODEID);
        if (nodeId == null || !(nodeId instanceof Number)) {
            throw new BaseException("SDB_SYS",
                    String.format("invalid content[%s] of field[%s]",
                            nodeId == null ? "null" : nodeId.toString(), SequoiadbConstants.FIELD_NAME_NODEID));
        }
        Object hostNameObj = primaryData.get(SequoiadbConstants.FIELD_NAME_HOST);
        if (hostNameObj == null || !(hostNameObj instanceof String)) {
            throw new BaseException("SDB_SYS",
                    String.format("invalid content[%s] of field[%s]",
                            hostNameObj == null ? "null" : hostNameObj.toString(), SequoiadbConstants.FIELD_NAME_HOST));
        }
        String hostName = hostNameObj.toString();
        int port = getNodePort(primaryData);
        return new Node(hostName, port, Integer.parseInt(nodeId.toString()), this);
    }

    /**
     * @fn Node getSlave()
     * @brief Get the random slave of current replica group.
     * @return the slave node
     * @exception com.sequoiadb.exception.BaseException
     */
    public Node getSlave() throws BaseException {
        List<Integer> list = new ArrayList<Integer>();
        return getSlave(list);
    }

    private Node getSlave(List<Integer> positions) throws BaseException {
        boolean needGeneratePosition = false;
        List<Integer> validPositions = new ArrayList<Integer>();
        // check arguments
        if (positions == null || positions.size() == 0) {
            needGeneratePosition = true;
        } else {
            for (int pos : positions) {
                if (pos < 1 || pos > 7) {
                    throw new BaseException("SDB_INVALIDARG",
                            String.format("invalid position(%d) in the list", pos));
                }
                if (!validPositions.contains(pos)) {
                    validPositions.add(pos);
                }
            }
            if (validPositions.size() < 1 || validPositions.size() > 7) {
                throw new BaseException("SDB_INVALIDARG",
                        String.format("the number of valid position in the list is %d, it should be in [1, 7]",
                                validPositions.size()));
            }
        }
        // get information of nodes from catalog
        BSONObject groupInfoObj = sequoiadb.getDetailById(id);
        if (groupInfoObj == null) {
            throw new BaseException("SDB_CLS_GRP_NOT_EXIST",
                    String.format("no information of group id[%d]", id));
        }
        // check the nodes in current group
        Object nodesInfoArr = groupInfoObj.get(SequoiadbConstants.FIELD_NAME_GROUP);
        if (nodesInfoArr == null || !(nodesInfoArr instanceof BasicBSONList)) {
            throw new BaseException("SDB_SYS",
                    String.format("invalid content[%s] of field[%s]",
                            nodesInfoArr == null ? "null" : nodesInfoArr.toString(), SequoiadbConstants.FIELD_NAME_GROUP));
        }
        BasicBSONList nodesInfoList = (BasicBSONList) nodesInfoArr;
        if (nodesInfoList.isEmpty()) {
            throw new BaseException("SDB_CLS_EMPTY_GROUP");
        }
        // check whether there has primary or not
        Object primaryNodeId = groupInfoObj.get(SequoiadbConstants.FIELD_NAME_PRIMARY);
        boolean hasPrimary = true;
        if (primaryNodeId == null) {
            hasPrimary = false;
        } else if (!(primaryNodeId instanceof Number)) {
            throw new BaseException("SDB_SYS", "invalid primary node's information: " + primaryNodeId.toString());
        } else if (primaryNodeId.equals(Integer.valueOf(-1))) {
            hasPrimary = false;
        }
        // try to mark the position of primary node in the nodes list,
        // the value of position is [1, 7]
        int primaryNodePosition = 0;
        for (int i = 0; i < nodesInfoList.size(); i++) {
            BSONObject nodeInfo = (BSONObject) nodesInfoList.get(i);
            Object nodeIdValue = nodeInfo.get(SequoiadbConstants.FIELD_NAME_NODEID);
            if (nodeIdValue == null) {
                throw new BaseException("SDB_SYS", "node id can not be null");
            }
            if (hasPrimary && nodeIdValue.equals(primaryNodeId)) {
                primaryNodePosition = i + 1;
            }
        }
        if (hasPrimary && primaryNodePosition == 0) {
            throw new BaseException("SDB_SYS", "have no primary node in nodes list");
        }
        // try to generate positions
        int nodeCount = nodesInfoList.size();
        if (needGeneratePosition) {
            for (int i = 0; i < nodeCount; i++) {
                if (hasPrimary && primaryNodePosition == i + 1) {
                    continue;
                }
                validPositions.add(i + 1);
            }
        }
        // get a node position to create Node
        int nodeIndex = -1;
        BSONObject nodeInfoObj = null;
        // we must use "nodeCount" to compare first, since "validPositions" may be generate by us when
        // "needGeneratePosition" is true.
        if (nodeCount == 1) {
            nodeInfoObj = (BSONObject) nodesInfoList.get(0);
        } else if (validPositions.size() == 1) {
            // position is start from 1, so we need to decrease 1
            nodeIndex = (validPositions.get(0) - 1) % nodeCount;
            nodeInfoObj = (BSONObject) nodesInfoList.get(nodeIndex);
        } else {
            int position = 0;
            Random rand = new Random();
            int[] flags = new int[7];
            List<Integer> includePrimaryPositions = new ArrayList<Integer>();
            List<Integer> excludePrimaryPositions = new ArrayList<Integer>();
            for (int pos : validPositions) {
                if (pos <= nodeCount) {
                    nodeIndex = pos - 1;
                    if (flags[nodeIndex] == 0) {
                        flags[nodeIndex] = 1;
                        includePrimaryPositions.add(pos);
                        if (hasPrimary && primaryNodePosition != pos) {
                            excludePrimaryPositions.add(pos);
                        }
                    }
                } else {
                    nodeIndex = (pos - 1) % nodeCount;
                    if (flags[nodeIndex] == 0) {
                        flags[nodeIndex] = 1;
                        includePrimaryPositions.add(pos);
                        if (hasPrimary && primaryNodePosition != nodeIndex + 1) {
                            excludePrimaryPositions.add(pos);
                        }
                    }
                }
            }
            if (excludePrimaryPositions.size() > 0) {
                position = rand.nextInt(excludePrimaryPositions.size());
                position = excludePrimaryPositions.get(position);
            } else {
                position = rand.nextInt(includePrimaryPositions.size());
                position = includePrimaryPositions.get(position);
                if (needGeneratePosition) {
                    position += 1;
                }
            }
            nodeIndex = (position - 1) % nodeCount;
            nodeInfoObj = (BSONObject) nodesInfoList.get(nodeIndex);
        }
        int nodeId = Integer.parseInt(nodeInfoObj.get(SequoiadbConstants.FIELD_NAME_NODEID).toString());
        String hostName = nodeInfoObj.get(SequoiadbConstants.FIELD_NAME_HOST).toString();
        int port = getNodePort(nodeInfoObj);
        return new Node(hostName, port, nodeId, this);
    }

    /**
     * whether the specified node exists in current group or not
     *
     * @param nodeName the name of the node. e.g. "192.168.20.165:20000"
     * @return true or false
     */
    public boolean isNodeExist(String nodeName) {
        try {
            getNode(nodeName);
        } catch (BaseException e) {
            return false;
        }
        return true;
    }

    /**
     *whether the specified node exists in current group or not
     *
     * @param hostName
     * @param port
     * @return true or false
     */
    public boolean isNodeExist(String hostName, int port) {
        try {
            getNode(hostName, port);
        } catch (BaseException e) {
            return false;
        }
        return true;
    }

    /**
     * @param nodeName The name of the node
     * @return the specified node
     * @throws com.sequoiadb.exception.BaseException
     * @fn Node getNode(String nodeName)
     * @brief Get node by node's name (IP:PORT).
     */
    public Node getNode(String nodeName) throws BaseException {
        // check arguemnt
        if (nodeName == null || nodeName.isEmpty()) {
            throw new BaseException("SDB_INVALIDARG", nodeName);
        }
        // get node hostname and port
        String[] nodeMetaInfoArray = nodeName.split(":");
        if (nodeMetaInfoArray.length != 2) {
            throw new BaseException("SDB_INVALIDARG", nodeName);
        }
        String inputHostName = nodeMetaInfoArray[0];
        int inputPort = Integer.parseInt((nodeMetaInfoArray[1]));
        // get node object
        Node node = getNodeByMetaInfo(inputHostName, inputPort);
        if (node != null) {
            return node;
        } else {
            throw new BaseException("SDB_CLS_NODE_NOT_EXIST", nodeName);
        }
    }

    /**
     * @param hostName host name
     * @param port     port
     * @return the Node object
     * @throws com.sequoiadb.exception.BaseException
     * @fn Node getNode(String hostName, int port)
     * @brief Get node by hostName and port.
     */
    public Node getNode(String hostName, int port) throws BaseException {
        Node node = getNodeByMetaInfo(hostName, port);
        if (node != null) {
            return node;
        } else {
            throw new BaseException("SDB_CLS_NODE_NOT_EXIST", hostName + ":" + port);
        }
    }

    /**
     * @fn Node attachNode(String hostName, int port,
    BSONObject configure)
     * @brief Attach node.
     * @param hostName
     *          host name
     * @param port
     *          port
     * @param configure
     *          configuration for this operation
     * @return the attach Node object
     * @exception com.sequoiadb.exception.BaseException
     */
    public Node attachNode(String hostName, int port,
                           BSONObject configure) throws BaseException {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        config.put(SequoiadbConstants.FIELD_NAME_ONLY_ATTACH, true);
        if (configure != null) {
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_ONLY_ATTACH))
                    continue;

                config.put(key, configure.get(key));
            }
        }
        SDBMessage rtn = adminCommand(SequoiadbConstants.CREATE_CMD,
                SequoiadbConstants.NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, hostName, port, configure);
        }

        return getNode(hostName, port);
    }

    /**
     * @fn void detachNode(String hostName, int port,
    BSONObject configure)
     * @brief Detach node.
     * @param hostName
     *          host name
     * @param port
     *          port
     * @param configure
     *          configuration for this operation
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void detachNode(String hostName, int port,
                           BSONObject configure) throws BaseException {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        config.put(SequoiadbConstants.FIELD_NAME_ONLY_DETACH, true);
        if (configure != null) {
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_ONLY_DETACH))
                    continue;

                config.put(key, configure.get(key));
            }
        }
        SDBMessage rtn = adminCommand(SequoiadbConstants.REMOVE_CMD,
                SequoiadbConstants.NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, hostName, port, configure);
        }
    }

    /**
     * @fn Node createNode(String hostName, int port, String dbPath,
    Map<String, String> configure)
     * @brief Create node.
     * @param hostName
     * 			host name
     * @param port
     * 			port
     * @param dbPath
     * 			the path for node
     * @param configure
     * 			configuration for this operation
     * @return the created Node object
     * @exception com.sequoiadb.exception.BaseException
     * @deprecated we have override this api by passing a "BSONObject" instead of a "Map"
     */
    public Node createNode(String hostName, int port, String dbPath,
                           Map<String, String> configure) throws BaseException {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        config.put(SequoiadbConstants.PMD_OPTION_DBPATH, dbPath);
        if (configure != null && !configure.isEmpty())
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME))
                    continue;
                config.put(key, configure.get(key));
            }
        SDBMessage rtn = adminCommand(SequoiadbConstants.CREATE_CMD,
                SequoiadbConstants.NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, hostName, port, dbPath, configure);
        }
        return getNode(hostName, port);
    }

    /**
     * @fn Node createNode(String hostName, int port, String dbPath, BSONObject configure)
     * @brief Create node.
     * @param hostName
     *          host name
     * @param port
     *          port
     * @param dbPath
     *          the path for node
     * @param configure
     *          configuration for this operation
     * @return the created Node object
     * @exception com.sequoiadb.exception.BaseException
     */
    public Node createNode(String hostName, int port, String dbPath,
                           BSONObject configure) throws BaseException {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        config.put(SequoiadbConstants.PMD_OPTION_DBPATH, dbPath);
        if (configure != null && !configure.isEmpty())
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME))
                    continue;
                config.put(key, configure.get(key));
            }
        SDBMessage rtn = adminCommand(SequoiadbConstants.CREATE_CMD,
                SequoiadbConstants.NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, hostName, port, dbPath, configure);
        }
        return getNode(hostName, port);
    }

    /**
     * @fn Node createNode(String hostName, int port, String dbPath)
     * @brief Create node.
     * @param hostName  host name
     * @param port      port
     * @param dbPath    the path for node
     * @return the created Node object
     * @throws BaseException If error happens.
     */
    public Node createNode(String hostName, int port, String dbPath) throws BaseException {
        return createNode(hostName, port, dbPath, new BasicBSONObject());
    }

    /**
     * @fn void removeNode(String hostName, int port,
    BSONObject configure)
     * @brief Remove node.
     * @param hostName
     * 			host name
     * @param port
     * 			port
     * @param configure
     * 			configuration for this operation
     * @exception com.sequoiadb.exception.BaseException
     */
    public void removeNode(String hostName, int port,
                           BSONObject configure) throws BaseException {
        BSONObject config = new BasicBSONObject();
        config.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, name);
        config.put(SequoiadbConstants.FIELD_NAME_HOST, hostName);
        config.put(SequoiadbConstants.PMD_OPTION_SVCNAME,
                Integer.toString(port));
        if (configure != null)
            for (String key : configure.keySet()) {
                if (key.equals(SequoiadbConstants.FIELD_NAME_GROUPNAME)
                        || key.equals(SequoiadbConstants.FIELD_NAME_HOST)
                        || key.equals(SequoiadbConstants.PMD_OPTION_SVCNAME))
                    continue;
                config.put(key, configure.get(key));
            }
        SDBMessage rtn = adminCommand(SequoiadbConstants.REMOVE_CMD,
                SequoiadbConstants.NODE, config);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, port, configure);
        }
    }

    /**
     * @fn void start()
     * @brief Start current replica group.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void start() throws BaseException {
        BSONObject groupName = new BasicBSONObject();
        groupName.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, this.name);
        SDBMessage rtn = adminCommand(SequoiadbConstants.ACTIVE_CMD,
                SequoiadbConstants.GROUP, groupName);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, this.name);
        }
    }

    /**
     * @fn void stop()
     * @brief Stop current replica group.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void stop() throws BaseException {
        BSONObject groupName = new BasicBSONObject();
        groupName.put(SequoiadbConstants.FIELD_NAME_GROUPNAME, this.name);
        SDBMessage rtn = adminCommand(SequoiadbConstants.SHUTDOWN_CMD,
                SequoiadbConstants.GROUP, groupName);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags, this.name);
        }
    }

    /**
     * @fn boolean isCatalog()
     * @brief Judge whether current replicaGroup is catalog replica group or not.
     * @return true is while false is not
     */
    public boolean isCatalog() {
        return isCataRG;
    }

    private Node getNodeByMetaInfo(final String inputHostName, final int inputPort) {
        // check
        if (inputHostName == null || inputHostName.isEmpty() || inputPort < 0 || inputPort > 65535) {
            throw new BaseException("SDB_INVALIDARG",
                    String.format("invalid node info[%s:%d]", inputHostName, inputPort));
        }
        // get group info from catalog
        BSONObject groupInfoObj = sequoiadb.getDetailById(id);
        if (groupInfoObj == null) {
            throw new BaseException("SDB_CLS_GRP_NOT_EXIST",
                    String.format("no information of group id[%d]", id));
        }
        // extract nodes info
        Object nodesInfoArr = groupInfoObj.get(SequoiadbConstants.FIELD_NAME_GROUP);
        if (nodesInfoArr == null || !(nodesInfoArr instanceof BasicBSONList)) {
            throw new BaseException("SDB_SYS",
                    String.format("invalid content[%s] of field[%s]",
                            nodesInfoArr == null ? "null" : nodesInfoArr.toString(), SequoiadbConstants.FIELD_NAME_GROUP));
        }
        BasicBSONList nodesInfoList = (BasicBSONList) nodesInfoArr;
        if (nodesInfoList.size() == 0) {
            return null;
        }
        // try to build node object
        Object nodeIdFromCatalog = null;
        String hostNameFromCatalog = null;
        int portFromCatalog = -1;
        for (Object nodeInfoObj : nodesInfoList) {
            BSONObject nodeInfo = (BSONObject) nodeInfoObj;
            nodeIdFromCatalog = nodeInfo.get(SequoiadbConstants.FIELD_NAME_NODEID);
            hostNameFromCatalog = (String) nodeInfo.get(SequoiadbConstants.FIELD_NAME_HOST);
            portFromCatalog = getNodePort(nodeInfo);
            if (nodeIdFromCatalog == null || hostNameFromCatalog == null) {
                throw new BaseException("SDB_SYS", "invalid node's information");
            }
            // compare
            if (hostNameFromCatalog.equals(inputHostName) && portFromCatalog == inputPort) {
                return new Node(inputHostName, inputPort,
                        Integer.parseInt(nodeIdFromCatalog.toString()), this);
            }
        }
        return null;
    }

    private int getNodePort(BSONObject node) {
        if (node == null) {
            throw new BaseException("SDB_SYS", "invalid information of node");
        }
        Object services = node.get(SequoiadbConstants.FIELD_NAME_GROUPSERVICE);
        if (services == null)
            throw new BaseException("SDB_SYS", node);
        BasicBSONList serviceInfos = (BasicBSONList) services;
        if (serviceInfos.size() == 0)
            throw new BaseException("SDB_CLS_NODE_NOT_EXIST");
        int port = -1;
        for (Object obj : serviceInfos) {
            BSONObject service = (BSONObject) obj;
            if (service.get(SequoiadbConstants.FIELD_NAME_SERVICETYPE)
                    .toString().equals("0")) {
                port = Integer.parseInt(service.get(
                        SequoiadbConstants.FIELD_NAME_SERVICENAME).toString());
                break;
            }
        }
        if (port == -1)
            throw new BaseException("SDB_SYS", node);
        return port;
    }

    private SDBMessage adminCommand(String cmdType, String contextType,
                                    BSONObject query) throws BaseException {
        IConnection connection = sequoiadb.getConnection();
        // Admin command request
        // long reqId = 0;
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage sdbMessage = new SDBMessage();
        String commandString = SequoiadbConstants.ADMIN_PROMPT + cmdType + " "
                + contextType;
        if (query != null)
            sdbMessage.setMatcher(query);
        else
            sdbMessage.setMatcher(dummyObj);
        sdbMessage.setCollectionFullName(commandString);
        sdbMessage.setFlags(0);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        // sdbMessage.setResponseTo(reqId);
        // reqId++;
        sdbMessage.setRequestID(sequoiadb.getNextRequstID());
        sdbMessage.setSkipRowsCount(-1);
        sdbMessage.setReturnRowsCount(-1);
        sdbMessage.setSelector(dummyObj);
        sdbMessage.setOrderBy(dummyObj);
        sdbMessage.setHint(dummyObj);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage, sequoiadb.endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(sequoiadb.endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);
        return rtnSDBMessage;
    }
}
