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
import com.sequoiadb.util.Helper;
import com.sequoiadb.util.SDBMessageHelper;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.List;

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
     * @fn ObjectId getID()
     * @brief get the lob's id
     * @return the lob's id
     */
    public ObjectId getID();

    /**
     * @fn long getSize()
     * @brief get the size of lob
     * @return the lob's size
     */
    public long getSize();

    /**
     * @fn long getCreateTime()
     * @brief get the create time of lob
     * @return the lob's create time
     */
    public long getCreateTime();

    /**
     * @fn void write( InputStream in )
     * @brief Writes bytes from the input stream to this lob.
     * @param       in   the input stream.
     * @exception com.sequoiadb.exception.BaseException
     * @note user need to close the input stream
     */
    public void write(InputStream in) throws BaseException;

    /**
     * @fn void write( byte[] b )
     * @brief Writes <code>b.length</code> bytes from the specified
     *              byte array to this lob. 
     * @param       b   the data.
     * @exception com.sequoiadb.exception.BaseException
     */
    public void write(byte[] b) throws BaseException;

    /**
     * @fn void write( byte[] b, int off, int len )
     * @brief Writes <code>len</code> bytes from the specified
     *              byte array starting at offset <code>off</code> to this lob. 
     * @param       b   the data.
     * @param       off the start offset in the data.
     * @param       len the number of bytes to write.
     * @exception com.sequoiadb.exception.BaseException
     */
    public void write(byte[] b, int off, int len) throws BaseException;

    /**
     * @fn void read( OutputStream out )
     * @brief Reads the content to the output stream.
     * @param       out   the output stream.
     * @exception com.sequoiadb.exception.BaseException
     * @note user need to close the output stream
     */
    public void read(OutputStream out) throws BaseException;

    /**
     * @fn int read( byte[] b )
     * @brief Reads up to <code>b.length</code> bytes of data from this lob into
     *              an array of bytes. 
     * @param       b   the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     *              there is no more data because the end of the file has been 
     *              reached, or <code>0</code> if <code>b.length</code> is Zero.
     * @exception com.sequoiadb.exception.BaseException
     */
    public int read(byte[] b) throws BaseException;

    /**
     * @fn int read( byte[] b, int off, int len )
     * @brief Reads up to <code>len</code> bytes of data from this lob into
     *              an array of bytes.
     * @param       b   the buffer into which the data is read.
     * @param       off the start offset in the destination array <code>b</code>.
     * @param       len the maximum number of bytes read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     *              there is no more data because the end of the file has been 
     *              reached, or <code>0</code> if <code>len</code> is Zero.
     * @exception com.sequoiadb.exception.BaseException
     */
    public int read(byte[] b, int off, int len) throws BaseException;

    /**
     * @fn seek(long size, int seekType)
     * @brief change the read position of the lob. The new position is
     *              obtained by adding size to the position specified by 
     *              seekType. If seekType is set to SDB_LOB_SEEK_SET, 
     *              SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, the offset is 
     *              relative to the start of the lob, the current position 
     *              of lob, or the end of lob.
     * @param       size the adding size.
     * @param       seekType  SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void seek(long size, int seekType) throws BaseException;

    /**
     * @fn close()
     * @brief close the lob
     * @param       null
     * @exception com.sequoiadb.exception.BaseException
     */
    public void close() throws BaseException;
}

class DBLobConcrete implements DBLob {
    public final static int SDB_LOB_CREATEONLY = 0x00000001;
    public final static int SDB_LOB_READ = 0x00000004;

    // the max lob data size to send for one message
    private final static int SDB_LOB_MAX_WRITE_DATA_LENGTH = 2097152; // 2M;
    private final static int SDB_LOB_WRITE_DATA_LENGTH = 524288; // 512k;
    private final static int SDB_LOB_READ_DATA_LENGTH = 65536; // 64k;

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
    private ByteBuffer _writeBuff =  null;

    /**
     * @fn DBLob(DBCollection cl)
     * @brief Constructor
     * @param       cl   The instance of DBCollection 
     * @exception com.sequoiadb.exception.BaseException
     */
    public DBLobConcrete(DBCollection cl) throws BaseException {
        if (cl == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "cl is null");
        }

