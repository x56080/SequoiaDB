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
import com.sequoiadb.net.IConnection;
import com.sequoiadb.util.SDBMessageHelper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * @class DBCursor
 * @brief Database operation interfaces of cursor.
 */
public class DBCursor {
    private long reqId;
    private SDBMessage sdbMessage;
    private DBCollection dbc;
    private IConnection connection;
    private BSONObject current = null;
    private List<BSONObject> list;

    private byte[] currentRaw;
    private List<byte[]> listRaw;
    private int index;
    private boolean hasMore;
    private byte times;
    private long contextId;
    boolean endianConvert;
    private Sequoiadb sequoiadb;

    DBCursor() {
        hasMore = false;
        sdbMessage = null;
        connection = null;
        dbc = null;
        list = null;
        listRaw = null;
        index = -1;
        reqId = 0;
        contextId = -1;
        endianConvert = false;
        sequoiadb = null;
    }

    DBCursor(SDBMessage rtnSDBMessage, DBCollection dbc) {
        this.dbc = dbc;
        sequoiadb = dbc.getSequoiadb();
        endianConvert = sequoiadb.endianConvert;
        connection = dbc.getConnection();
        sdbMessage = rtnSDBMessage;
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        sdbMessage.setNumReturned(-1); // return data count
        list = new ArrayList<BSONObject>();
        listRaw = new ArrayList<byte[]>();
        reqId = rtnSDBMessage.getRequestID();
        contextId = rtnSDBMessage.getContextIDList().get(0);
        hasMore = false;
        current = null;
        times = 0;
        index = -1;

        List<BSONObject> tmpList = sdbMessage.getObjectList();
        if (null != tmpList && tmpList.size() != 0) {
            list = tmpList;
        }
    }

    DBCursor(SDBMessage rtnSDBMessage, Sequoiadb sdb) {
        this.connection = sdb.getConnection();
        sequoiadb = sdb;
        sdbMessage = rtnSDBMessage;
        sdbMessage.setNodeID(SequoiadbConstants.ZERO_NODEID);
        sdbMessage.setNumReturned(-1); // return data count
        list = new ArrayList<BSONObject>();
        listRaw = new ArrayList<byte[]>();
        reqId = rtnSDBMessage.getRequestID();
        contextId = rtnSDBMessage.getContextIDList().get(0);
        hasMore = false;
        current = null;
        times = 0;
        index = -1;
        endianConvert = sdb.endianConvert;

        List<BSONObject> tmpList = sdbMessage.getObjectList();
        if (null != tmpList && tmpList.size() != 0) {
            list = tmpList;
        }
    }

    /**
     * @return true for next data exists while false for not
     * @throws com.sequoiadb.exception.BaseException
     * @fn boolean hasNext()
     * @brief Judge whether the next document exists or not.
     */
    public boolean hasNext() throws BaseException {
        if (connection == null)
            return hasMore;
        if (times > 0 || sdbMessage == null)
            return hasMore;
        if (list == null || index >= (list.size() - 1)) {
            getListFromDB(true);
            index = -1;
        }
        if (list == null || list.size() == 0) {
            hasMore = false;
        } else {
            hasMore = true;
        }
        ++times;
        return hasMore;
    }

    /*
     * @fn boolean hasNextRaw()
     * @brief Judge whether next raw data exists.
     * @return true for next raw data exists while false for not
     * @exception com.sequoiadb.exception.BaseException
     */
    public boolean hasNextRaw() throws BaseException {
        if (connection == null)
            return hasMore;
        if (times > 0 || sdbMessage == null)
            return hasMore;
        if (listRaw == null || index >= (listRaw.size() - 1)) {
            getListFromDB(false);
            index = -1;
        }
        if (listRaw == null || listRaw.size() == 0) {
            hasMore = false;
        } else {
            hasMore = true;
        }
        ++times;
        return hasMore;
    }

    /**
     * @return the next date or null if the cursor is empty
     * or the cursor is closed
     * @throws com.sequoiadb.exception.BaseException
     * @fn BSONObject getNext()
     * @brief Get next document.
     * @note calling this function after the cursor have been closed
     * will throw BaseException "SDB_RTN_CONTEXT_NOTEXIST"
     */
    public BSONObject getNext() throws BaseException {
        if (connection == null)
            throw new BaseException(SDBError.SDB_RTN_CONTEXT_NOTEXIST, "connection is null");
        if (times == 0)
            hasNext();
        if (hasMore) {
            ++index;
            current = list.get(index);
            times = 0;
            return current;
        }
        return null;
    }

