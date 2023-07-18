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

package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;

import java.util.Objects;

/**
 * Token to describe the position to start or resume stream.
 */
public class StreamToken {
    private final String token;

    /**
     * Read the data stream starting from the latest position.
     */
    public StreamToken() {
        this.token = "";
    }

    /**
     * Read the data stream starting from the position specified by the token.
     */
    public StreamToken(String token) {
        if (token == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "token can not be null");
        }
        this.token = token;
    }

    public String getToken() {
        return token;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        StreamToken that = (StreamToken) o;
        return Objects.equals(token, that.token);
    }

    @Override
    public int hashCode() {
        return Objects.hash(token);
    }

    @Override
    public String toString() {
        return "StreamToken{" +
                "token='" + token + '\'' +
                '}';
    }
}
