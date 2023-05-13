/*
 * Copyright 2023 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.message.request;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.message.MsgOpCode;
import com.sequoiadb.util.Helper;
import org.bson.BSONObject;

import java.nio.ByteBuffer;

public class LobPutRequest extends LobRequest {

    private static final int FIXED_LENGTH = LOB_HEADER_LENGTH + 16; // 96
    private static final int sequence = 0;
    private static final long offset = 0;
    private final byte[] metaBytes;
    private final int dateLen;
    private final ByteBuffer dataBuff;

    public LobPutRequest(BSONObject metaObj, byte[] data, int offset, int len) {
        if (metaObj == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "The meta object is null");
        }

        opCode = MsgOpCode.LOB_PUT_REQ;
        length = FIXED_LENGTH;

        metaBytes = Helper.encodeBSONObj(metaObj);
        bsonLength = metaBytes.length;
        length += Helper.alignedSize(metaBytes.length);

        dateLen = len;
        dataBuff = ByteBuffer.wrap(data, offset, len);
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
