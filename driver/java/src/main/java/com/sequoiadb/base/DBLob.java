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

package com.sequoiadb.base;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import com.sequoiadb.base.SequoiadbConstants.Operation;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.util.Helper;
import com.sequoiadb.util.SDBMessageHelper;

/**
 * @class DBLob
 * @brief Operation interfaces of DBLob.
 */
public interface DBLob {
    /**
     * @memberof SDB_LOB_SEEK_SET 0
     * @brief Change the position from the beginning of lob
     */
    public final static int SDB_LOB_SEEK_SET = 0;

    /**
     * @memberof SDB_LOB_SEEK_CUR 1
     * @brief Change the position from the current position of lob
     */
    public final static int SDB_LOB_SEEK_CUR = 1;

    /**
     * @memberof SDB_LOB_SEEK_END 2
     * @brief Change the position from the end of lob
     */
    public final static int SDB_LOB_SEEK_END = 2;

    /**
     * @return the lob's id
     * @fn ObjectId getID()
     * @brief get the lob's id
     */
    public ObjectId getID();

    /**
     * @return the lob's size
     * @fn long getSize()
     * @brief get the size of lob
     */
    public long getSize();

    /**
     * @return the lob's create time
     * @fn long getCreateTime()
     * @brief get the create time of lob
     */
    public long getCreateTime();

    /**
     * @param b the data.
     * @throws com.sequoiadb.exception.BaseException
     * @fn write(byte[] b)
     * @brief Writes <code>b.length</code> bytes from the specified
     * byte array to this lob.
     */
    public void write(byte[] b) throws BaseException;

    /**
     * @param b   the data.
     * @param off the start offset in the data.
     * @param len the number of bytes to write.
     * @throws com.sequoiadb.exception.BaseException
     * @fn write(byte[] b, int off, int len)
     * @brief Writes <code>len</code> bytes from the specified
     * byte array starting at offset <code>off</code> to this lob.
     */
    public void write(byte[] b, int off, int len) throws BaseException;

    /**
     * @param b the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     * there is no more data because the end of the file has been
     * reached, or <code>0</code> if <code>b.length</code> is Zero.
     * @throws com.sequoiadb.exception.BaseException
     * @fn int read( byte[] b )
     * @brief Reads up to <code>b.length</code> bytes of data from this lob into
     * an array of bytes.
     */
    public int read(byte[] b) throws BaseException;

    /**
     * @param b   the buffer into which the data is read.
     * @param off the start offset in the destination array <code>b</code>.
     * @param len the maximum number of bytes read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     * there is no more data because the end of the file has been
     * reached, or <code>0</code> if <code>len</code> is Zero.
     * @throws com.sequoiadb.exception.BaseException
     * @fn int read( byte[] b, int off, int len )
     * @brief Reads up to <code>len</code> bytes of data from this lob into
     * an array of bytes.
     */
    public int read(byte[] b, int off, int len) throws BaseException;

    /**
     * @param size     the adding size.
     * @param seekType SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @throws com.sequoiadb.exception.BaseException.
     * @fn seek(long size, int seekType)
     * @brief change the read position of the lob. The new position is
     * obtained by adding size to the position specified by
     * seekType. If seekType is set to SDB_LOB_SEEK_SET,
     * SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, the offset is
     * relative to the start of the lob, the current position
     * of lob, or the end of lob.
     */
    public void seek(long size, int seekType) throws BaseException;

    /**
     * @param null
     * @throws com.sequoiadb.exception.BaseException
     * @fn close()
     * @brief close the lob
     */
    public void close() throws BaseException;
}

class DBLobConcrete implements DBLob {
    public final static int SDB_LOB_CREATEONLY = 0x00000001;
    public final static int SDB_LOB_READ = 0x00000004;

    // the max lob data size to send for one message
    private final static int SDB_LOB_MAX_DATA_LENGTH = 1024 * 1024;

    private final static long SDB_LOB_DEFAULT_OFFSET = -1;
    private final static int SDB_LOB_DEFAULT_SEQ = 0;

    private final static int SDB_LOB_ALIGNED_LEN = 524288; // 512k
    private final static int FLG_LOBOPEN_WITH_RETURNDATA = 0X00000002;

