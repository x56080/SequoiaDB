using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using SequoiaDB.Bson;

namespace SequoiaDB
{

    /** \class DBLob
     *  \brief Database operation interfaces of large object
     */
	public class  DBLob
	{
        /**
         *  \memberof SDB_LOB_SEEK_SET 0
         *  \brief Change the position from the beginning of lob 
         */
        public const int SDB_LOB_SEEK_SET   = 0;
    
        /**
         *  \memberof SDB_LOB_SEEK_CUR 1
         *  \brief Change the position from the current position of lob 
         */
        public const int SDB_LOB_SEEK_CUR   = 1;
    
        /**
         *  \memberof SDB_LOB_SEEK_END 2
         *  \brief Change the position from the end of lob 
         */
        public const int SDB_LOB_SEEK_END   = 2;

        /**
         *  \memberof SDB_LOB_CREATEONLY 0x00000001
         *  \brief Open a new lob only
         */
        public const int SDB_LOB_CREATEONLY = 0x00000001;

        /**
         *  \memberof SDB_LOB_READ 0x00000004
         *  \brief Open an existing lob to read
         */
        public const int SDB_LOB_READ       = 0x00000004;

        // the max lob data size to send for one message
        private const int SDB_LOB_MAX_WRITE_DATA_LENGTH = 2097152; // 2M;
        private const int SDB_LOB_WRITE_DATA_LENGTH = 524288; // 512k;
        private const int SDB_LOB_READ_DATA_LENGTH = 65536; // 64k;

        private const int SDB_LOB_ALIGNED_LEN = 524288; // 512k
        private const int FLG_LOBOPEN_WITH_RETURNDATA = 0x00000002;
    
        private const long SDB_LOB_DEFAULT_OFFSET  = -1;
        private const int SDB_LOB_DEFAULT_SEQ      = 0;
    
        private DBCollection _cl = null;
        private IConnection  _connection = null;
        internal bool        _isBigEndian = false;

        private ObjectId     _id;
        private int          _mode;
        private int          _pageSize;
        private long         _size;
        private long         _createTime;
        private long         _readOffset;
        private long         _cachedOffset;
        private ByteBuffer   _cachedDataBuff;
        private bool         _isOpen;
        // when first open/create DBLob, sequoiadb return the contextID for the
        // further reading/writing/close
        private long         _contextID;

