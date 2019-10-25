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
package com.sequoiadb.net;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.util.Helper;
import com.sequoiadb.util.logger;

import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

/**
 * @author Jacky Zhang
 *
 */
public class ConnectionTCPImpl implements IConnection {

    private Socket clientSocket;
    private InputStream input;
    private OutputStream output;
    private ConfigOptions options;
    private ServerAddress hostAddress;
    private boolean endianConvert;
    private byte[] receive_buffer;
    final static private int DEF_BUFFER_LENGTH = 64 * 1024;
    private int REAL_BUFFER_LENGTH;
    private long lastUseTime;

    @Override
    public void setEndianConvert(boolean endianConvert) {
        this.endianConvert = endianConvert;
    }

    @Override
    public boolean isEndianConvert() {
        return endianConvert;
    }

    public long getLastUseTime() {
        return lastUseTime;
    }

    public ConnectionTCPImpl(ServerAddress addr, ConfigOptions options) {
        this.hostAddress = addr;
        this.options = options;
        endianConvert = false;
        receive_buffer = new byte[DEF_BUFFER_LENGTH];
        REAL_BUFFER_LENGTH = DEF_BUFFER_LENGTH;
    }

    static class SSLContextHelper {
        private volatile static SSLContext sslContext = null;

        public static SSLContext getSSLContext() throws BaseException {
            if (sslContext == null) {
                // init SSLContext with a TrustManager which doesn't check certificate
                synchronized (SSLContextHelper.class) {
                    // double check
                    if (sslContext == null) {
                        try {
                            X509TrustManager tm = new X509TrustManager() {
                                public X509Certificate[] getAcceptedIssuers() {
                                    return null;
                                }

                                public void checkClientTrusted(X509Certificate[] arg0, String arg1)
                                        throws CertificateException {
                                }

                                public void checkServerTrusted(X509Certificate[] arg0, String arg1)
                                        throws CertificateException {
                                }
                            };
                            SSLContext ctx = SSLContext.getInstance("SSL");
                            ctx.init(null, new TrustManager[]{tm}, null);
                            sslContext = ctx;
                        } catch (NoSuchAlgorithmException nsae) {
                            throw new BaseException(SDBError.SDB_NETWORK, nsae);
                        } catch (KeyManagementException kme) {
                            throw new BaseException(SDBError.SDB_NETWORK, kme);
                        }
                    }
                }
            }

            return sslContext;
        }
    }

    private void connect() throws BaseException {
        logger.getInstance().debug(0, "enter connect\n");
        String objectidentity = Integer.toString(hashCode());
        logger.getInstance().debug(0, "objidentity:" + objectidentity + "\n");
        if (clientSocket != null) {
            return;
        }

        long sleepTime = 100;
        long maxAutoConnectRetryTime = options.getMaxAutoConnectRetryTime();
        long start = System.currentTimeMillis();
        while (true) {
            BaseException lastError = null;
            InetSocketAddress addr = hostAddress.getHostAddress();
            try {
                if (options.getUseSSL()) {
                    clientSocket = SSLContextHelper.getSSLContext().getSocketFactory().createSocket();
                } else {
                    clientSocket = new Socket();
                }
                clientSocket.connect(addr, options.getConnectTimeout());
                clientSocket.setTcpNoDelay(!options.getUseNagle());
                clientSocket.setKeepAlive(options.getSocketKeepAlive());
                clientSocket.setSoTimeout(options.getSocketTimeout());
                input = new BufferedInputStream(clientSocket.getInputStream());
                output = clientSocket.getOutputStream();
                logger.getInstance().debug(0, "leave connect\n");
                return;
            } catch (IOException ioe) {
                lastError = new BaseException(SDBError.SDB_NETWORK, ioe);
                close();
            }
            // when we come here, it means network error, let's try until
            // maxAutoConnectRetryTime run out
            long executedTime = System.currentTimeMillis() - start;
            if (executedTime >= maxAutoConnectRetryTime) {
                throw lastError;
            }

            if (sleepTime + executedTime > maxAutoConnectRetryTime)
                sleepTime = maxAutoConnectRetryTime - executedTime;

            try {
                Thread.sleep(sleepTime);
            } catch (InterruptedException e) {
            }
            sleepTime *= 2;
        }
    }

    public void close() {
        logger.getInstance().debug(0, "enter close\n");
        String trace = logger.getInstance().getStackMsg();
        logger.getInstance().debug(0, trace);
        if (clientSocket != null) {
            try {
                clientSocket.close();
            } catch (Exception e) {
            } finally {
                receive_buffer = null;
                REAL_BUFFER_LENGTH = 0;
                input = null;
                output = null;
                clientSocket = null;
            }
        }
        logger.getInstance().debug(0, "leave close\n");
        logger.getInstance().save();
    }

    @Override
    public boolean isClosed() {
        if (clientSocket == null) {
            return true;
        }
        return clientSocket.isClosed();
    }

    /*
     * (non-Javadoc)
     *
     * @see com.sequoiadb.net.IConnection#changeConfigOptions(com.sequoiadb.net.
     * ConfigOptions)
     */
    @Override
    public void changeConfigOptions(ConfigOptions opts) throws BaseException {
        logger.getInstance().debug(0, "enter changeConfigOptions\n");
        this.options = opts;
        close();
        connect();
        logger.getInstance().debug(0, "leave changeConfigOptions\n");
    }


