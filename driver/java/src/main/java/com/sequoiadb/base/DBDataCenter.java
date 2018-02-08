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
 * @author YouBin Lin
 */
/**
 * @package com.sequoiadb.base;
 * @brief SequoiaDB Driver for Java
 * @author YouBin Lin
 */

package com.sequoiadb.base;

import com.sequoiadb.base.SequoiadbConstants.Operation;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.net.IConnection;
import com.sequoiadb.util.SDBMessageHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import java.nio.ByteBuffer;

/**
 * @class DBDataCenter
 * @brief Operation interfaces of DBDataCenter.
 */
public interface DBDataCenter {
    /**
     * @fn String getName()
     * @brief get the DataCenter's Name
     * @return the Name of DataCenter
     */
    public String getName();

    /**
     * @fn BSONObject getDetail()
     * @brief get the detail of DataCenter
     * @return the detail of DataCenter
     * @exception com.sequoiadb.exception.BaseException
     */
    public BSONObject getDetail();

    /**
     * @fn void activate()
     * @brief activate the DataCenter
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void activate();

    /**
     * @fn void deactivate()
     * @brief deactivate the DataCenter
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void deactivate();

    /**
     * @fn void disableReadonly()
     * @brief disable the DataCenter's read only mode.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void disableReadonly();

    /**
     * @fn void enableReadonly()
     * @brief enable the DataCenter's read only mode.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void enableReadonly();

    /**
     * @fn void createImage(String cataAddrList)
     * @brief create image
     * @param       cataAddrList Catalog address list of remote data center.
     *              e.g. "192.168.20.165:30003"
     *              e.g. "192.168.20.165:30003,192.168.20.166:30003" 
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void createImage(String cataAddrList);

    /**
     * @fn void removeImage()
     * @brief remove image
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void removeImage();

    /**
     * @fn void enableImage()
     * @brief enable image
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void enableImage();

    /**
     * @fn void disableImage()
     * @brief disable image
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void disableImage();

    /**
     * @fn void attachGroups(BSONObject groupInfo)
     * @brief attach specified groups to data center
     * @param       groupInfo The information of groups to attach, 
     *              e.g. {Groups:[["a", "a"], ["b", "b"]]}
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void attachGroups(BSONObject groupInfo);

    /**
     * @fn void detachGroups(BSONObject groupInfo)
     * @brief detach specified groups from data center
     * @param       groupInfo The information of groups to detach, 
     *              e.g. {Groups:[["a", "a"], ["b", "b"]]}
     *              if groupInfo is empty, that suggest to detach all groups
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     */
    public void detachGroups(BSONObject groupInfo);
}

class DBDataCenterConcrete implements DBDataCenter {
    private String _name;
    private Sequoiadb _sdb;
    private IConnection _connection;

    public DBDataCenterConcrete(Sequoiadb sdb) {
        _sdb = sdb;
        _connection = _sdb.getConnection();
        BSONObject obj = getDetail();
        BSONObject subObj = (BSONObject) obj.get(
                SequoiadbConstants.FIELD_NAME_DATACENTER);
        String clusterName = (String) subObj.get(
                SequoiadbConstants.FIELD_NAME_CLUSTERNAME);
        String businessName = (String) subObj.get(
                SequoiadbConstants.FIELD_NAME_BUSINESSNAME);
        _name = clusterName + ":" + businessName;
    }

    @Override
    public String getName() {
        return _name;
    }

    @Override
    public BSONObject getDetail() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_GET_DCINFO;
        BSONObject dummyObj = new BasicBSONObject();
        SDBMessage rtn = adminCommand(commandString, dummyObj, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            if (flags == SequoiadbConstants.SDB_DMS_EOC) {
                throw new BaseException(SDBError.SDB_SYS);
            } else {
                throw new BaseException(flags);
            }
        }

        DBCursor cursor = new DBCursor(rtn, _sdb);
        if (!cursor.hasNext()) {
            throw new BaseException(SDBError.SDB_DMS_EOC);
        }

        BSONObject obj;
        try {
            obj = cursor.getNext();
        } finally {
            cursor.close();
        }
        return obj;
    }

    @Override
    public void activate() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();
        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_ACTIVATE);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void deactivate() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();
        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_DEACTIVATE);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void disableReadonly() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();
        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_DISABLE_READONLY);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void enableReadonly() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();
        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_ENABLE_READONLY);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void createImage(String cataAddrList) {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject address = new BasicBSONObject();
        address.put(SequoiadbConstants.FIELD_NAME_ADDRESS, cataAddrList);

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_CREATE);
        query.put(SequoiadbConstants.FIELD_NAME_OPTIONS, address);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void removeImage() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_REMOVE);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void enableImage() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_ENABLE);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void disableImage() {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_DISABLE);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void attachGroups(BSONObject groupInfo) {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_ATTACH);
        query.put(SequoiadbConstants.FIELD_NAME_OPTIONS, groupInfo);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    @Override
    public void detachGroups(BSONObject groupInfo) {
        String commandString = SequoiadbConstants.ADMIN_PROMPT
                + SequoiadbConstants.CMD_NAME_ALTER_DC;
        BSONObject dummyObj = new BasicBSONObject();

        BSONObject query = new BasicBSONObject();
        query.put(SequoiadbConstants.FIELD_NAME_ACTION,
                SequoiadbConstants.CMD_VALUE_NAME_DETACH);
        query.put(SequoiadbConstants.FIELD_NAME_OPTIONS, groupInfo);
        SDBMessage rtn = adminCommand(commandString, query, dummyObj, dummyObj,
                dummyObj, -1, -1, 0);
        int flags = rtn.getFlags();
        if (flags != 0) {
            throw new BaseException(flags);
        }
    }

    private SDBMessage adminCommand(String commandString, BSONObject query,
                                    BSONObject selector, BSONObject orderBy, BSONObject hint,
                                    long skipRows, long returnRows, int flag) throws BaseException {
        SDBMessage sdbMessage = new SDBMessage();
        BSONObject dummy = new BasicBSONObject();
        if (query == null)
            query = dummy;
        if (selector == null)
            selector = dummy;
        if (orderBy == null)
            orderBy = dummy;
        if (hint == null)
            hint = dummy;

        sdbMessage.setCollectionFullName(commandString);

        sdbMessage.setVersion(1);
        sdbMessage.setW((short) 0);
        sdbMessage.setPadding((short) 0);
        sdbMessage.setFlags(flag);
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        sdbMessage.setRequestID(_sdb.getNextRequstID());
        sdbMessage.setSkipRowsCount(skipRows);
        sdbMessage.setReturnRowsCount(returnRows);
        sdbMessage.setMatcher(query);
        sdbMessage.setSelector(selector);
        sdbMessage.setOrderBy(orderBy);
        sdbMessage.setHint(hint);
        sdbMessage.setOperationCode(Operation.OP_QUERY);

        byte[] request = SDBMessageHelper.buildQueryRequest(sdbMessage,
                _sdb.endianConvert);
        _sdb.getConnection().sendMessage(request);

        ByteBuffer byteBuffer = _connection.receiveMessage(_sdb.endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        SDBMessageHelper.checkMessage(sdbMessage, rtnSDBMessage);

        return rtnSDBMessage;
    }
}