        internal DBLob(DBCollection cl)
        {
            this._cl = cl;
            this._connection = cl.CollSpace.SequoiaDB.Connection;
            this._isBigEndian = cl.isBigEndian;
            _id = ObjectId.Empty;
            _mode = -1;
            _size = 0;
            _createTime = 0;
            _readOffset = -1;
            _cachedOffset = -1;
            _cachedDataBuff = null;
            _isOpen = false;
            _contextID = -1;
        }
/*
        internal void Open()
        {
            Open(ObjectId.Empty, SDB_LOB_CREATEONLY);
        }

        internal void Open(ObjectId id)
        {
            Open(id, SDB_LOB_READ);
        }
*/
        /** \fn         Open( ObjectId id, int mode )
         * \brief       Open an exist lob, or create a lob
         * \param       id   the lob's id
         * \param       mode available mode is SDB_LOB_CREATEONLY or SDB_LOB_READ.
         *              SDB_LOB_CREATEONLY 
         *                  create a new lob with given id, if id is null, it will 
         *                  be generated in this function;
         *              SDB_LOB_READ
         *                  read an exist lob
         * \exception SequoiaDB.BaseException
         * \exception System.Exception
         */
        internal void Open(ObjectId id, int mode)
        {
            // check
            if (_isOpen)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_HAS_OPEN);
            }
            if (SDB_LOB_CREATEONLY != mode && SDB_LOB_READ != mode)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG);
            }
            if (SDB_LOB_READ == mode)
            {
                if (ObjectId.Empty == id)
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG);
                }
            }
            // gen oid
            _id = id;
            if (SDB_LOB_CREATEONLY == mode)
            {
                if (ObjectId.Empty == _id)
                {
                    _id = ObjectId.GenerateNewId();
                }
            }
            // mode
            _mode = mode;
            _readOffset = 0;
            // open
            _Open();
            _isOpen = true;
        }

        /** \fn          Close()
          * \brief       Close the lob
          * \return void
          * \exception SequoiaDB.BaseException
          * \exception System.Exception
          */
        public void Close()
        {
            if (!_isOpen)
            {
                return;
            }

            ByteBuffer request = _GenerateCloseLobRequest();
            ByteBuffer respone = _SendAndReiveMessage(request);

            SDBMessage retInfo = SDBMessageHelper.ExtractReply(respone);
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_CLOSE_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int flag = retInfo.Flags;
            if (0 != flag)
            {
                throw new BaseException(flag);
            }

            _isOpen = false;
        }

        /** \fn          Read( byte[] b )
         *  \brief       Reads up to b.length bytes of data from this 
         *               lob into an array of bytes. 
         *  \param       b   the buffer into which the data is read.
         *  \return      the total number of bytes read into the buffer, or
         *               <code>-1</code> if there is no more data because the end of
         *               the file has been reached, or <code>0<code> if 
         *               <code>b.length</code> is Zero.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public int Read(byte[] b)
        {
            return Read(b, 0, b.Length);
        }

        /** \fn          Read(byte[] b, int off, int len)
         *  \brief       Reads up to <code>len</code> bytes of data from this lob into
         *               an array of bytes.
         *  \param       b   the buffer into which the data is read.
         *  \param       off the start offset in the destination array <code>b</code>.
         *  \param       len the maximum number of bytes read.
         *  \return      the total number of bytes read into the buffer, or
         *               <code>-1</code> if there is no more data because the end of
         *               the file has been reached, or <code>0<code> if 
         *               <code>b.length</code> is Zero.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public int Read(byte[] b, int off, int len)
        {
            if (!_isOpen)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (b == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "b is null");
            }

            if (len < 0 || len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid len");
            }

            if (off < 0 || off > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid off");
            }

            if (off + len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "off + len is great than b.Length");
            }

            if (b.Length == 0)
            {
                return 0;
            }
            return _Read(b, off, len);
        }

        /** \fn          Write( byte[] b )
         *  \brief       Writes b.length bytes from the specified 
         *               byte array to this lob. 
         *  \param       b   The data.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Write(byte[] b)
        {
            Write(b, 0, b.Length);
        }

        /** \fn          Write(byte[] b, int off, int len)
         *  \brief       Writes len bytes from the specified 
         *               byte array to this lob. 
         *  \param       b   The data.
         *  \param       off   The offset of the data buffer.
         *  \param       len   The lenght to write.
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Write(byte[] b, int off, int len)
        {
            if (!_isOpen)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (b == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "input is null");
            }

            if (len < 0 || len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid len");
            }

            if (off < 0 || off > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "invalid off");
            }

            if (off + len > b.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "off + len is great than b.length");
            }
            int offset = off;
            int leftLen = len;
            int writeLen = 0;
            while (leftLen > 0)
            {
                writeLen = (leftLen < SDB_LOB_MAX_WRITE_DATA_LENGTH) ?
                        leftLen : SDB_LOB_MAX_WRITE_DATA_LENGTH;
                _Write(b, offset, writeLen);
                leftLen -= writeLen;
                offset += writeLen;
            }
        }

        /** \fn          void Seek( long size, int seekType )
         *  \brief       Change the read position of the lob. The new position is 
         *               obtained by adding <code>size</code> to the position 
         *               specified by <code>seekType</code>. If <code>seekType</code> 
         *               is set to SDB_LOB_SEEK_SET, SDB_LOB_SEEK_CUR, or SDB_LOB_SEEK_END, 
         *               the offset is relative to the start of the lob, the current 
         *               position of lob, or the end of lob.
         *  \param       size the adding size.
         *  \param       seekType  SDB_LOB_SEEK_SET/SDB_LOB_SEEK_CUR/SDB_LOB_SEEK_END
         *  \return void
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public void Seek(long size, int seekType)
        {
            if (!_isOpen)
            {
                throw new BaseException((int)Errors.errors.SDB_LOB_NOT_OPEN, "lob is not open");
            }

            if (_mode != SDB_LOB_READ)
            {
                throw new BaseException((int)Errors.errors.SDB_OPTION_NOT_SUPPORT, "seek() is not supported"
                        + " in mode=" + _mode);
            }

            if (SDB_LOB_SEEK_SET == seekType)
            {
                if (size < 0 || size > _size)
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "out of bound");
                }

                _readOffset = size;
            }
            else if (SDB_LOB_SEEK_CUR == seekType)
            {
                if ((_size < _readOffset + size)
                        || (_readOffset + size < 0))
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "out of bound");
                }

                _readOffset += size;
            }
            else if (SDB_LOB_SEEK_END == seekType)
            {
                if (size < 0 || size > _size)
                {
                    throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "out of bound");
                }

                _readOffset = _size - size;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "unreconigzed seekType: " + seekType);
            }
        }

        /** \fn          bool IsClosed()
         *  \brief       Test whether lob has been closed or not
         *  \return      true for lob has been closed, false for not
         */
        public bool IsClosed()
        {
            return !_isOpen;
        }

        /** \fn          ObjectId GetID()
         *  \brief       Get the lob's id
         *  \return      the lob's id
         */
        public ObjectId GetID()
        {
            return _id;
        }

        /** \fn          long GetSize()
         *  \brief       Get the size of lob
         *  \return      the lob's size
         */
        public long GetSize()
        {
            return _size;
        }

        /** \fn          long GetCreateTime()
         *  \brief       get the create time of lob
         *  \return The lob's create time
         *  \exception SequoiaDB.BaseException
         *  \exception System.Exception
         */
        public long GetCreateTime()
        { 
            return _createTime;
        }

        /************************************** private methond **************************************/

        private void _Open()
        {
            /*
                /// open reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            // add info into object
            BsonDocument openLob = new BsonDocument();
            openLob.Add(SequoiadbConstants.FIELD_COLLECTION, _cl.FullName);
            openLob.Add(SequoiadbConstants.FIELD_LOB_OID, _id);
            openLob.Add(SequoiadbConstants.FIELD_LOB_OPEN_MODE, _mode);

            // set return data flag
            int flags = (_mode == SDB_LOB_READ) ? FLG_LOBOPEN_WITH_RETURNDATA :
                SequoiadbConstants.DEFAULT_FLAGS;

            // generate request
            ByteBuffer request = _GenerateOpenLobRequest(openLob, flags);

            // send and receive message
            ByteBuffer respone = _SendAndReiveMessage(request);

            // extract info from the respone
            SDBMessage retInfo = SDBMessageHelper.ExtractLobOpenReply(respone);

            // check the respone opcode
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_OPEN_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }

            // check the result
            int rc = retInfo.Flags;
            if (rc != 0)
            {
                throw new BaseException(rc);
            }
            // get lob info return from engine
            List<BsonDocument> objList = retInfo.ObjectList;
            if (objList.Count() != 1)
            {
                throw new BaseException((int)Errors.errors.SDB_NET_BROKEN_MSG);
            }
            BsonDocument obj = objList[0];
            if (null == obj)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            // lob size
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_SIZE) && obj[SequoiadbConstants.FIELD_LOB_SIZE].IsInt64)
            {
                _size = obj[SequoiadbConstants.FIELD_LOB_SIZE].AsInt64;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            // lob create time
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_CREATTIME) && obj[SequoiadbConstants.FIELD_LOB_CREATTIME].IsInt64)
            {
                _createTime = obj[SequoiadbConstants.FIELD_LOB_CREATTIME].AsInt64;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            // page size
            if (obj.Contains(SequoiadbConstants.FIELD_LOB_PAGESIZE) && obj[SequoiadbConstants.FIELD_LOB_PAGESIZE].IsInt32)
            {
                _pageSize = obj[SequoiadbConstants.FIELD_LOB_PAGESIZE].AsInt32;
            }
            else
            {
                throw new BaseException((int)Errors.errors.SDB_SYS);
            }
            // cache data
            _cachedDataBuff = retInfo.LobCachedDataBuf;
            if (_cachedDataBuff != null)
            {
                _readOffset = 0;
                _cachedOffset = _readOffset;
            }
            retInfo.LobCachedDataBuf = null;
            // contextID
            _contextID = retInfo.ContextIDList[0];
        }

        private int _Read(byte[] b, int off, int len)
        {
            int offset = off;
            int needRead = len;
            int onceRead = 0;
            int totalRead = 0;

            // when no data for return
            if (_readOffset >= _size)
            {
                return -1;
            }
            while (needRead > 0 && _readOffset < _size)
            {
                onceRead = _OnceRead(b, offset, needRead);
                if (onceRead == -1)
                {
                    if (totalRead == 0)
                    {
                        totalRead = -1;
                    }
                    // when we finish reading, let's stop
                    break;
                }
                offset += onceRead;
                needRead -= onceRead;
                totalRead += onceRead;
                onceRead = 0;
            }
            return totalRead;
        }

        private void _Write(byte[] input, int off, int len)
        {
            if (len == 0)
            {
                return;
            }
            ByteBuffer request = _GenerateWriteLobRequest(input, off, len);
            // send and receive msg
            ByteBuffer respone = _SendAndReiveMessage(request);
            // extract info from return msg
            SDBMessage retInfo = SDBMessageHelper.ExtractReply(respone);

            if (retInfo.OperationCode != Operation.MSG_BS_LOB_WRITE_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int flag = retInfo.Flags;
            if (0 != flag)
            {
                throw new BaseException(flag);
            }
            _size += len;
        }

        private int _OnceRead(byte[] buf, int off, int len)
        {
            int needRead = len;
            int totalRead = 0;
            int onceRead = 0;
            int alignedLen = 0;

            // try to get data from local needRead
            if (_HasDataCached())
            {
                onceRead = _ReadInCache(buf, off, needRead);
                totalRead += onceRead;
                needRead -= onceRead;
                _readOffset += onceRead;
                return totalRead;
            }

            // get data from database
            _cachedOffset = -1;
            _cachedDataBuff = null;

            // page align
            alignedLen = _ReviseReadLen(needRead);
            // build read message
            ByteBuffer request = _GenerateReadLobRequest(alignedLen);
            // seed and receive message to engine
            ByteBuffer respone = _SendAndReiveMessage(request);
            // extract return message
            SDBMessage retInfo = SDBMessageHelper.ExtractLobReadReply(respone);
            if (retInfo.OperationCode != Operation.MSG_BS_LOB_READ_RES)
            {
                throw new BaseException((int)Errors.errors.SDB_UNKNOWN_MESSAGE,
                        string.Format("Receive Unexpected operation code: {0}", retInfo.OperationCode));
            }
            int rc = retInfo.Flags;
            // meet the end of the lob
            if (rc == (int)Errors.errors.SDB_EOF)
            {
                return -1;
            }
            if (rc != 0)
            {
                throw new BaseException(rc);
            }
            // sanity check
            // return message is |MsgOpReply|_MsgLobTuple|data|
            int retMsgLen = retInfo.RequestLength;
            if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                    SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "invalid message's length: " + retMsgLen);
            }
            long offsetInEngine = retInfo.LobOffset;
            if (_readOffset != offsetInEngine)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "local read offset(" + _readOffset +
                                ") is not equal with what we expect(" + offsetInEngine + ")");
            }
            int retLobLen = (int)retInfo.LobLen;
            if (retMsgLen < SDBMessageHelper.MESSAGE_OPREPLY_LENGTH +
                    SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH + retLobLen)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                        "invalid message's length: " + retMsgLen);
            }
            /// get return data
            _cachedDataBuff = retInfo.LobCachedDataBuf;
            retInfo.LobCachedDataBuf = null;
            // sanity check 
            int remainLen = _cachedDataBuff.Remaining();
            if (remainLen != retLobLen)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "the remaining in buffer(" + remainLen +
                        ") is not equal with what we expect(" + retLobLen + ")");
            }
            // if what we got is more than what we expect,
            // let's cache some for next reading request.
            int copy = (needRead < retLobLen) ? needRead : retLobLen;
            int output = _cachedDataBuff.PopByteArray(buf, off, copy);
            if (copy != output)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                    string.Format("Invalid bytes length in the cached buffer, copy {0} bytes, actually output {1} bytes", copy, output));
            }
            totalRead += output;
            _readOffset += output;
            if (needRead < retLobLen)
            {
                _cachedOffset = _readOffset;
            }
            else
            {
                _cachedOffset = -1;
                _cachedDataBuff = null;
            }
            return totalRead;
        }

        private bool _HasDataCached()
        {
            int remaining = (_cachedDataBuff != null) ? _cachedDataBuff.Remaining() : 0;
            return (_cachedDataBuff != null && 0 < remaining &&
                    0 <= _cachedOffset &&
                    _cachedOffset <= _readOffset &&
                    _readOffset < (_cachedOffset + remaining));
        }

        private int _ReadInCache(byte[] buf, int off, int needRead)
        {
            if (needRead > buf.Length - off)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "buf size is to small");
            }
            int readInCache = (int)(_cachedOffset + _cachedDataBuff.Remaining() - _readOffset);
            readInCache = readInCache <= needRead ? readInCache : needRead;
            // if we had used "lobSeek" to adjust "_readOffset",
            // let's adjust the right place to copy data
            if (_readOffset > _cachedOffset)
            {
                int currentPos = _cachedDataBuff.Position();
                int newPos = currentPos + (int)(_readOffset - _cachedOffset);
                _cachedDataBuff.Position(newPos);
            }
            // copy the data from cache out to the buf for user
            int outputNum = _cachedDataBuff.PopByteArray(buf, off, readInCache);
            if (readInCache != outputNum)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS,
                   string.Format("Invalid bytes length in the cached buffer, copy {0} bytes, actually output {1} bytes", readInCache, outputNum));
            }
            if (_cachedDataBuff.Remaining() == 0)
            {
                // TODO: shell we need to reuse the ByteBuffer ?
                _cachedDataBuff = null;
            }
            else
            {
                _cachedOffset = _readOffset + readInCache;
            }
            return readInCache;
        }

        private int _ReviseReadLen(int needLen)
        {
            int mod = (int)(_readOffset & (_pageSize - 1));
            // when "needLen" is great than (2^31 - 1) - 3,
            // alignedLen" will be less than 0, but we should not worry
            // about this, because before we finish using the cached data,
            // we won't come here, at that moment, "alignedLen" will be not be less
            // than "needLen"
            int alignedLen = Helper.RoundToMultipleXLength(needLen, SDB_LOB_ALIGNED_LEN);
            if (alignedLen < needLen)
            {
                alignedLen = SDB_LOB_ALIGNED_LEN;
            }
            alignedLen -= mod;
            if (alignedLen < SDB_LOB_ALIGNED_LEN)
            {
                alignedLen += SDB_LOB_ALIGNED_LEN;
            }
            return alignedLen;
        }

        // TODO: need to put these functions to SDBMessageHelper.cs ?
        private ByteBuffer _GenerateOpenLobRequest(BsonDocument openLob, int flags)
        {
            /*
                /// open reg msg is |MsgOpLob|bsonobj|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            byte[] openLobBytes = openLob.ToBson();

            // get the total length of buffer we need
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH +
                Helper.RoundToMultipleXLength(openLobBytes.Length, 4);

            // alloc buffer
            ByteBuffer totalBuff = new ByteBuffer(totalLen);
            if (_isBigEndian)
            {
                totalBuff.IsBigEndian = true;
                // keep the openLobBytes save in big endian
                SDBMessageHelper.BsonEndianConvert(openLobBytes, 0, openLobBytes.Length, true);
            }

            // MsgHeader
            SDBMessageHelper.AddMsgHeader(totalBuff, totalLen, 
                (int)Operation.MSG_BS_LOB_OPEN_REQ, SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(totalBuff,SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W,(short)0,flags,
                SequoiadbConstants.DEFAULT_CONTEXTID,openLobBytes.Length);

            // meta
            SDBMessageHelper.AddBytesToByteBuffer(totalBuff, openLobBytes, 0, openLobBytes.Length, 4);

            return totalBuff;
        }

        private ByteBuffer _GenerateCloseLobRequest()
        {
            /*
                /// close reg msg is |MsgOpLob|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;
             */
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH;

            // add _MsgOpLob into buff with convert(db.endianConvert)
            ByteBuffer buff = new ByteBuffer(SDBMessageHelper.MESSAGE_OPLOB_LENGTH);
            buff.IsBigEndian = _isBigEndian;

            // MsgHeader
            SDBMessageHelper.AddMsgHeader(buff, totalLen,
                    (int)Operation.MSG_BS_LOB_CLOSE_REQ,
                    SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                    SequoiadbConstants.DEFAULT_W, (short)0,
                    SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);
            return buff;
        }

        private ByteBuffer _GenerateReadLobRequest(int length)
        {
            /*
                /// read req msg is |MsgOpLob|_MsgLobTuple|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;

                union _MsgLobTuple
                {
                   struct
                   {
                      UINT32 len ;
                      UINT32 sequence ;
                      SINT64 offset ;
                   } columns ;

                   CHAR data[16] ;
                } ;
             */
            // total length of the message
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                    + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH;

            // new byte buffer
            ByteBuffer buff = new ByteBuffer(totalLen);
            buff.IsBigEndian = _isBigEndian;

            // add MsgHeader
            SDBMessageHelper.AddMsgHeader(buff, totalLen,
                (int)Operation.MSG_BS_LOB_READ_REQ,
                SequoiadbConstants.ZERO_NODEID, 0);

            // add MsgOpLob
            SDBMessageHelper.AddLobOpMsg(buff, SequoiadbConstants.DEFAULT_VERSION,
                SequoiadbConstants.DEFAULT_W, (short)0,
                SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

            // add MsgLobTuple
            AddMsgTuple(buff, length, SDB_LOB_DEFAULT_SEQ, _readOffset);

            return buff;
        }

        private ByteBuffer _GenerateWriteLobRequest(byte[] input, int off, int len)
        {
            /*
                /// write req msg is |MsgOpLob|_MsgLobTuple|data|
                struct _MsgHeader
                {
                   SINT32 messageLength ; // total message size, including this
                   SINT32 opCode ;        // operation code
                   UINT32 TID ;           // client thead id
                   MsgRouteID routeID ;   // route id 8 bytes
                   UINT64 requestID ;     // identifier for this message
                } ;

                typedef struct _MsgOpLob
                {
                   MsgHeader header ;
                   INT32 version ;
                   SINT16 w ;
                   SINT16 padding ;
                   SINT32 flags ;
                   SINT64 contextID ;
                   UINT32 bsonLen ;
                } MsgOpLob ;

                union _MsgLobTuple
                {
                   struct
                   {
                      UINT32 len ;
                      UINT32 sequence ;
                      SINT64 offset ;
                   } columns ;

                   CHAR data[16] ;
                } ;
             */
            if (off + len > input.Length)
            {
                throw new BaseException((int)Errors.errors.SDB_SYS, "off + len is more than input.length");
            }
            int totalLen = SDBMessageHelper.MESSAGE_OPLOB_LENGTH
                    + SDBMessageHelper.MESSAGE_LOBTUPLE_LENGTH
                    + Helper.RoundToMultipleXLength(len, 4);
            // alloc ByteBuffer
            ByteBuffer totalBuf = new ByteBuffer(totalLen);
            totalBuf.IsBigEndian = _isBigEndian;
            
            // MsgHeader
            SDBMessageHelper.AddMsgHeader(totalBuf, totalLen,
                    (int)Operation.MSG_BS_LOB_WRITE_REQ,
                    SequoiadbConstants.ZERO_NODEID, 0);

            // MsgOpLob
            SDBMessageHelper.AddLobOpMsg(totalBuf, SequoiadbConstants.DEFAULT_VERSION,
                    SequoiadbConstants.DEFAULT_W, (short)0,
                    SequoiadbConstants.DEFAULT_FLAGS, _contextID, 0);

            // MsgLobTuple
            AddMsgTuple(totalBuf, len, SDB_LOB_DEFAULT_SEQ,
                    SDB_LOB_DEFAULT_OFFSET);

            // lob data
            SDBMessageHelper.AddBytesToByteBuffer(totalBuf, input, off, len, 4);

            return totalBuf;
        }

        private void AddMsgTuple(ByteBuffer buff, int length, int sequence,
                                 long offset)
        {
            buff.PushInt(length);
            buff.PushInt(sequence);
            buff.PushLong(offset);
        }

        private ByteBuffer _SendAndReiveMessage(ByteBuffer request)
        {
            if (request == null)
            {
                throw new BaseException((int)Errors.errors.SDB_INVALIDARG, "request can't be null");
            }

            _connection.SendMessage(request.ByteArray());
            return _connection.ReceiveMessage2(_isBigEndian);
        }

	}
}