    /*
     * (non-Javadoc)
     *
     * @see com.sequoiadb.net.IConnection#receiveMessage()
     */
    @Override
    public ByteBuffer receiveMessage(boolean endianConvert) throws BaseException {
        // check
        if (this.isClosed()) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
        if (input == null) {
            throw new BaseException(SDBError.SDB_SYS, "input stream is null");
        }
        // update the time we use the socket
        lastUseTime = System.currentTimeMillis();
        try {
            // before use, check the buffer
            if (REAL_BUFFER_LENGTH < DEF_BUFFER_LENGTH) {
                receive_buffer = new byte[DEF_BUFFER_LENGTH];
                REAL_BUFFER_LENGTH = DEF_BUFFER_LENGTH;
            }
            input.mark(4);
            int rtn = 0;
            while (rtn < 4) {
                int retSize = input.read(receive_buffer, rtn, 4 - rtn);
                if (retSize == -1) {
                    throw new BaseException(SDBError.SDB_NETWORK, "failed to get the header of message");
                }
                rtn += retSize;
            }
            // get the length of the message
            int msgSize = Helper.byteToInt(receive_buffer, endianConvert);
            if (msgSize > REAL_BUFFER_LENGTH) {
                receive_buffer = new byte[msgSize];
                REAL_BUFFER_LENGTH = msgSize;
            }
            input.reset();
            rtn = 0;
            int retSize = 0;
            while (rtn < msgSize) {
                retSize = input.read(receive_buffer, rtn, msgSize - rtn);
                if (retSize == -1) {
                    throw new BaseException(SDBError.SDB_NETWORK, "failed to get the body of message");
                }
                rtn += retSize;
            }

            // if the byte count we read is not equal with the massage length, throw error
            if (rtn != msgSize) {
                throw new BaseException(SDBError.SDB_NETWORK, "unexpected length of message");
            }
            // wrap the receive byte into a byteBuffer and then return it
            ByteBuffer byteBuffer = ByteBuffer.wrap(receive_buffer, 0, msgSize);
            if (endianConvert) {
                // "endianConvert == true" means the bytes in byteBuffer
                // is in little-endian
                byteBuffer.order(ByteOrder.LITTLE_ENDIAN);
            } else {
                // "endianConvert == false" means the bytes in byteBuffer
                // is in big-endian
                byteBuffer.order(ByteOrder.BIG_ENDIAN);
            }
            logger.getInstance().debug(0, "leave receiveMessage\n");
            return byteBuffer;
        } catch(BaseException e) {
            close();
            throw e;
        } catch (Exception e) {
            close();
            throw new BaseException(SDBError.SDB_NETWORK, "receive message failed", e);
        }
    }

    @Override
    public byte[] receiveSysInfoMsg(int msgSize) throws BaseException {
        logger.getInstance().debug(0, "enter receiveSysInfoMsg\n");
        // check
        if (this.isClosed()) {
            throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
        if (input == null) {
            throw new BaseException(SDBError.SDB_SYS, "input stream is null");
        }

        byte[] buf = new byte[msgSize];
        try {
            int rtn = 0;
            int retSize = 0;
            while (rtn < msgSize) {
                retSize = input.read(buf, rtn, msgSize - rtn);
                if (retSize == -1) {
                    throw new BaseException(SDBError.SDB_NETWORK, "failed to get endian info");
                }
                rtn += retSize;
            }
            if (rtn != msgSize) {
                throw new BaseException(SDBError.SDB_NETWORK, "unexpected length of message");
            }
        } catch(BaseException e) {
            close();
            throw e;
        } catch (Exception e) {
            close();
            throw new BaseException(SDBError.SDB_NETWORK, "receive message failed", e);
        }
        return buf;
    }

    /*
     * (non-Javadoc)
     *
     * @see com.sequoiadb.net.IConnection#initialize()
     */
    @Override
    public void initialize() throws BaseException {
        connect();
    }

    @Override
    public void sendMessage(ByteBuffer buffer) throws BaseException {
        if (buffer == null) {
            throw new BaseException(SDBError.SDB_SYS, "send ByteBuffer is null");
        }
        if (buffer.hasArray()) {
            sendMessage(buffer.array(), 0 ,buffer.limit());
        } else {
            throw new BaseException(SDBError.SDB_SYS, "send ByteBuffer is not ok");
        }
    }

    @Override
    public void sendMessage(byte[] msg) throws BaseException {
        sendMessage(msg, 0, msg.length);
    }

    @Override
    public void sendMessage(byte[] msg, int off, int length) throws BaseException {
        logger.getInstance().debug(0, "enter sendMessage\n");
        if (this.isClosed()) {
        	throw new BaseException(SDBError.SDB_NOT_CONNECTED);
        }
        if (output != null) {
            try {
                output.write(msg, 0, length);
            } catch (IOException e) {
                close();
                throw new BaseException(SDBError.SDB_NETWORK, e);
            }
        } else {
            throw new BaseException(SDBError.SDB_SYS, "output stream is null");
        }
        logger.getInstance().debug(0, "leave sendMessage\n");
    }

    public void shrinkBuffer() {
        if (REAL_BUFFER_LENGTH != DEF_BUFFER_LENGTH) {
            receive_buffer = new byte[DEF_BUFFER_LENGTH];
            REAL_BUFFER_LENGTH = DEF_BUFFER_LENGTH;
        }
    }
}
