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
package com.sequoiadb.util;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

/**
 * @author Jacky Zhang
 *
 */
public class Helper {

    public static void addBytesToByteBuffer(ByteBuffer buffer, byte[] byteArray,
                                            int off, int len, int multipler) {
        if (off + len > byteArray.length) {
            throw new BaseException(SDBError.SDB_SYS, "off + len is more then byteArray.length");
        }
        int newLength = (len % multipler == 0) ? len
                : (len + multipler - len % multipler);
        int incLength = newLength - len;
        // check
        if (newLength > buffer.remaining()) {
            throw new BaseException(SDBError.SDB_SYS, String.format(
                    "buffer is too small, need %d bytes, but remaining is %d",
                    newLength, buffer.remaining()));
        }
        // put data
        buffer.put(byteArray, off, len);
        // align ByteBuffer
        alignByteBuffer(buffer, incLength);
    }

    public static void alignByteBuffer(ByteBuffer buffer, int inc) {
        if (inc > buffer.remaining()) {
            throw new BaseException(SDBError.SDB_SYS, "inc is more than the remaining in ByteBuffer");
        }
        if (inc > 0) {
            buffer.position(buffer.position() + inc);
        }
    }

    public static byte[] alignBytes(byte[] byteArray, int multipler) {
        byte[] retArr = null;
        if (multipler == 0) {
            retArr = new byte[0];
            return retArr;
        }
        int inLength = byteArray.length;
        int newLength = (inLength % multipler == 0) ? inLength
                : (inLength + multipler - inLength % multipler);
        int incLength = newLength - inLength;
        retArr = new byte[incLength];
        return retArr;
    }

    @Deprecated
    public static byte[] roundToMultipleX(byte[] byteArray, int multipler) {
        if (multipler == 0)
            return byteArray;

        int inLength = byteArray.length;
        int newLength = (inLength % multipler == 0) ? inLength
                : (inLength + multipler - inLength % multipler);
        // warning: data copy happen here
        ByteBuffer buf = ByteBuffer.allocate(newLength);
        buf.put(byteArray);

        return buf.array();
    }

    public static int roundToMultipleXLength(int inLength, int multipler) {
        if (multipler == 0)
            return inLength;

        return (inLength % multipler == 0) ? inLength
                : (inLength + multipler - inLength % multipler);
    }

    @Deprecated
    public static byte[] concatByteArray(List<byte[]> inByteArrayList)
            throws BaseException {
        if (inByteArrayList == null || inByteArrayList.size() == 0)
            return null;

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        try {
            for (int i = 0; i < inByteArrayList.size(); i++) {
                outputStream.write(inByteArrayList.get(i));
            }
            return outputStream.toByteArray();
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        }
    }

    public static List<byte[]> splitByteArray(byte[] inByteArray, int length) {
        if (inByteArray == null)
            return null;

        List<byte[]> rtnList = new ArrayList<byte[]>();
        if (length >= inByteArray.length) {
            rtnList.add(inByteArray);
            rtnList.add(null);
            return rtnList;
        }

        //byte[] firstPart = Arrays.copyOfRange(inByteArray, 0, length);
        //byte[] seconPart = Arrays.copyOfRange(inByteArray, length,
        //		inByteArray.length);

        byte[] firstPart = new byte[length];
        System.arraycopy(inByteArray, 0, firstPart, 0, length);
        byte[] seconPart = new byte[inByteArray.length - length];
        System.arraycopy(inByteArray, length, seconPart, 0, inByteArray.length - length);

        rtnList.add(firstPart);
        rtnList.add(seconPart);

        return rtnList;
    }

    public static int byteToInt(byte[] byteArray, boolean endianConvert) {
        if (byteArray == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        ByteBuffer bb = ByteBuffer.wrap(byteArray);
        if (endianConvert) {
            bb.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            bb.order(ByteOrder.BIG_ENDIAN);
        }

        return bb.getInt();
    }

    public static int byteToInt(byte[] array, int begin) {
        if (array == null)
            throw new BaseException(SDBError.SDB_INVALIDARG);
        ByteBuffer bb = ByteBuffer.allocate(4);
        int count = 0;
        while (count < 4) {
            bb.put(array[begin + count]);
            ++count;
        }
        return bb.getInt(0);
    }

    public static int byteToInt(byte[] array) {
        if (array == null)
            throw new BaseException(SDBError.SDB_INVALIDARG);
        ByteBuffer bb = ByteBuffer.allocate(4);
        int count = 0;
        while (count < 4) {
            bb.put(array[count]);
            ++count;
        }
        return bb.getInt(0);
    }

    public static byte[] intToByte(int num) {
        byte[] b = new byte[4];
        for (int i = 0; i < 4; i++) {
            b[i] = (byte) (num >>> (24 - i * 8));
        }
        return b;
    }

    public static long byteToLong(byte[] byteArray, boolean endianConvert) {
        if (byteArray == null || byteArray.length != 8) {
            throw new BaseException(SDBError.SDB_INVALIDARG);
        }

        ByteBuffer bb = ByteBuffer.wrap(byteArray);
        if (endianConvert) {
            bb.order(ByteOrder.LITTLE_ENDIAN);
        } else {
            bb.order(ByteOrder.BIG_ENDIAN);
        }
        return bb.getLong();
    }

}
