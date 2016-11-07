/**
 *      Copyright (C) 2012 SequoiaDB Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
/**
 * @package com.sequoiadb.base;
 * @brief  this option of SequoiadbDatasource 
 * @author gaosj
 */
package com.sequoiadb.datasource;




/**
 * @class DatasourceOptions
 * @brief the options of data source
 * @since v1.12.6 & v2.2
 */
public class DatasourceOptions implements Cloneable
{
    private int _deltaIncCount = 10 ;
	private int _maxIdleCount = 10;
	private int _maxCount = 500;
	private int _keepAliveTimeout = 0 * 60 * 1000; // 0 min
    private int _checkInterval = 1 * 60 * 1000; // 1 min
    //private int _syncCoordInterval = 10 * 60 * 1000; // 10 min
    private int _syncCoordInterval = 0; // 0 min
	private boolean _validateConnection = false;
	private ConnectStrategy _connectStrategy = ConnectStrategy.BALANCE;

	/**
	 * @fn Object clone()
	 * @brief Close the current options. 
	 * @since v1.12.6 and v2.2
	 */
    public Object clone() throws CloneNotSupportedException {
		return super.clone();
    }
        
	/**
	 * @fn void setDeltaIncCount(int deltaIncCount)
	 * @brief Set the number of new connections to create once running out the
	 *        connection pool.
	 * @param deltaIncCount Default to be 10.
	 */
	public void setDeltaIncCount (int deltaIncCount) {
		_deltaIncCount = deltaIncCount ;
	}

	/**
	 * @fn void setMaxIdleCount(int maxIdleCount)
	 * @brief Set the max number of the idle connection left in connection 
	 *        pool after periodically cleaning.
	 * @param maxIdleCount Default to be 10.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setMaxIdleCount(int maxIdleCount) {
		_maxIdleCount = maxIdleCount;
	}
	
	/**
	 * @fn void setMaxCount(int maxCount)
	 * @brief Set the capacity of the connection pool. 
	 * @param  maxCount Default to be 500.
	 * @note When maxCount is set to 0, the connection pool will be disabled.
	 * @see Sequoiadb::disableDatasource() 
	 * @since v1.12.6 and 1.12.6
	 */
	public void setMaxCount(int maxCount) {
		_maxCount = maxCount;
	}
	
	/**
	 * @fn void setKeepAliveTimeout(int keepAliveTimeout)
	 * @brief Set the time in milliseconds for abandoning a connection which keep alive time is up.
	 *        If a connection has not be used(send and receive) for a long time(longer 
	 *        than "keepAliveTimeout"), the pool will not let it come back.
	 *        The pool will also clean this kind of idle connections in the pool periodically. 
	 * @param keepAliveTimeout Default to be 0ms, means not care about how long does a connection
	 *                         have not be used(send and receive).
	 * @note When "keepAliveTimeout" is not set to 0, it's better to set it 
	 *       greater than "checkInterval" twice over. Besides, unless you know what you needs,
	 *       never enable this option.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setKeepAliveTimeout(int keepAliveTimeout) {
		_keepAliveTimeout = keepAliveTimeout;
	}
	
	/**
	 * @fn void setCheckInterval(int checkInterval)
	 * @brief Set the checking interval in milliseconds. Every interval, 
	 *        the pool cleans all the idle connection which keep alive time is up,
	 *        and keeps the number of idle connection not more than "maxIdleCount". 
	 * @param checkInterval Default to be 1 * 60 * 1000ms. 
	 * @note When "keepAliveTimeout" is not be 0, "checkInterval" should be less than it. 
	 *       It's better to set "keepAliveTimeout" greater than "checkInterval" twice over.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setCheckInterval(int checkInterval) {
		_checkInterval = checkInterval ;
	}
	
	/**
	 * @fn void setSyncCoordInterval(int syncCoordInterval)
	 * @brief Set the interval for updating coord's addresses from catalog in milliseconds.
	 * @param syncCoordInterval Default to be 1 * 60 * 1000ms. 
	 * @note The updated coord addresses will cover the addresses in the pool. 
	 *       When "syncCoordInterval" is 0, the pool will stop updating coord's addresses from
	 *       catalog.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setSyncCoordInterval(int syncCoordInterval) {
		_syncCoordInterval = syncCoordInterval ;
	}

	/**
	 * @fn void setValidateConnection(boolean validateConnection)
	 * @brief When a idle connection is got out of pool, we need  
	 *        to validate whether it can be used or not.
	 * @param validateConnection Default to be false.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setValidateConnection(boolean validateConnection) {
		_validateConnection = validateConnection;
	}
	
	/**
	 * @fn void setConnectStrategy(ConnectStrategy strategy)
	 * @brief Set connection strategy.
	 * @param strategy Should one of the follow: 
	 *                 ConnectStrategy.SERIAL, 
	 *                 ConnectStrategy.RANDOM, 
	 *                 ConnectStrategy.LOCAL, 
	 *                 ConnectStrategy.BALANCE
	 * @note When choosing ConnectStrategy.LOCAL, if there have no local coord address, 
	 *       use other address instead.
	 * @since v1.12.6 and 1.12.6
	 */
	public void setConnectStrategy(ConnectStrategy strategy) {
		_connectStrategy = strategy;
	}	
	
