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
/**
 * @package com.sequoiadb.net;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
package com.sequoiadb.net;

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
     * @fn void setMaxAutoConnectRetryTime(long maxRetryTimeMilli)
     * @brief Set the max auto connect retry time in milliseconds. Default to be 15,000ms.
     *  when "connectTimeout" is set to 10,000ms(default value), the max number of retries is
     *  ceiling("maxAutoConnectRetryTime" / "connectTimeout"), which is 2.
     * @param maxRetryTimeMilli the max auto connect retry time in milliseconds.
     */
    public void setMaxAutoConnectRetryTime(long maxRetryTimeMilli) {
        this.maxAutoConnectRetryTime = maxRetryTimeMilli;
    }

    /**
     * @fn void setConnectTimeout(int connectTimeoutMilli)
     * @brief Set the connection timeout in milliseconds. A value of 0 means no timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     *
     * @param connectTimeoutMilli The connection timeout in milliseconds. Default is 10,000ms.
     */
    public void setConnectTimeout(int connectTimeoutMilli) {
        this.connectTimeout = connectTimeoutMilli;
    }

    /**
     * @fn void setSocketTimeout(int socketTimeoutMilli)
     * @brief Get the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     * @param socketTimeoutMilli The socket timeout in milliseconds. Default is 0ms and means no timeout.
     */
    public void setSocketTimeout(int socketTimeoutMilli) {
        this.socketTimeout = socketTimeoutMilli;
    }

    /**
     * @fn void setSocketKeepAlive(boolean on)
     * @brief This flag controls the socket keep alive feature that keeps a connection alive through firewalls {@link java.net.Socket#setKeepAlive(boolean)}
     * @param on whether keep-alive is enabled on each socket. Default is false.
     */
    public void setSocketKeepAlive(boolean on) {
        this.socketKeepAlive = on;
    }

    /**
     * @fn void setUseNagle(boolean on)
     * @brief Set whether enable/disable Nagle's algorithm(disable/enable TCP_NODELAY)
     * @param on <code>false</code> to enable TCP_NODELAY, default to be false and going to use enable TCP_NODELAY.
     */
    public void setUseNagle(boolean on) {
        this.useNagle = on;
    }

    /**
     * @fn void setUseSSL(boolean on)
     * @brief Set whether use the SSL or not
     * @param on Default to be false.
     * @author David Li
     * @since 1.12
     */
    public void setUseSSL(boolean on) {
        this.useSSL = on;
    }

    /**
     * @fn long getMaxAutoConnectRetryTime()
     * @brief Get the max auto connect retry time in milliseconds.
     * @return the max auto connect retry time in milliseconds.
     */
    public long getMaxAutoConnectRetryTime() {
        return maxAutoConnectRetryTime;
    }

    /**
     * @fn int getConnectTimeout()
     * @brief The connection timeout in milliseconds. A timeout of zero is interpreted as an infinite timeout.
     * It is used solely when establishing a new connection {@link java.net.Socket#connect(java.net.SocketAddress, int) }
     * <p/>
     * Default is 10,000ms.
     *
     * @return the socket connect timeout
     */
    public int getConnectTimeout() {
        return connectTimeout;
    }

    /**
     * @fn int getSocketTimeout()
     * @brief Get the socket timeout in milliseconds.
     * It is used for I/O socket read operations {@link java.net.Socket#setSoTimeout(int)}
     * <p/>
     * Default is 0ms and means no timeout.
     *
     * @return the socket timeout
     */
    public int getSocketTimeout() {
        return socketTimeout;
    }

    /**
     * @fn boolean getSocketKeepAlive()
     * @brief Get whether the socket keeps alive or not
     * @return the status of setting
     */
    public boolean getSocketKeepAlive() {
        return socketKeepAlive;
    }

    /**
     * @fn boolean getUseNagle()
     * @brief Get whether use the Nagle Algorithm or not
     * @return the status of setting
     */
    public boolean getUseNagle() {
        return useNagle;
    }

    /**
     * @fn boolean getUseSSL()
     * @brief Get whether use the SSL or not
     * @return the status of setting
     * @author David Li
     * @since 1.12
     */
    public boolean getUseSSL() {
        return useSSL;
    }

}