    /**
     * @return a byte array of raw date of next record or null
     * if the cursor is empty
     * @throws com.sequoiadb.exception.BaseException
     * @fn byte[] getNextRaw()
     * @brief Get raw date of next record.
     * @note calling this function after the cursor have been closed
     * will throw BaseException "SDB_RTN_CONTEXT_NOTEXIST"
     */
    public byte[] getNextRaw() throws BaseException {
        if (connection == null)
            throw new BaseException(SDBError.SDB_RTN_CONTEXT_NOTEXIST, "connection is null");
        if (times == 0)
            hasNextRaw();
        if (hasMore) {
            ++index;
            currentRaw = listRaw.get(index);
            times = 0;
            return currentRaw;
        }
        return null;
    }

    /**
     * @return the current date or null if the cursor is empty
     * @throws com.sequoiadb.exception.BaseException
     * @fn BSONObject getCurrent()
     * @brief Get current document.
     * @note calling this function after the cursor have been closed
     * will throw BaseException "SDB_RTN_CONTEXT_NOTEXIST"
     */
    public BSONObject getCurrent() throws BaseException {
        if (connection == null)
            throw new BaseException(SDBError.SDB_RTN_CONTEXT_NOTEXIST);
        // in case the first time we get date
        if (index == -1)
            return getNext();
        else
            return list.get(index);
    }

    /**
     * @fn void updateCurrent(BSONObject modifier, BSONObject hint)
     * @brief update current document.
     * @param modifier
     *            the modify rule
     * @param hint
     *            update by hint
     * @exception com.sequoiadb.exception.BaseException
     */
    /*
	public void updateCurrent(BSONObject modifier, BSONObject hint)
			throws BaseException {
		if (dbc != null && current != null) {
			BSONObject query = new BasicBSONObject();
			query.put("_id", (ObjectId) current.get("_id"));
			dbc.update(query, modifier, hint);
			DBCursor cursor = dbc.query(query, null, null, hint);
			current = cursor.getNext();
		}
	}*/

	/*
	 * @fn void deleteCurrent()
	 * @brief delete current data in DB
	 * @exception com.sequoiadb.exception.BaseException
	 */
	/*
	public void deleteCurrent() throws BaseException {
		if (dbc != null && current != null) {
			BSONObject query = new BasicBSONObject();
			query.put("_id", (ObjectId) current.get("_id"));
			dbc.delete(query);
		}
		current = null;
	}
	*/

    /**
     * @return void
     * @throws com.sequoiadb.exception.BaseException
     * @fn void close()
     * @brief Close the cursor.
     */
    public void close() throws BaseException {
        killCursor();
        sdbMessage = null;
        dbc = null;
        hasMore = false;
        current = null;
        list = null;
        listRaw = null;
    }

    private void getListFromDB(boolean decode) {
        if (connection == null)
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);

        if (contextId == -1) {
            hasMore = false;
            index = -1;
            current = null;
            list = null;
            listRaw = null;
            return;
        }

        if (decode) {
            list.clear();
        } else {
            listRaw.clear();
        }

        sdbMessage.setRequestID(reqId);
        sdbMessage.setOperationCode(Operation.OP_GETMORE);
        byte[] request = SDBMessageHelper.buildGetMoreRequest(sdbMessage,
                endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtnSDBMessage = null;

        if (decode) {
            rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        } else {
            rtnSDBMessage = SDBMessageHelper.msgExtractReplyRaw(byteBuffer);
        }

        if (rtnSDBMessage.getOperationCode() != Operation.OP_GETMORE_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    rtnSDBMessage.getOperationCode().toString());
        }

        int flags = rtnSDBMessage.getFlags();
        if (flags == SequoiadbConstants.SDB_DMS_EOC // in case end of collection or wrong contextId
                || contextId != rtnSDBMessage.getContextIDList().get(0)) {
            hasMore = false;
            index = -1;
            current = null;
            list = null;
            listRaw = null;
        } else if (flags != 0) { // in case one of the other errors happen
            throw new BaseException(flags);
        } else { // in case nornal, get the data
            reqId = rtnSDBMessage.getRequestID();
            if (decode) {
                list = rtnSDBMessage.getObjectList();
            } else {
                listRaw = rtnSDBMessage.getObjectListRaw();
            }
        }
    }

    private void killCursor() {
        //  connection == null means the cursor has been close
        if (connection == null) {
            return;
        }
        if (contextId == -1) {
            connection = null;
            return;
        }
        long[] contextIds = new long[]{contextId};
        byte[] request = SDBMessageHelper.buildKillCursorMsg(
                sequoiadb.getNextRequstID(), contextIds, endianConvert);
        connection.sendMessage(request);

        ByteBuffer byteBuffer = connection.receiveMessage(endianConvert);
        SDBMessage rtnSDBMessage = SDBMessageHelper.msgExtractReply(byteBuffer);
        assert (rtnSDBMessage.getOperationCode() == Operation.OP_KILL_CONTEXT_RES);
        assert (rtnSDBMessage.getFlags() == 0);

        connection = null;
        contextId = -1;
    }
}