	/**
	 * @fn int getDeltaIncCount()
	 * @brief Get the number of connections to create once running out the
	 *        connection pool.
	 * @setDeltaIncCount
	 */
	public int getDeltaIncCount () {
		return _deltaIncCount ;
	}
	
	/**
	 * @fn int getMaxIdleCount()
	 * @brief Get the max number of idle connection.
	 * @return The max number of idle connection after checking.
	 */
	public int getMaxIdleCount() {
		return _maxIdleCount;
	}
	
	/**
	 * @fn int getMaxCount()
	 * @brief Get the capacity of the pool.
	 * @return The capacity of the pool.
	 */
	public int getMaxCount() {
		return _maxCount;
	}
	
	/**
	 * @fn int getKeepAliveTimeout()
	 * @brief Get the setup time for abandoning a connection 
	 *        which has not been used for long time.
	 * @return the time
	 * @since v1.12.6 and 1.12.6
	 */
	public int getKeepAliveTimeout() {
		return _keepAliveTimeout;
	}
	
	/**
	 * @fn int getCheckInterval()
	 * @brief Get the interval for checking the idle connections periodically.
	 * @return the interval
	 * @since v1.12.6 and 1.12.6
	 */
	public int getCheckInterval() {
	   return _checkInterval ;
	}
	
	/**
	 * @fn int getSyncCoordInterval()
	 * @brief Get the interval for updating coord's addresses from catalog periodically.
	 * @return the interval
	 * @since v1.12.6 and 1.12.6
	 */
	public int getSyncCoordInterval() {
	   return _syncCoordInterval;
	}

	/**
	 * @fn boolean getValidateConnection()
	 * @brief Get whether to validate a 
	 *        connection which is got from the pool or not.
	 * @return true or false
	 * @since v1.12.6 and 1.12.6
	 */
	public boolean getValidateConnection() {
		return _validateConnection;
	}
	
	/**
	 * @fn ConnectStrategy getConnectStrategy()
	 * @brief Get the current strategy of creating connections.
	 * @return the strategy 
	 * @since v1.12.6 and 1.12.6
	 */
	public ConnectStrategy getConnectStrategy() {
		return _connectStrategy;
	}	
	
	
	
	/// the follow APIs are deprecated
	
	
	
	/**
	 * @fn void setInitConnectionNum(int initConnectionNum)
	 * @brief Set the initial number of connection.
	 * @param  initConnectionNum default to be 10
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             When the connection pool is enabled, the first time to get connection, 
	 *             the pool increases "deltaIncCount" number of connections. Used
	 *             setDeltaIncCount() instead.
	 * @see setDeltaIncCount()
	 */
	public void setInitConnectionNum(int initConnectionNum) {
	}
	
	/**
	 * @fn void setMaxIdeNum(int maxIdeNum)
	 * @brief Set the max number of the idle connection left in connection 
	 *        pool after periodically cleaning.
	 * @param maxIdeNum default to be 10
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used setMaxIdleCount() instead.
	 * @see setMaxIdleCount()
	 */
	public void setMaxIdeNum(int maxIdeNum) {
		setMaxIdleCount(maxIdeNum);
	}
	
	/**
	 * @fn void setMaxConnectionNum(int maxConnectionNum)
	 * @brief Set the max number of connection for use. When maxConnectionNum is 0,
	 *        the connection pool doesn't really work. In this situation, when request comes, 
	 *        it builds a connection and return it directly. When a connection goes back to pool, 
	 *        it disconnects the connection directly and will not put the connection back to pool. 
	 * @param  maxConnectionNum default to be 500 
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used setMaxCount() instead.
	 * @see setMaxCount()
	 */
	public void setMaxConnectionNum(int maxConnectionNum) {
		setMaxCount(maxConnectionNum);
	}
	
