package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;

public class LobPutRequest extends LobRequest {

    private static final int FIXED_LENGTH = 96; // LOB_HEADER_LENGTH + 16
    private static final int sequence = 0;
    private final byte[] metaBytes;
    private final int dateLen;
    private final long offset;
    private final ByteBuffer dataBuff;

    public LobPutRequest(BSONObject metaObj, byte[] data) {
        if (metaObj == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The meta object is null");
        }

        opCode = MsgOpCode.LOB_PUT_REQ;
        length = FIXED_LENGTH;

        metaBytes = Helper.encodeBSONObj(metaObj);
        bsonLength = metaBytes.length;
        length += Helper.alignedSize(metaBytes.length);

        offset = 0;
        dateLen = data.length;
        dataBuff = ByteBuffer.wrap(data, 0, dateLen);
        length += Helper.alignedSize(data.length);
    }

    @Override
    protected void encodeLobBody(ByteBuffer out) {
        encodeBSONBytes(metaBytes, out);

        out.putInt(dateLen);
        out.putInt(sequence);
        out.putLong(offset);

        out.put(dataBuff);
        int paddingSize = Helper.alignedSize(dateLen) - dateLen;
        Helper.fillZero(out, paddingSize);
    }
}
