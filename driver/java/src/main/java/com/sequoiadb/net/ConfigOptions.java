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
 * @package com.sequoiadb.net;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
package com.sequoiadb.net;

import com.sequoiadb.exception.BaseException;

/**
 * @class ConfigOptions
 * @brief Database Connection Configuration Option
 */
public class ConfigOptions {
    private long maxAutoConnectRetryTime = 15000;
    private int connectTimeout = 10000;
    private int socketTimeout = 0;
    private boolean socketKeepAlive = false;
    private boolean useNagle = false;
    private boolean useSSL = false;

    /**
     * @param maxRetryTimeMillis the max auto connect retry time in milliseconds.
     * @fn void setMaxAutoConnectRetryTime(long maxRetryTimeMilli)
     * @brief Set the max auto connect retry time in milliseconds. Default to be 15,000ms.
     * when "connectTimeout" is set to 10,000ms(default value), the max number of retries is
     * ceiling("maxAutoConnectRetryTime" / "connectTimeout"), which is 2.
     */
    public void setMaxAutoConnectRetryTime(long maxRetryTimeMillis) {
        if (maxRetryTimeMillis < 0) {
            throw new BaseException("SDB_INVALIDARG", "should not less than 0");
        }
        this.maxAutoConnectRetryTime = maxRetryTimeMillis;
    }

    /**
     * @param connectTimeoutMillis The connection timeout in milliseconds. Default is 10,000ms.
     * @fn void setConnectTimeout(int connectTimeoutMilli)
     * @brief Set the connection timeout in milliseconds. A value of 0 means no timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     */
    public void setConnectTimeout(int connectTimeoutMillis) {
        if (connectTimeoutMillis < 0) {
            throw new BaseException("SDB_INVALIDARG", "should not less than 0");
        }
        this.connectTimeout = connectTimeoutMillis;
    }

    /**
     * @param socketTimeoutMillis The socket timeout in milliseconds. Default is 0ms and means no timeout.
     * @fn void setSocketTimeout(int socketTimeoutMilli)
     * @brief Get the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     */
    public void setSocketTimeout(int socketTimeoutMillis) {
        if (socketTimeoutMillis < 0) {
            throw new BaseException("SDB_INVALIDARG", "should not less than 0");
        }
        this.socketTimeout = socketTimeoutMillis;
    }

    /**
     * @param on whether keep-alive is enabled on each socket. Default is false.
     * @fn void setSocketKeepAlive(boolean on)
     * @brief This flag controls the socket keep alive feature that keeps a connection alive through firewalls {@link java.net.Socket#setKeepAlive(boolean)}
     */
    public void setSocketKeepAlive(boolean on) {
        this.socketKeepAlive = on;
    }

    /**
     * @param on <code>false</code> to enable TCP_NODELAY, default to be false and going to use enable TCP_NODELAY.
     * @fn void setUseNagle(boolean on)
     * @brief Set whether enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
     */
    public void setUseNagle(boolean on) {
        this.useNagle = on;
    }

    /**
     * @param on Default to be false.
     * @fn void setUseSSL(boolean on)
     * @brief Set whether use the SSL or not
     * @author David Li
     * @since 1.12
     */
    public void setUseSSL(boolean on) {
        this.useSSL = on;
    }

    /**
     * @return the max auto connect retry time in milliseconds.
     * @fn long getMaxAutoConnectRetryTime()
     * @brief Get the max auto connect retry time in milliseconds.
     */
    public long getMaxAutoConnectRetryTime() {
        return maxAutoConnectRetryTime;
    }

    /**
     * @return the socket connect timeout
     * @fn int getConnectTimeout()
     * @brief The connection timeout in milliseconds. A timeout of zero is interpreted as an infinite timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     * <p/>
     * Default is 10,000ms.
     */
    public int getConnectTimeout() {
        return connectTimeout;
    }

    /**
     * @return the socket timeout
     * @fn int getSocketTimeout()
     * @brief Get the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     * <p/>
     * Default is 0ms and means no timeout.
     */
    public int getSocketTimeout() {
        return socketTimeout;
    }

    /**
     * @return the status of setting
     * @fn boolean getSocketKeepAlive()
     * @brief Get whether the socket keeps alive or not
     */
    public boolean getSocketKeepAlive() {
        return socketKeepAlive;
    }

    /**
     * @return the status of setting
     * @fn boolean getUseNagle()
     * @brief Get whether use the Nagle Algorithm or not
     */
    public boolean getUseNagle() {
        return useNagle;
    }

    /**
     * @return the status of setting
     * @fn boolean getUseSSL()
     * @brief Get whether use the SSL or not
     * @author David Li
     * @since 1.12
     */
    public boolean getUseSSL() {
        return useSSL;
    }

}
