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
/**
 * SequoiaDB Data Source.
 * @package com.sequoiadb.datasource;
 * @author tanzhaobo
 */
package com.sequoiadb.base;

import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasourceImpl;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;

import java.util.List;

/**
 * SequoiaDB Data Source.
 */
public class SequoiadbDatasource extends SequoiadbDatasourceImpl {


    /**
     * Constructor.
     * @param urls the addresses of coord nodes, can't be null or empty,
     *        e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt the options for connection
     * @param dsOpt the options for connection pool
     * @note When offer several addresses for connection pool to use, if
     *       some of them are not available(invalid address, network error, coord shutdown,
     *       catalog replica group is not available), we will put these addresses
     *       into a queue, and check them periodically. If some of them is valid again,
     *       get them back for use. When connection pool get a unavailable address to connect,
     *       the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     *       can change both of the default value.
     * @see ConfigOptions
     * @see DatasourceOptions
     * @exception com.sequoiadb.exception.BaseException
     */
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    /**
     * Constructor.
     * @param urls the addresses of coord nodes, can't be null or empty,
     *        e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt the options for connection
     * @param dsOpt the options for connection pool
     * @note When offer several addresses for connection pool to use, if
     *       some of them are not available(invalid address, network error, coord shutdown,
     *       catalog replica group is not available), we will put these addresses
     *       into a queue, and check them periodically. If some of them is valid again,
     *       get them back for use. When connection pool get a unavailable address to connect,
     *       the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     *       can change both of the default value.
     * @see ConfigOptions
     * @see DatasourceOptions
     * @exception com.sequoiadb.exception.BaseException
     * @deprecated
     */
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               ConfigOptions nwOpt, SequoiadbOption dsOpt) throws BaseException {
        super(urls, username, password, nwOpt, dsOpt);
    }

    /**
     * Constructor.
     * @param url the address of coord, can't be empty or null, e.g."ubuntu1:11810".
     * @param username the user name for logging sequoiadb.
     * @param password the password for logging sequoiadb.
     * @param dsOpt the options for connection pool.
     * @exception com.sequoiadb.exception.BaseException
     */
    public SequoiadbDatasource(String url, String username, String password,
                               DatasourceOptions dsOpt) throws BaseException {
        super(url, username, password, dsOpt);
    }

    /**
     * Get the current idle connection amount.
     * @return The current idle connection amount.
     */
    public int getIdleConnNum() {
        return super.getIdleConnNum();
    }

    /**
     * Get the current used connection amount.
     * @return The current used connection amount.
     */
    public int getUsedConnNum() {
        return super.getUsedConnNum();
    }

    /**
     * Get the current normal address amount.
     * @return The current normal address amount.
     */
    public int getNormalAddrNum() {
        return super.getNormalAddrNum();
    }

    /**
     * Get the current abnormal address amount.
     * @return The current abnormal address amount.
     */
    public int getAbnormalAddrNum() {
        return super.getAbnormalAddrNum();
    }

    /**
     * Get the amount of local coord node address .
     * @return the amount of local coord node address.
     * @note this API works only when the pool is enabled and the connect
     *       strategy is ConnectStrategy.LOCAL,
     *       otherwise, return 0.
     * @exception com.sequoiadb.exception.BaseException
     * @since v1.12.6 & v2.2
     */
    public int getLocalAddrNum() {
        return super.getLocalAddrNum();
    }

    /**
     * Add coord address with the format "hostname:port" or "ip:port".
     * @exception com.sequoiadb.exception.BaseException
     */
    public void addCoord(String url) throws BaseException {
        super.addCoord(url);
    }

    /**
     * Remove coord address with the format "hostname:port" or "ip:port".
     * @since v1.12.6 & v2.2
     */
    public void removeCoord(String url) throws BaseException {
        super.removeCoord(url);
    }

    /**
     * Get a copy of the connection pool options.
     * @return a copy of the connection pool options.
     * @throws BaseException
     * @since v1.12.6 & v2.2
     */
    public DatasourceOptions getDatasourceOptions() throws BaseException {
        return super.getDatasourceOptions();
    }

    /**
     * Update connection pool options.
     * @return dsOpt the newly connection pool for update.
     * @exception com.sequoiadb.exception.BaseException
     * @since v1.12.6 & v2.2
     */
    public void updateDatasourceOptions(DatasourceOptions dsOpt) throws BaseException {
        super.updateDatasourceOptions(dsOpt);
    }

    /**
     * Enable data source.
     * @note When maxCount is 0, set it to be the default value(500).
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     * @exception InterruptedException
     * @since v1.12.6 & v2.2
     */
    public void enableDatasource() {
        super.enableDatasource();
    }

    /**
     * Disable data source.
     * @return void
     * @exception com.sequoiadb.exception.BaseException
     * @exception InterruptedException
     * @note After disable data source, the pool will not manage
     *       the connections again. When a getting request comes,
     *       the pool build and return a connection; When a connection
     *       is put back, the pool disconnect it directly.
     * @since v1.12.6 & v2.2
     */
    public void disableDatasource() {
        super.disableDatasource();
    }

    /**
     * Get a connection from current connection pool.
     * @return Sequoiadb the connection for using.
     * @exception com.sequoiadb.exception.BaseException
     * @exception InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     * @note When the pool runs out, a request will wait up to 5 seconds. When time is up, if the pool
     * 		 still has no idle connection, it throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT".
     */
    public Sequoiadb getConnection() throws BaseException, InterruptedException {
        return super.getConnection(5000);
    }

    /**
     * Get a connection from current connection pool.
     * @param timeout the time for waiting for connection in millisecond. 0 for waiting until a connection is available.
     * @return Sequoiadb the connection for using.
     * @exception com.sequoiadb.exception.BaseException
     *            when connection pool run out, throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT".
     * @exception InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     * @since v1.12.6 & v2.2
     */
    public Sequoiadb getConnection(long timeout) throws BaseException, InterruptedException {
        return super.getConnection(timeout);
    }

    /**
     * Put the connection back to the connection pool.
     * @param sdb the connection to come back, can't be null.
     * @note When the data source is enable, we can't double release
     *       one connection, and we can't offer a connection which is
     *       not belong to the pool.
     * @exception com.sequoiadb.exception.BaseException
     * @since v1.12.6 & v2.2
     */
    public void releaseConnection(Sequoiadb sdb) throws BaseException {
        super.releaseConnection(sdb);
    }

    /**
     * Put the connection back to the connection pool.
     * @param sdb the connection to come back, can't be null.
     * @note When the data source is enable, we can't double release
     *       one connection, and we can't offer a connection which is
     *       not belong to the pool.
     * @exception com.sequoiadb.exception.BaseException
     * @deprecated
     * @see #releaseConnection(Sequoiadb), use releaseConnection instead.
     */
    public void close(Sequoiadb sdb) throws BaseException {
        super.releaseConnection(sdb);
    }

    /**
     * Clean all resources of current connection pool.
     */
    public void close() {
        super.close();
    }


}