    private DBCollection _cl;
    private ObjectId _id;
    private int _mode;
    private int _pageSize;
    private long _size;
    private long _createTime;
    private long _readOffset = 0;
    private long _cachedOffset = -1;
    private ByteBuffer _cachedDataBuff = null;
    private boolean _isOpen = false;
    private boolean _endianConvert;

    /* when first open/create DBLob, sequoiadb return the contextID for the
     * further reading/writing/close
    */
    private long _contextID;

    /**
     * @param cl The instance of DBCollection
     * @throws com.sequoiadb.exception.BaseException
     * @fn DBLob(DBCollection cl)
     * @brief Constructor
     */
    public DBLobConcrete(DBCollection cl) throws BaseException {
        if (cl == null) {
            throw new BaseException("SDB_INVALIDARG", "cl is null");
        }

        _cl = cl;
        _endianConvert = cl.getSequoiadb().endianConvert;
    }

    /**
     * @throws com.sequoiadb.exception.BaseException.
     * @fn open()
     * @brief create a lob, lob's id will auto generate in this function
     */
    public void open() {
        open(null, SDB_LOB_CREATEONLY);
    }

    /**
     * @param id the lob's id
     * @throws com.sequoiadb.exception.BaseException.
     * @fn open(ObjectId id)
     * @brief open an exist lob with id
     */
    public void open(ObjectId id) {
        open(id, SDB_LOB_READ);
    }

    /**
     * @param id   the lob's id
     * @param mode available mode is SDB_LOB_CREATEONLY or SDB_LOB_READ.
     *             SDB_LOB_CREATEONLY
     *             create a new lob with given id, if id is null, it will
     *             be generated in this function;
     *             SDB_LOB_READ
     *             read an exist lob
     * @throws com.sequoiadb.exception.BaseException.
     * @fn open(ObjectId id, int mode)
     * @brief open an exist lob, or create a lob
     */
    public void open(ObjectId id, int mode) throws BaseException {
        if (_isOpen) {
            throw new BaseException("SDB_INVALIDARG", "lob have opened:id="
                    + _id);
        }

        if (SDB_LOB_CREATEONLY != mode && SDB_LOB_READ != mode) {
            throw new BaseException("SDB_INVALIDARG", "mode is unsupported:"
                    + mode);
        }

        if (SDB_LOB_READ == mode) {
            if (null == id) {
                throw new BaseException("SDB_INVALIDARG", "id must be specify"
                        + " in mode:" + mode);
            }
        }

        _id = id;
        if (SDB_LOB_CREATEONLY == mode) {
            if (null == _id) {
                _id = ObjectId.get();
            }
        }

        _mode = mode;
        _readOffset = 0;

        _open();
        _isOpen = true;
    }