        _cl = cl;
        _endianConvert = cl.getSequoiadb().endianConvert;
    }

    /**
     * @fn open()
     * @brief create a lob, lob's id will auto generate in this function
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void open() {
        open(null, SDB_LOB_CREATEONLY);
    }

    /**
     * @fn open(ObjectId id)
     * @brief open an exist lob with id
     * @param       id   the lob's id
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void open(ObjectId id) {
        open(id, SDB_LOB_READ);
    }

    /**
     * @fn open(ObjectId id, int mode)
     * @brief open an exist lob, or create a lob
     * @param       id   the lob's id
     * @param       mode available mode is SDB_LOB_CREATEONLY or SDB_LOB_READ.
     *              SDB_LOB_CREATEONLY 
     *                  create a new lob with given id, if id is null, it will 
     *                  be generated in this function;
     *              SDB_LOB_READ
     *                  read an exist lob
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void open(ObjectId id, int mode) throws BaseException {
        if (_isOpen) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "lob have opened: id = " + _id);
        }

        if (SDB_LOB_CREATEONLY != mode && SDB_LOB_READ != mode) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "mode is unsupported: " + mode);
        }

        if (SDB_LOB_READ == mode) {
            if (null == id) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "id must be specify"
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

        ByteBuffer request = generateOpenLobRequest(openLob, flags);
        ByteBuffer res = sendRequest(request);

        SDBMessage resMessage = SDBMessageHelper.msgExtractLobOpenReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_OPEN_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    resMessage.getOperationCode().toString());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag, openLob.toString());
        }
        List<BSONObject> objList = resMessage.getObjectList();
        if (objList.size() != 1) {
            throw new BaseException(SDBError.SDB_NET_BROKEN_MSG,
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

    private ByteBuffer sendRequest(ByteBuffer request)
            throws BaseException {
        if (request == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "request can't be null");
        }

        _cl.getConnection().sendMessage(request);
        return _cl.getConnection().receiveMessage(_endianConvert);
    }

    /**
     * @fn getID()
     * @brief get the lob's id
     * @return the lob's id
     */
    public ObjectId getID() {
        return _id;
    }

    /**
     * @fn getSize()
     * @brief get the size of lob
     * @return the lob's size
     */
    public long getSize() {
        return _size;
    }

    /**
     * @fn getCreateTime()
     * @brief get the create time of lob
     * @return the lob's create time
     */
    public long getCreateTime() {
        return _createTime;
    }

    /**
     * @fn close()
     * @brief close the lob
     * @exception com.sequoiadb.exception.BaseException
     */
    public void close() throws BaseException {
        if (!_isOpen) {
            return;
        }

        ByteBuffer request = generateCloseLobRequest();
        ByteBuffer res = sendRequest(request);

        SDBMessage resMessage = SDBMessageHelper.msgExtractReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_CLOSE_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    resMessage.getOperationCode().toString());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag);
        }

        _isOpen = false;
        recycleWriteBuff();
    }

    /**
     * @fn void write( InputStream in )
     * @brief Writes bytes from the input stream to this lob.
     * @param       in   the input stream.
     * @exception com.sequoiadb.exception.BaseException
     */
    @Override
    public void write(InputStream in) throws BaseException {
        if (!_isOpen) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }
        if (in == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "input is null");
        }
        // get data from input stream
        int readNum = 0;
        byte[] tmpBuf = new byte[SDB_LOB_WRITE_DATA_LENGTH];
        try {
            while (-1 < (readNum = in.read(tmpBuf))) {
                write(tmpBuf, 0, readNum);
            }
        } catch (IOException e) {
            BaseException exp = new BaseException(SDBError.SDB_SYS);
            exp.initCause(e);
            throw exp;
        }
    }

    /**
     * @fn void write( byte[] b )
     * @brief Writes <code>b.length</code> bytes from the specified
     *              byte array to this lob. 
     * @param       b   the data.
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void write(byte[] b) throws BaseException {
        write(b, 0, b.length);
    }

    /**
     * @fn void write( byte[] b, int off, int len )
     * @brief Writes <code>len</code> bytes from the specified
     *              byte array starting at offset <code>off</code> to this lob. 
     * @param       b   the data.
     * @param       off the start offset in the data.
     * @param       len the number of bytes to write.
     * @exception com.sequoiadb.exception.BaseException
     */
    public void write(byte[] b, int off, int len) throws BaseException {
        if (!_isOpen) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (b == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "input is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "off + len is great than b.length");
        }
        int offset = off;
        int leftLen = len;
        int writeLen = 0;
        while (leftLen > 0) {
            writeLen = (leftLen < SDB_LOB_MAX_WRITE_DATA_LENGTH) ?
                    leftLen : SDB_LOB_MAX_WRITE_DATA_LENGTH;
            _write(b, offset, writeLen);
            leftLen -= writeLen;
            offset += writeLen;
        }

    }

    /**
     * @fn void read( OutputStream out )
     * @brief Reads data from this
     *              lob into the output stream. 
     * @param       out the output stream.
     * @exception com.sequoiadb.exception.BaseException.
     */
    @Override
    public void read(OutputStream out) throws BaseException {
        if (!_isOpen) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (out == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "output stream is null");
        }
        // read data to output stream
        int readNum = 0;
        byte[] tmpBuf = new byte[SDB_LOB_READ_DATA_LENGTH];
        while (-1 < (readNum = read(tmpBuf, 0, tmpBuf.length))) {
            try {
                out.write(tmpBuf, 0, readNum);
            } catch (IOException e) {
                BaseException exp = new BaseException(SDBError.SDB_SYS);
                exp.initCause(e);
                throw exp;
            }
        }
    }

    /**
     * @fn read(byte[] b)
     * @brief Reads up to <code>b.length</code> bytes of data from this
     *              lob into an array of bytes. 
     * @param       b   the buffer into which the data is read.
     * @return the total number of bytes read into the buffer, or
     *              <code>-1</code> if there is no more data because the end of
     *              the file has been reached, or <code>0<code> if 
     *              <code>b.length</code> is Zero.
     * @exception com.sequoiadb.exception.BaseException.
     */
    public int read(byte[] b) throws BaseException {
        return read(b, 0, b.length);
    }

    /**
     * @fn int read( byte[] b, int off, int len )
     * @brief Reads up to <code>len</code> bytes of data from this lob into
     *              an array of bytes.
     * @param       b   the buffer into which the data is read.
     * @param       off the start offset in the destination array <code>b</code>.
     * @param       len the maximum number of bytes read.
     * @return the total number of bytes read into the buffer, or <code>-1</code> if
     *              there is no more data because the end of the file has been 
     *              reached, or <code>0</code> if <code>len</code> is Zero.
     * @exception com.sequoiadb.exception.BaseException
     */
    public int read(byte[] b, int off, int len) throws BaseException {
        if (!_isOpen) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (b == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "b is null");
        }

        if (len < 0 || len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid len");
        }

        if (off < 0 || off > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid off");
        }

        if (off + len > b.length) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "off + len is great than b.length");
        }

        if (b.length == 0) {
            return 0;
        }
        return _read(b, off, len);
    }

    /**
     * @fn seek(long size, int seekType)
     * @brief change the read position of the lob. The new position is
     *              obtained by adding <code>size</code> to the position 
     *              specified by <code>seekType</code>. If <code>seekType</code> 
     *              is set to SDB_LOB_SEEK_SET, SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, 
     *              the offset is relative to the start of the lob, the current 
     *              position of lob, or the end of lob.
     * @param       size the adding size.
     * @param       seekType  SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
     * @exception com.sequoiadb.exception.BaseException.
     */
    public void seek(long size, int seekType) throws BaseException {
        if (!_isOpen) {
            throw new BaseException(SDBError.SDB_LOB_NOT_OPEN, "lob is not open");
        }

        if (_mode != SDB_LOB_READ) {
            throw new BaseException(SDBError.SDB_OPTION_NOT_SUPPORT, "seek() is not supported"
                    + " in mode=" + _mode);
        }

        if (SDB_LOB_SEEK_SET == seekType) {
            if (size < 0 || size > _size) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "out of bound");
            }

            _readOffset = size;
        } else if (SDB_LOB_SEEK_CUR == seekType) {
            if ((_size < _readOffset + size)
                    || (_readOffset + size < 0)) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "out of bound");
            }

            _readOffset += size;
        } else if (SDB_LOB_SEEK_END == seekType) {
            if (size < 0 || size > _size) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "out of bound");
            }

            _readOffset = _size - size;
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, "unreconigzed seekType: " + seekType);
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
            throw new BaseException(SDBError.SDB_SYS, "buf size is to small");
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
        ByteBuffer request = generateReadLobRequest(alignedLen);
        // seed message to engine
        ByteBuffer res = sendRequest(request);
        // receive and extract return message
        SDBMessage resMessage = SDBMessageHelper.msgExtractLobReadReply(res);
        /// check the return contents and make sure no error had happen
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_READ_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    resMessage.getOperationCode().toString());
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
            throw new BaseException(SDBError.SDB_SYS,
                    "invalid message's length: " + retMsgLen);
        }
        long offsetInEngine = resMessage.getLobOffset();
        if (_readOffset != offsetInEngine) {
            throw new BaseException(SDBError.SDB_SYS,
                    "local read offset(" + _readOffset +
                            ") is not equal with what we expect(" + offsetInEngine + ")");
        }
        int retLobLen = resMessage.getLobLen();
        if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH + retLobLen) {
            throw new BaseException(SDBError.SDB_SYS,
                    "invalid message's length: " + retMsgLen);
        }
        /// get return data
        _cachedDataBuff = resMessage.getLobCachedDataBuf();
        resMessage.setLobCachedDataBuf(null);
        // sanity check 
        int remainLen = _cachedDataBuff.remaining();
        if (remainLen != retLobLen) {
            throw new BaseException(SDBError.SDB_SYS, "the remaining in buffer(" + remainLen +
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

    private ByteBuffer generateReadLobRequest(int length) {
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

        return buff;
    }

    private void _write(byte[] input, int off, int len) throws BaseException {
        if (len == 0) {
            return;
        }
        ByteBuffer request = generateWriteLobRequest(input, off, len);
        // send and receive msg
        ByteBuffer res = sendRequest(request);
        // extract info from return msg
        SDBMessage resMessage = SDBMessageHelper.msgExtractReply(res);
        displayResponse(resMessage);
        if (resMessage.getOperationCode() != Operation.MSG_BS_LOB_WRITE_RES) {
            throw new BaseException(SDBError.SDB_UNKNOWN_MESSAGE,
                    resMessage.getOperationCode().toString());
        }
        int flag = resMessage.getFlags();
        if (0 != flag) {
            throw new BaseException(flag);
        }

        _size += len;
    }

    private void _getWriteBuff(int len) {
        if (_writeBuff == null) {
            _writeBuff = ByteBuffer.allocate(len);
        }else if (_writeBuff.capacity() < len) {
            _writeBuff = ByteBuffer.allocate(len);
        }else if (_writeBuff.capacity() > len) {
            _writeBuff.clear();
            _writeBuff.limit(len);
        }else {
            _writeBuff.clear();
        }
    }

    private void recycleWriteBuff(){
        _writeBuff = null;
    }

    private ByteBuffer generateWriteLobRequest(byte[] input, int off, int len) {
        if (off + len > input.length) {
            throw new BaseException(SDBError.SDB_SYS, "off + len is more than input.length");
        }
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH
                + Helper.roundToMultipleXLength(len, 4);
        // alloc ByteBuffer
        _getWriteBuff(totalLen);
        if (_endianConvert) {
            _writeBuff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            _writeBuff.order(ByteOrder.BIG_ENDIAN);
        }
        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(_writeBuff, totalLen,
                Operation.MSG_BS_LOB_WRITE_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(_writeBuff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

        //*******************_MsgLobTuple*******************
        addMsgTuple(_writeBuff, len, SDB_LOB_DEFAULT_SEQ,
                SDB_LOB_DEFAULT_OFFSET);

        //*******************lob data*******************
        Helper.addBytesToByteBuffer(_writeBuff, input, off, len, 4);

        return _writeBuff;
    }

    private void addMsgTuple(ByteBuffer buff, int length, int sequence,
                             long offset) {

        buff.putInt(length);
        buff.putInt(sequence);
        buff.putLong(offset);
    }

    private ByteBuffer generateCloseLobRequest() {
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
        return buff;
    }

    private ByteBuffer generateOpenLobRequest(BSONObject openLob, int flags) {

        byte bOpenLob[] = SDBMessageHelper.bsonObjectToByteArray(openLob);
        int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                + Helper.roundToMultipleXLength(bOpenLob.length, 4);

        // convert the openLob's buff
        if (!_endianConvert) {
            SDBMessageHelper.bsonEndianConvert(bOpenLob, 0, bOpenLob.length,
                    true);
        }

        // alloc buffer
        ByteBuffer totalBuff = ByteBuffer.allocate(totalLen);
        if (_endianConvert) {
            totalBuff.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            totalBuff.order(ByteOrder.BIG_ENDIAN);
        }

        //*******************MsgHeader*******************
        SDBMessageHelper.addLobMsgHeader(totalBuff, totalLen,
                Operation.MSG_BS_LOB_OPEN_REQ.getOperationCode(),
                SequoiadbConstants.ZERO_NODEID, 0);

        //*******************_MsgOpLob**********************
        SDBMessageHelper.addLobOpMsg(totalBuff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short) 0, flags,
                SequoiadbConstants.DEFAULT_CONTEXTID, bOpenLob.length);

        //*******************_meta**********************
        Helper.addBytesToByteBuffer(totalBuff, bOpenLob, 0, bOpenLob.length, 4);

        return totalBuff;
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