	/**
	 * @fn void setTimeout(int timeout)
	 * @brief Set the wait time in milliseconds. If the number of connection reaches
	 *        maxConnectionNum, the pool can't offer connection immediately, the
	 *        requests will be blocked to wait for a moment. When timeout, and there is
	 *        still no available connection, connection pool throws exception  
	 * @param timeout Default to be 5 * 1000ms.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used Sequoiadb.getConnection(int timeout) instead.
	 * @see Sequoiadb.getConnection(int timeout)
	 * @since v1.12.6 and 1.12.6
	 */	
	public void setTimeout(int timeout) {
	}
	
	/**
	 * @fn void setRecheckCyclePeriod(int recheckCyclePeriod)
	 * @brief Set the recheck cycle in milliseconds. In each cycle
	 *        connection pool cleans all the discardable connection,
	 *        and keep the number of valid connection not more than maxIdeNum. 
	 * @param recheckCyclePeriod recheckCyclePeriod should be less than abandonTime. Default to be 1 * 60 * 1000ms
	 * @note It's better to set abandonTime greater than recheckCyclePeriod twice over.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used setCheckInterval() instead.
	 * @see setCheckInterval()
	 */
	public void setRecheckCyclePeriod (int recheckCyclePeriod) {
		setCheckInterval(recheckCyclePeriod);
	}
	
	/**
	 * @fn void setRecaptureConnPeriod(int recaptureConnPeriod)
	 * @brief Set the time in milliseconds for getting back the useful address.
	 *        When offer several addresses for connection pool to use, if 
	 *        some of them are not available(invalid address, network error, coord shutdown,
	 *        catalog replica group is not available), we will put these addresses
	 *        into a queue, and check them periodically. If some of them is valid again,
	 *        get them back for use; 
	 * @param recaptureConnPeriod default to be 30 * 1000ms
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             The pool will test the invalid address automatically every 30 seconds.
	 */
	public void setRecaptureConnPeriod(int recaptureConnPeriod) {
	}
	
	/**
	 * @fn void setAbandonTime(int abandonTime)
	 * @brief Set the time in milliseconds for abandoning discardable connection.
	 *        If a connection has not be used for a long time(longer than abandonTime),
	 *        connection pool would not let it come back to pool. And it will clean this 
	 *        kind of connections in the pool periodically. 
	 * @param abandonTime default to be 10 * 60 * 1000ms
	 * @note It's better to set abandonTime greater than recheckCyclePeriod twice over.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used setKeepAliveTimeout() instead.
	 * @see setKeepAliveTimeout()
	 */
	public void setAbandonTime(int abandonTime) {
		setKeepAliveTimeout(abandonTime);
	}
	
	/**
	 * @fn int getInitConnectionNum()
	 * @brief Get the setup number of initial connection.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Return 0 instead.
	 */
	public int getInitConnectionNum() {
		return 0;
	}
	
	/**
	 * @fn int getMaxConnectionNum()
	 * @brief Get the max number of connection.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Return 0. Used getMaxCount() instead.
	 * @see getMaxCount()
	 */
	public int getMaxConnectionNum() {
		return getMaxCount();
	}
	
	/**
	 * @fn int getMaxIdeNum()
	 * @brief Get the max number of the idle connection.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Return 0. Used getMaxIdleCount() instead.
	 * @see getMaxIdleCount()
	 */
	public int getMaxIdeNum() {
		return getMaxIdleCount();
	}
	
	/**
	 * @fn int getAbandonTime()
	 * @brief Get the setup time for abandoning a connection 
	 *        which is not used for long time.
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used getKeepAliveTimeout() instead.
	 * @see getKeepAliveTimeout()
	 */
	public int getAbandonTime() {
		return getKeepAliveTimeout();
	}
	
	/**
	 * @fn int getRecheckCyclePeriod()
	 * @brief get the cycle for checking
	 * @deprecated Does not work since v1.12.6 and v2.2. 
	 *             Used getKeepAliveTimeout() instead.
	 * @see getCheckInterval()
	 */
	public int getRecheckCyclePeriod () {
	   return getCheckInterval() ;
	}
	
	/**
	 * @fn int getRecaptureConnPeriod()
	 * @brief Get the period for getting back useful addresses
	 * @deprecated Does not work since v1.12.6 and v2.2. Return 0.
	 */
	public int getRecaptureConnPeriod() {
		return 0;
	}
	
	/**
	 * @fn int getTimeout()
	 * @brief get the wait time. 
	 * @deprecated Does not work since v1.12.6 and v2.2. Return 0.
	 */
	public int getTimeout() {
		return 0;
	}
	
}