    private void _open() throws BaseException {
        BSONObject openLob = new BasicBSONObject();
        openLob.put(SequoiadbConstants.FIELD_COLLECTION, _cl.getFullName());
        openLob.put(SequoiadbConstants.FIELD_NAME_LOB_OID, _id);
        openLob.put(SequoiadbConstants.FIELD_NAME_LOB_OPEN_MODE, _mode);

        int flags = (_mode == SDB_LOB_READ) ? FLG_LOBOPEN_WITH_RETURNDATA :
                SequoiadbConstants.DEFAULT_FLAGS;

        byte[] request = generateOpenLobRequest(openLob, flags);
        ByteBuffer res = sendRequest(request, request.length);

        SDBMessage resMessage = SDBMessageHelper.msgExtractLobOpenReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_OPEN_RES) {
            throw new BaseException("SDB_UNKNOWN_MESSAGE",
                    resMessage.getOperationCode());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag, openLob);
        }
        List<BSONObject> objList = resMessage.getObjectList();
        if (objList.size() != 1) {
            throw new BaseException("SDB_NET_BROKEN_MSG",
                    "objList.size()=" + objList.size());
        }

        BSONObject obj = objList.get(0);
        _size = (Long) obj.get(SequoiadbConstants.FIELD_NAME_LOB_SIZE);
        _createTime = (Long) obj.get(
                SequoiadbConstants.FIELD_NAME_LOB_CREATTIME);
        _pageSize = (Integer) obj.get(
                SequoiadbConstants.FIELD_NAME_LOB_PAGESIZE);
        _cachedDataBuff = resMessage.getLobCachedDataBuf();
        if (_cachedDataBuff != null) {
            _readOffset = 0;
            _cachedOffset = _readOffset;
        }
        resMessage.setLobCachedDataBuf(null);
        _contextID = resMessage.getContextIDList().get(0);
    }

    private ByteBuffer sendRequest(byte[] request, int length)
            throws BaseException {
        if (request == null) {
            throw new BaseException("SDB_INVALIDARG", "request can't be null");
        }

        _cl.getConnection().sendMessage(request, length);
        return _cl.getConnection().receiveMessage(_endianConvert);
    }

    /**
     * @return the lob's id
     * @fn getID()
     * @brief get the lob's id
     */
    public ObjectId getID() {
        return _id;
    }

    /**
     * @return the lob's size
     * @fn getSize()
     * @brief get the size of lob
     */
    public long getSize() {
        return _size;
    }

    /**
     * @return the lob's create time
     * @fn getCreateTime()
     * @brief get the create time of lob
     */
    public long getCreateTime() {
        return _createTime;
    }

    /**
     * @throws com.sequoiadb.exception.BaseException
     * @fn close()
     * @brief close the lob
     */
    public void close() throws BaseException {
        if (!_isOpen) {
            return;
        }

        byte[] request = generateCloseLobRequest();
        ByteBuffer res = sendRequest(request, request.length);

        SDBMessage resMessage = SDBMessageHelper.msgExtractReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_CLOSE_RES) {
            throw new BaseException("SDB_UNKNOWN_MESSAGE",
                    resMessage.getOperationCode());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag);
        }

        _isOpen = false;
    }

    /**
     * @param b the data.
     * @throws com.sequoiadb.exception.BaseException.
     * @fn write(byte[] b)
     * @brief Writes <code>b.length</code> bytes from the specified
     * byte array to this lob.
     */
    public void write(byte[] b) throws BaseException {
        write(b, 0, b.length);
    }

    /**
     * @param b   the data.
     * @param off the start offset in the data.
     * @param len the number of bytes to write.
     * @throws com.sequoiadb.exception.BaseException
     * @fn write(byte[] b, int off, int len)
     * @brief Writes <code>len</code> bytes from the specified
     * byte array starting at offset <code>off</code> to this lob.
     */
    public void write(byte[] b, int off, int len) throws BaseException {
        if (!_isOpen) {
            throw new BaseException("SDB_LOB_NOT_OPEN", "lob is not open");
        }

        if (b == null) {
            throw new BaseException("SDB_INVALIDARG", "input is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException("SDB_INVALIDARG", "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException("SDB_INVALIDARG", "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException("SDB_INVALIDARG", "off + len is great than b.length");
        }

        int offset = off;
        int leftLen = len;
        ByteBuffer byteBuf = ByteBuffer.wrap(b, off, len);
        while (leftLen > 0) {
            /* if b.length is more then SDB_LOB_MAX_DATA_LENGTH. we will split 
             * the data to pieces with length=SDB_LOB_MAX_DATA_LENGTH. 
             */
            int writeLen = leftLen > SDB_LOB_MAX_DATA_LENGTH ?
                    SDB_LOB_MAX_DATA_LENGTH : leftLen;
            // set the correct position for next reading
            byteBuf.position(offset);
            byteBuf.limit(offset + writeLen);
            // TODO: we should avoid copy here
            byte[] tmpBuf = new byte[writeLen];
            byteBuf.get(tmpBuf);
            _write(tmpBuf);
            offset += writeLen;
            leftLen -= writeLen;
        }
    }

    /**
     * @param b the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or
     * <code>-1</code> if there is no more data because the end of
     * the file has been reached, or <code>0<code> if
     * <code>b.length</code> is Zero.
     * @throws com.sequoiadb.exception.BaseException.
     * @fn read(byte[] b)
     * @brief Reads up to <code>b.length</code> bytes of data from this
     * lob into an array of bytes.
     */
    public int read(byte[] b) throws BaseException {
        return read(b, 0, b.length);
    }

    /**
     * @param b   the buffer into which the data is read.
     * @param off the start offset in the destination array <code>b</code>.
     * @param len the maximum number of bytes read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     * there is no more data because the end of the file has been
     * reached, or <code>0</code> if <code>len</code> is Zero.
     * @throws com.sequoiadb.exception.BaseException
     * @fn int read( byte[] b, int off, int len )
     * @brief Reads up to <code>len</code> bytes of data from this lob into
     * an array of bytes.
     */
    public int read(byte[] b, int off, int len) throws BaseException {
        if (!_isOpen) {
            throw new BaseException("SDB_LOB_NOT_OPEN", "lob is not open");
        }

        if (b == null) {
            throw new BaseException("SDB_INVALIDARG", "b is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException("SDB_INVALIDARG", "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException("SDB_INVALIDARG", "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException("SDB_INVALIDARG", "off + len is great than b.length");
        }

        if (b.length == 0) {
            return 0;
        }

        return _read(b, off, len);
    }

    /**
     * @param size     the adding size.
     * @param seekType SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @throws com.sequoiadb.exception.BaseException.
     * @fn seek(long size, int seekType)
     * @brief change the read position of the lob. The new position is
     * obtained by adding <code>size</code> to the position
     * specified by <code>seekType</code>. If <code>seekType</code>
     * is set to SDB_LOB_SEEK_SET, SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END,
     * the offset is relative to the start of the lob, the current
     * position of lob, or the end of lob.
     */
    public void seek(long size, int seekType) throws BaseException {
        if (!_isOpen) {
            throw new BaseException("SDB_LOB_NOT_OPEN", "lob is not open");
        }

        if (_mode != SDB_LOB_READ) {
            throw new BaseException("SDB_INVALIDARG", "seek() is not supported"
                    + "in mode=" + _mode);
        }

        if (SDB_LOB_SEEK_SET == seekType) {
            if (size < 0 || size > _size) {
                throw new BaseException("SDB_INVALIDARG", "out of bound");
            }

            _readOffset = size;
        } else if (SDB_LOB_SEEK_CUR == seekType) {
            if ((_size < _readOffset + size)
                    || (_readOffset + size < 0)) {
                throw new BaseException("SDB_INVALIDARG", "out of bound");
            }

            _readOffset += size;
        } else if (SDB_LOB_SEEK_END == seekType) {
            if (size < 0 || size > _size) {
                throw new BaseException("SDB_INVALIDARG", "out of bound");
            }

            _readOffset = _size - size;
        } else {
            throw new BaseException("SDB_INVALIDARG", "unreconigzed seekType:"
                    + seekType);
        }
    }

    private int _reviseReadLen(int needLen) {
        int mod = (int) (_readOffset & (_pageSize - 1));
        // when "needLen" is great than (2^31 - 1) - 3,
        // alignedLen" will be less than 0, but we should not worry
        // about this, because before we finish using the cached data,
        // we won't come here, at that moment, "alignedLen" will be not be less
        // than "needLen"
        int alignedLen = Helper.roundToMultipleXLength(needLen, SDB_LOB_ALIGNED_LEN);
        if (alignedLen < needLen) {
            alignedLen = SDB_LOB_ALIGNED_LEN;
        }
        alignedLen -= mod;
        if (alignedLen < SDB_LOB_ALIGNED_LEN) {
            alignedLen += SDB_LOB_ALIGNED_LEN;
        }
        return alignedLen;
    }

    private boolean _hasDataCached() {
        int remaining = (_cachedDataBuff != null) ? _cachedDataBuff.remaining() : 0;
        return (_cachedDataBuff != null && 0 < remaining &&
                0 <= _cachedOffset &&
                _cachedOffset <= _readOffset &&
                _readOffset < (_cachedOffset + remaining));
    }

    private int _readInCache(byte[] buf, int off, int needRead) {
        if (needRead > buf.length - off) {
            throw new BaseException("SDB_SYS", "buf size is to small");
        }
        int readInCache = (int) (_cachedOffset + _cachedDataBuff.remaining() - _readOffset);
        readInCache = readInCache <= needRead ? readInCache : needRead;
        // if we had used "lobSeek" to adjust "_readOffset",
        // let's adjust the right place to copy data
        if (_readOffset > _cachedOffset) {
            int currentPos = _cachedDataBuff.position();
            int newPos = currentPos + (int) (_readOffset - _cachedOffset);
            _cachedDataBuff.position(newPos);
        }
        // copy the data from cache out to the buf for user
        _cachedDataBuff.get(buf, off, readInCache);
        if (_cachedDataBuff.remaining() == 0) {
            // TODO: shell we need to reuse the ByteBuffer ?
            _cachedDataBuff = null;
        } else {
            _cachedOffset = _readOffset + readInCache;
        }
        return readInCache;
    }

    private int _onceRead(byte[] buf, int off, int len) {
        int needRead = len;
        int totalRead = 0;
        int onceRead = 0;
        int alignedLen = 0;

        // try to get data from local cache
        if (_hasDataCached()) {
            onceRead = _readInCache(buf, off, needRead);
            totalRead += onceRead;
            needRead -= onceRead;
            _readOffset += onceRead;
            return totalRead;
        }

        // get data from database
        _cachedOffset = -1;
        _cachedDataBuff = null;

        // page align
        alignedLen = _reviseReadLen(needRead);
        // build read message
        byte[] request = generateReadLobRequest(alignedLen);
        // seed message to engine
        ByteBuffer res = sendRequest(request, request.length);
        // receive and extract return message
        SDBMessage resMessage = SDBMessageHelper.msgExtractLobReadReply(res);
        /// check the return contents and make sure no error had happen
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_READ_RES) {
            throw new BaseException("SDB_UNKNOWN_MESSAGE",
                    resMessage.getOperationCode());
        }
        int rc = resMessage.getFlags();
        // meet the end of the lob
        if (rc == SequoiadbConstants.SDB_EOF) {
            return -1;
        }
        if (rc != 0) {
            throw new BaseException(rc);
        }
        // sanity check
        // return message is |MsgOpReply|_MsgLobTuple|data|
        int retMsgLen = resMessage.getRequestLength();
        if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH) {
            throw new BaseException("SDB_SYS",
                    "invalid message's length: " + retMsgLen);
        }
        long offsetInEngine = resMessage.getLobOffset();
        if (_readOffset != offsetInEngine) {
            throw new BaseException("SDB_SYS",
                    "local read offset(" + _readOffset +
                            ") is not equal with what we expect(" + offsetInEngine + ")");
        }
        int retLobLen = resMessage.getLobLen();
        if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH + retLobLen) {
            throw new BaseException("SDB_SYS",
                    "invalid message's length: " + retMsgLen);
        }
        /// get return data
        _cachedDataBuff = resMessage.getLobCachedDataBuf();
        resMessage.setLobCachedDataBuf(null);
        // sanity check 
        int remainLen = _cachedDataBuff.remaining();
        if (remainLen != retLobLen) {
            throw new BaseException("SDB_SYS", "the remaining in buffer(" + remainLen +
                    ") is not equal with what we expect(" + retLobLen + ")");
        }
        // if what we got is more than what we expect,
        // let's cache some for next reading request.
        if (needRead < retLobLen) {
            _cachedDataBuff.get(buf, off, needRead);
            totalRead += needRead;
            _readOffset += needRead;
            _cachedOffset = _readOffset;
        } else {
            _cachedDataBuff.get(buf, off, retLobLen);
            totalRead += retLobLen;
            _readOffset += retLobLen;
            _cachedOffset = -1;
            _cachedDataBuff = null;
        }
        return totalRead;
    }

    private int _read(byte[] b, int off, int len) {
        int offset = off;
        int needRead = len;
        int onceRead = 0;
        int totalRead = 0;
        // when no data for return
        if (_readOffset == _size) {
            return -1;
        }
        while (needRead > 0 && _readOffset < _size) {
            onceRead = _onceRead(b, offset, needRead);
            if (onceRead == -1) {
                if (totalRead == 0) {
                    totalRead = -1;
                }
                // when we finish read, let's stop
                break;
            }
            offset += onceRead;
            needRead -= onceRead;
            totalRead += onceRead;
            onceRead = 0;
        }
        return totalRead;
    }

    private byte[] generateReadLobRequest(int length) {
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH;

        // add _MsgOpLob into buff with convert(db.endianConvert)
        ByteBuffer buff = ByteBuffer.allocate(
                SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                        + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH);
        if (_endianConvert) {
            buff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            buff.order(ByteOrder.BIG_ENDIAN);
        }

        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(buff, totalLen,
                Operation.MSG_BS_LOB_READ_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

        //*******************_MsgLobTuple*******************
        addMsgTuple(buff, length, SDB_LOB_DEFAULT_SEQ,
                _readOffset);

        return buff.array();
    }

    private void _write(byte[] input) throws BaseException {
        byte[] request = generateWriteLobRequest(input);
        ByteBuffer res = sendRequest(request, request.length);

        SDBMessage resMessage = SDBMessageHelper.msgExtractReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_WRITE_RES) {
            throw new BaseException("SDB_UNKNOWN_MESSAGE",
                    resMessage.getOperationCode());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag);
        }

        _size += input.length;
    }

    private byte[] generateWriteLobRequest(byte[] input) {
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH
                + Helper.roundToMultipleXLength(input.length, 4);

        // add _MsgOpLob into buff with convert(db.endianConvert)
        ByteBuffer buff = ByteBuffer.allocate(
                SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                        + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH);
        if (_endianConvert) {
            buff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            buff.order(ByteOrder.BIG_ENDIAN);
        }

        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(buff, totalLen,
                Operation.MSG_BS_LOB_WRITE_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

        //*******************_MsgLobTuple*******************
        addMsgTuple(buff, input.length, SDB_LOB_DEFAULT_SEQ,
                SDB_LOB_DEFAULT_OFFSET);

        List<byte[]> buffList = new ArrayList<byte[]>();
        buffList.add(buff.array());
        // TODO: roundToMultipleX will copy data
        buffList.add(Helper.roundToMultipleX(input, 4));

        return Helper.concatByteArray(buffList);
    }

    private void addMsgTuple(ByteBuffer buff, int length, int sequence,
                             long offset) {

        buff.putInt(length);
        buff.putInt(sequence);
        buff.putLong(offset);
    }

    private byte[] generateCloseLobRequest() {
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH;

        // add _MsgOpLob into buff with convert(db.endianConvert)
        ByteBuffer buff = ByteBuffer.allocate(
                SDBMessageHelper.MESSAGE_OPLOB_LENGTH);
        if (_endianConvert) {
            buff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            buff.order(ByteOrder.BIG_ENDIAN);
        }

        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(buff, totalLen,
                Operation.MSG_BS_LOB_CLOSE_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

        return buff.array();
    }

    private byte[] generateOpenLobRequest(BSONObject openLob, int flags) {

        byte bOpenLob[] = SDBMessageHelper.bsonObjectToByteArray(openLob);
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                + Helper.roundToMultipleXLength(bOpenLob.length, 4);

        // convert the openLob's buff
        if (!_endianConvert) {
            SDBMessageHelper.bsonEndianConvert(bOpenLob, 0, bOpenLob.length,
                    true);
        }

        // add _MsgOpLob into buff with convert(db.endianConvert)
        ByteBuffer buff = ByteBuffer.allocate(
                SDBMessageHelper.MESSAGE_OPLOB_LENGTH);
        if (_endianConvert) {
            buff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            buff.order(ByteOrder.BIG_ENDIAN);
        }

        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(buff, totalLen,
                Operation.MSG_BS_LOB_OPEN_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0,
                flags,
                SequoiadbConstants.DEFAULT_CONTEXTID, bOpenLob.length);

        List<byte[]> buffList = new ArrayList<byte[]>();
        buffList.add(buff.array());
        buffList.add(Helper.roundToMultipleX(bOpenLob, 4));

        return Helper.concatByteArray(buffList);
    }

    private void displayResponse(SDBMessage resMessage) {
//        int flag = resMessage.getFlags();
//        System.out.println( "flags=" + flag );
//        List<BSONObject> objList = resMessage.getObjectList();
//        if ( objList != null ) {
//            for ( int i = 0; i < objList.size(); i++ ) {
//                BSONObject obj = objList.get( i );
//                System.out.println( "obj " + i + ":" + obj.toString() );
//            }
//        }
    }
}
