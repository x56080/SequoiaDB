/*
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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/*
 * Auto-expand ByteBuffer
 * @author: David Li
 */
public class IOBuffer {
    private ByteBuffer buf;

    public IOBuffer(int capacity) {
        buf = ByteBuffer.allocate(capacity);
    }

    public ByteOrder order() {
        return buf.order();
    }

    public IOBuffer order(ByteOrder bo) {
        buf.order(bo);
        return this;
    }

    /**
     * Normalizes the specified capacity of the buffer to power of 2, which is
     * often helpful for optimal memory usage and performance.
     * If it is greater than or equal to Integer.MAX_VALUE, it returns Integer.MAX_VALUE.
     * If it is zero, it returns zero.
     */
    private static int normalizeCapacity(int capacity) {
        if (capacity < 0) {
            return Integer.MAX_VALUE;
        }

        int newCapacity = Integer.highestOneBit(capacity);
        newCapacity <<= (newCapacity < capacity ? 1 : 0);
        return newCapacity < 0 ? Integer.MAX_VALUE : newCapacity;
    }

    private void expand(int newCapacity) {
        if (newCapacity > capacity()) {
            // save the state
            int pos = position();
            ByteOrder bo = order();

            // reallocate
            ByteBuffer oldBuf = buf;
            ByteBuffer newBuf = ByteBuffer.allocate(newCapacity);
            oldBuf.clear();
            newBuf.put(oldBuf);
            buf = newBuf;

            // restore the state
            buf.position(pos);
            buf.order(bo);
        }
    }

    private void ensureCapacity(int expectedRemaining) {
        int pos = position();
        int end = pos + expectedRemaining;
        if (end > capacity()) {
            int newCapacity = normalizeCapacity(end);
            expand(newCapacity);
        }
    }

    public IOBuffer put(byte value) {
        ensureCapacity(1);
        buf.put(value);
        return this;
    }

    public IOBuffer putShort(short value) {
        ensureCapacity(2);
        buf.putShort(value);
        return this;
    }

    public IOBuffer putInt(int value) {
        ensureCapacity(4);
        buf.putInt(value);
        return this;
    }

    public IOBuffer putLong(long value) {
        ensureCapacity(8);
        buf.putLong(value);
        return this;
    }

    public IOBuffer putDouble(double value) {
        ensureCapacity(8);
        buf.putDouble(value);
        return this;
    }

    public IOBuffer put(byte[] src, int offset, int length) {
        ensureCapacity(length);
        buf.put(src, offset, length);
        return this;
    }

    public IOBuffer put(byte[] src) {
        put(src, 0, src.length);
        return this;
    }

    public int position() {
        return buf.position();
    }

    public IOBuffer position(int newPosition) {
        buf.position(newPosition);
        return this;
    }

    public int capacity() {
        return buf.capacity();
    }

    public byte[] array() {
        return buf.array();
    }

    public IOBuffer clear() {
        buf.clear();
        return this;
    }
}
