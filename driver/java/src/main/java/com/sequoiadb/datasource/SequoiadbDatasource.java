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

package com.sequoiadb.datasource;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.base.ConfigOptions;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * The implements for SequoiaDB data source
 * @since v1.12.6 & v2.2
 */
public class SequoiadbDatasource {
    // for coord address
    private List<String> _normalAddrs = Collections.synchronizedList(new ArrayList<String>());
    private ConcurrentSkipListSet<String> _abnormalAddrs = new ConcurrentSkipListSet<String>();
    private ConcurrentSkipListSet<String> _localAddrs = new ConcurrentSkipListSet<String>();
    private List<String> _localIPs = Collections.synchronizedList(new ArrayList<String>());
    // for created connections
    private LinkedBlockingQueue<Sequoiadb> _destroyConnQueue = new LinkedBlockingQueue<Sequoiadb>();
    private IConnectionPool _idleConnPool = null;
    private IConnectionPool _usedConnPool = null;
    private IConnectStrategy _strategy = null;
    private ConnectionItemMgr _connItemMgr = null;
    private final Object _createConnSignal = new Object();
    // for creating connections
    private String _username = null;
    private String _password = null;
    private ConfigOptions _normalNwOpt = null;
    private ConfigOptions _abnormalNwOpt = null;
    private ConfigOptions _userNwOpt = null;
    private DatasourceOptions _dsOpt = null;
    private long _currentSequenceNumber = 0;
    // for  thread
    private ExecutorService _threadExec = null;
    private ScheduledExecutorService _timerExec = null;
    // for pool status
    private volatile boolean _isDatasourceOn = false;
    private volatile boolean _hasClosed = false;
    // for thread safe
    private ReentrantReadWriteLock _rwLock = new ReentrantReadWriteLock();
    private final Object _objForReleaseConn = new Object();
    // for error report
    private volatile BaseException _lastException;
    // for session
    private volatile BSONObject _sessionAttr = null;
    // for others
    private Random _rand = new Random(47);
    private double MULTIPLE = 1.5;
    private volatile int _preDeleteInterval = 0;
    private static final int _deleteInterval = 180000; // 3min

    // finalizer guardian
    @SuppressWarnings("unused")
    private final Object finalizerGuardian = new Object() {
        @Override
        protected void finalize() throws Throwable {
            try {
                close();
            } catch (Exception e) {
                // do nothing
            }
        }
    };

    /// when client program finish running,
    /// this task will be executed
    class ExitClearUpTask extends Thread {
        public void run() {
            try {
                close();
            } catch (Exception e) {
                // do nothing
            }
        }
    }

    class CreateConnectionTask implements Runnable {
        public void run() {
            try {
                while (!Thread.interrupted()) {
                    synchronized (_createConnSignal) {
                        _createConnSignal.wait();
                    }
                    Lock rlock = _rwLock.readLock();
                    rlock.lock();
                    try {
                        if (Thread.interrupted()) {
                            return;
                        } else {
                            _createConnections();
                        }
                    } finally {
                        rlock.unlock();
                    }
                }
            } catch (InterruptedException e) {
                // do nothing
            }
        }
    }

    class DestroyConnectionTask implements Runnable {
        public void run() {
            try {
                while (!Thread.interrupted()) {
                    Sequoiadb sdb = _destroyConnQueue.take();
                    try {
                        sdb.disconnect();
                    } catch (BaseException e) {
                        continue;
                    }
                }
            } catch (InterruptedException e) {
                try {
                    Sequoiadb[] arr = (Sequoiadb[]) _destroyConnQueue.toArray();
                    for (Sequoiadb db : arr) {
                        try {
                            db.disconnect();
                        } catch (Exception ex) {
                        }
                    }
                } catch (Exception exp) {
                }
            }
        }
    }

    class CheckConnectionTask implements Runnable {
        @Override
        public void run() {
            Lock wlock = _rwLock.writeLock();
            wlock.lock();
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                if (!_isDatasourceOn) {
                    return;
                }
                // check keep alive timeout
                if (_dsOpt.getKeepAliveTimeout() > 0) {
                    long lastTime = 0;
                    long currentTime = System.currentTimeMillis();
                    ConnItem connItem = null;
                    while ((connItem = _strategy.peekConnItemForDeleting()) != null) {
                        Sequoiadb sdb = _idleConnPool.peek(connItem);
                        lastTime = sdb.getLastUseTime();
                        if (currentTime - lastTime + _preDeleteInterval >= _dsOpt.getKeepAliveTimeout()) {
                            connItem = _strategy.pollConnItemForDeleting();
                            sdb = _idleConnPool.poll(connItem);
                            try {
                                _destroyConnQueue.add(sdb);
                            } finally {
                                _connItemMgr.releaseItem(connItem);
                            }
                        } else {
                            break;
                        }
                    }
                }
                // try to reduce the amount of idle connections
                if (_idleConnPool.count() > _dsOpt.getMaxIdleCount()) {
                    int destroyCount = _idleConnPool.count() - _dsOpt.getMaxIdleCount();
                    _reduceIdleConnections(destroyCount);
                }
                // when the number of idle connections in the pool is less than the minIdleCount,
                // we are going to create some connections
                if (_idleConnPool.count() < _dsOpt.getMinIdleCount()) {
                    synchronized (_createConnSignal) {
                        _createConnSignal.notify();
                    }
                }
            } finally {
                wlock.unlock();
            }
        }
    }

    class RetrieveAddressTask implements Runnable {
        public void run() {
            Lock rlock = _rwLock.readLock();
            rlock.lock();
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                if (_abnormalAddrs.size() == 0) {
                    return;
                }
                Iterator<String> abnormalAddrSetItr = _abnormalAddrs.iterator();
                String addr = "";
                while (abnormalAddrSetItr.hasNext()) {
                    addr = abnormalAddrSetItr.next();
                    try {
                        @SuppressWarnings("unused")
                        Sequoiadb sdb = new Sequoiadb(addr, _username, _password, _abnormalNwOpt);
                        try {
                            sdb.disconnect();
                        } catch (Exception e) {
                            // do nothing
                        }
                    } catch (Exception e) {
                        continue;
                    }
                    abnormalAddrSetItr.remove();
                    // add address to normal address set
                    synchronized (_normalAddrs) {
                        if (!_normalAddrs.contains(addr)) {
                            _normalAddrs.add(addr);
                        }
                    }
                    _strategy.addAddress(addr);
                }
            } finally {
                rlock.unlock();
            }
        }
    }

    class SynchronizeAddressTask implements Runnable {
        @Override
        public void run() {
            Lock wlock = _rwLock.writeLock();
            wlock.lock();
            try {
                if (Thread.interrupted()) {
                    return;
                }
                if (_hasClosed) {
                    return;
                }
                if (_dsOpt.getSyncCoordInterval() == 0) {
                    return;
                }
                // we don't need "synchronized(_normalAddrs)" here, for
                // "wlock" tell us that nobody is using "_normalAddrs"
                Iterator<String> itr = _normalAddrs.iterator();
                Sequoiadb sdb = null;
                while (itr.hasNext()) {
                    String addr = itr.next();
                    try {
                        sdb = new Sequoiadb(addr, _username, _password, _normalNwOpt);
                        break;
                    } catch (BaseException e) {
                        continue;
                    }
                }
                if (sdb == null) {
                    // if we can't connect to database, let's return
                    return;
                }
                // get the coord addresses from catalog
                List<String> cachedAddrList;
                try {
                    cachedAddrList = _synchronizeCoordAddr(sdb);
                } catch (Exception e) {
                    // if we failed, let's return
                    return;
                } finally {
                    try {
                        sdb.disconnect();
                    } catch (Exception e) {
                        // ignore
                    }
                    sdb = null;
                }
                // get the difference of coord addresses between catalog and local
                List<String> incList = new ArrayList<String>();
                List<String> decList = new ArrayList<String>();
                String addr = null;
                if (cachedAddrList != null && cachedAddrList.size() > 0) {
                    itr = _normalAddrs.iterator();
                    for (int i = 0; i < 2; i++, itr = _abnormalAddrs.iterator()) {
                        while (itr.hasNext()) {
                            addr = itr.next();
                            if (!cachedAddrList.contains(addr))
                                decList.add(addr);
                        }
                    }
                    itr = cachedAddrList.iterator();
                    while (itr.hasNext()) {
                        addr = itr.next();
                        if (!_normalAddrs.contains(addr) &&
                                !_abnormalAddrs.contains(addr))
                            incList.add(addr);
                    }
                }
                // check whether we need to handle the difference or not
                if (incList.size() > 0) {
                    // we are going to increase some coord addresses to local
                    itr = incList.iterator();
                    while (itr.hasNext()) {
                        addr = itr.next();
                        synchronized (_normalAddrs) {
                            if (!_normalAddrs.contains(addr)) {
                                _normalAddrs.add(addr);
                            }
                        }
                        _strategy.addAddress(addr);
                    }
                }
                if (decList.size() > 0) {
                    // we are going to remove some coord addresses from local
                    itr = decList.iterator();
                    while (itr.hasNext()) {
                        addr = itr.next();
                        _normalAddrs.remove(addr);
                        _abnormalAddrs.remove(addr);
                        _removeAddrInStrategy(addr);
                    }
                }
            } finally {
                wlock.unlock();
            }
        }

        private List<String> _synchronizeCoordAddr(Sequoiadb sdb) {
            List<String> addrList = new ArrayList<String>();
            BSONObject condition = new BasicBSONObject();
            condition.put("GroupName", "SYSCoord");
            BSONObject select = new BasicBSONObject();
            select.put("Group.HostName", "");
            select.put("Group.Service", "");
            DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS, condition, select, null);
            BaseException exp = new BaseException(SDBError.SDB_SYS, "Invalid coord information got from catalog");
            try {
                while (cursor.hasNext()) {
                    BSONObject obj = cursor.getNext();
                    BasicBSONList arr = (BasicBSONList) obj.get("Group");
                    if (arr == null) throw exp;
                    Object[] objArr = arr.toArray();
                    for (int i = 0; i < objArr.length; i++) {
                        BSONObject subObj = (BasicBSONObject) objArr[i];
                        String hostName = (String) subObj.get("HostName");
                        if (hostName == null || hostName.trim().isEmpty()) throw exp;
                        String svcName = "";
                        BasicBSONList subArr = (BasicBSONList) subObj.get("Service");
                        if (subArr == null) throw exp;
                        Object[] subObjArr = subArr.toArray();
                        for (int j = 0; j < subObjArr.length; j++) {
                            BSONObject subSubObj = (BSONObject) subObjArr[j];
                            Integer type = (Integer) subSubObj.get("Type");
                            if (type == null) throw exp;
                            if (type == 0) {
                                svcName = (String) subSubObj.get("Name");
                                if (svcName == null || svcName.trim().isEmpty()) throw exp;
                                String ip;
                                try {
                                    ip = _parseHostName(hostName.trim());
                                } catch (Exception e) {
                                    break;
                                }
                                addrList.add(ip + ":" + svcName.trim());
                                break;
                            }
                        }
                    }
                }
            } finally {
                cursor.close();
            }
            return addrList;
        }
    }

    /**
     * When offer several addresses for connection pool to use, if
     * some of them are not available(invalid address, network error, coord shutdown,
     * catalog replica group is not available), we will put these addresses
     * into a queue, and check them periodically. If some of them is valid again,
     * get them back for use. When connection pool get a unavailable address to connect,
     * the default timeout is 100ms, and default retry time is 0. Parameter nwOpt can
     * can change both of the default value.
     * @param urls     the addresses of coord nodes, can't be null or empty,
     *                 e.g."ubuntu1:11810","ubuntu2:11810",...
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param nwOpt    the options for connection
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     * @see ConfigOptions
     * @see DatasourceOptions
     */
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        if (null == urls || 0 == urls.size())
            throw new BaseException(SDBError.SDB_INVALIDARG, "coord addresses can't be empty or null");

        // init connection pool
        _init(urls, username, password, nwOpt, dsOpt);
    }

    /**
     * @deprecated Use com.sequoiadb.base.ConfigOptions instead of com.sequoiadb.net.ConfigOptions.
     * @see #SequoiadbDatasource(List, String, String, com.sequoiadb.base.ConfigOptions, DatasourceOptions)
     */
    @Deprecated
    public SequoiadbDatasource(List<String> urls, String username, String password,
                               com.sequoiadb.net.ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        this(urls, username, password, (ConfigOptions) nwOpt, dsOpt);
    }

    /**
     * @param url      the address of coord, can't be empty or null, e.g."ubuntu1:11810"
     * @param username the user name for logging sequoiadb
     * @param password the password for logging sequoiadb
     * @param dsOpt    the options for connection pool
     * @throws BaseException If error happens.
     */
    public SequoiadbDatasource(String url, String username, String password,
                               DatasourceOptions dsOpt) throws BaseException {
        if (url == null || url.isEmpty())
            throw new BaseException(SDBError.SDB_INVALIDARG, "coord address can't be empty or null");
        ArrayList<String> urls = new ArrayList<String>();
        urls.add(url);
        _init(urls, username, password, null, dsOpt);
    }

    /**
     * Get the current idle connection amount.
     */
    public int getIdleConnNum() {
        if (_idleConnPool == null)
            return 0;
        else
            return _idleConnPool.count();
    }

    /**
     * Get the current used connection amount.
     */
    public int getUsedConnNum() {
        if (_usedConnPool == null)
            return 0;
        else
            return _usedConnPool.count();
    }

    /**
     * Get the current normal address amount.
     */
    public int getNormalAddrNum() {
        return _normalAddrs.size();
    }

    /**
     * Get the current abnormal address amount.
     */
    public int getAbnormalAddrNum() {
        return _abnormalAddrs.size();
    }

    /**
     * Get the amount of local coord node address.
     * This method works only when the pool is enabled and the connect
     * strategy is ConnectStrategy.LOCAL, otherwise return 0.
     * @return the amount of local coord node address
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public int getLocalAddrNum() {
        return _localAddrs.size();
    }

    /**
     * Add coord address.
     * @param url The address in format "192.168.20.168:11810"
     * @throws BaseException If error happens.
     */
    public void addCoord(String url) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            if (null == url || "" == url) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "coord address can't be empty or null");
            }
            // parse coord address to the format "192.168.20.165:11810"
            String addr = _parseCoordAddr(url);
            // check whether the url exists in pool or not
            if (_normalAddrs.contains(addr) ||
                    _abnormalAddrs.contains(addr)) {
                return;
            }
            // add to local
            synchronized (_normalAddrs) {
                if (!_normalAddrs.contains(addr)) {
                    _normalAddrs.add(addr);
                }
            }
            if (ConcreteLocalStrategy.isLocalAddress(addr, _localIPs))
                _localAddrs.add(addr);
            // add to strategy
            if (_isDatasourceOn) {
                _strategy.addAddress(addr);
            }
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Remove coord address.
     * @since 2.2
     */
    public void removeCoord(String url) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            if (null == url) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "coord address can't be null");
            }
            // parse coord address to the format "192.168.20.165:11810"
            String addr = _parseCoordAddr(url);
            // remove from local
            _normalAddrs.remove(addr);
            _abnormalAddrs.remove(addr);
            _localAddrs.remove(addr);
            if (_isDatasourceOn) {
                // remove from strategy
                _removeAddrInStrategy(addr);
            }
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Get a copy of the connection pool options.
     * @return a copy of the connection pool options
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public DatasourceOptions getDatasourceOptions() throws BaseException {
        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            return (DatasourceOptions) _dsOpt.clone();
        } catch (CloneNotSupportedException e) {
            throw new BaseException(SDBError.SDB_SYS, "failed to clone connnection pool options");
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Update connection pool options.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void updateDatasourceOptions(DatasourceOptions dsOpt) throws BaseException {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            // check options
            _checkDatasourceOptions(dsOpt);
            // save previous values
            int previousMaxCount = _dsOpt.getMaxCount();
            int previousCheckInterval = _dsOpt.getCheckInterval();
            int previousSyncCoordInterval = _dsOpt.getSyncCoordInterval();
            ConnectStrategy previousStrategy = _dsOpt.getConnectStrategy();

            // reset options
            try {
                _dsOpt = (DatasourceOptions) dsOpt.clone();
            } catch (CloneNotSupportedException e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
            }
            // when data source is disable, return directly
            if (!_isDatasourceOn) {
                return;
            }

            // update network block timeout
            _normalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
            _abnormalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());

            // when _maxCount is set to 0, disable data source and return
            if (_dsOpt.getMaxCount() == 0) {
                disableDatasource();
                return;
            }
            _preDeleteInterval = (int) (_dsOpt.getCheckInterval() * MULTIPLE);
            // check need to adjust the capacity of connection pool or not.
            // when the data source is disable, we can't change the "_currentSequenceNumber"
            // to the value we want, that's a problem, so we will change "_currentSequenceNumber"
            // in "_enableDatasource()"
            if (previousMaxCount != _dsOpt.getMaxCount()) {
                // when "_enableDatasource()" is not called, "_connItemMgr" will be null
                if (_connItemMgr != null) {
                    _connItemMgr.resetCapacity(_dsOpt.getMaxCount());
                    if (_dsOpt.getMaxCount() < previousMaxCount) {
                        // make sure we have not get connection item more then
                        // _dsOpt.getMaxCount(), if so, let't decrease some in
                        // idle pool. But, we won't decrease any in used pool.
                        // When a connection is get out from used pool, we will
                        // check whether the item pool is full or not, if so, we
                        // won't let the connection and the item for it go to idle
                        // pool, we will destroy both out them.
                        int deltaNum = getIdleConnNum() + getUsedConnNum() - _dsOpt.getMaxCount();
                        int destroyNum = (deltaNum > getIdleConnNum()) ? getIdleConnNum() : deltaNum;
                        if (destroyNum > 0)
                            _reduceIdleConnections(destroyNum);
                        // update the version, so, all the outdated caching connections in used pool
                        // can not go back to idle pool any more.
                        _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
                    }
                } else {
                    // should never happen
                    throw new BaseException(SDBError.SDB_SYS, "the item manager is null");
                }
            }
            // check need to restart timer and threads or not
            if (previousStrategy != _dsOpt.getConnectStrategy()) {
                _cancelTimer();
                _cancelThreads();
                _changeStrategy();
                _startTimer();
                _startThreads();
                _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
            } else if (previousCheckInterval != _dsOpt.getCheckInterval() ||
                    previousSyncCoordInterval != _dsOpt.getSyncCoordInterval()) {
                _cancelTimer();
                _startTimer();
            }
        } finally {
            wlock.unlock();
        }
    }

    /**
     * Enable data source.
     * When maxCount is 0, set it to be the default value(500).
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void enableDatasource() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            if (_isDatasourceOn) {
                return;
            }
            if (_dsOpt.getMaxCount() == 0) {
                _dsOpt.setMaxCount(500);
            }
            _enableDatasource(_dsOpt.getConnectStrategy());
        } finally {
            wlock.unlock();
        }
        return;
    }

    /**
     * Disable the data source.
     * After disable data source, the pool will not manage
     * the connections again. When a getting request comes,
     * the pool build and return a connection; When a connection
     * is put back, the pool disconnect it directly.
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void disableDatasource() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            if (!_isDatasourceOn) {
                return;
            }
            // stop timer
            _cancelTimer();
            // stop threads
            _cancelThreads();
            // close the connections in idle pool
            _closePoolConnections(_idleConnPool);
            _isDatasourceOn = false;
        } finally {
            wlock.unlock();
        }
        return;
    }

    /**
     * Get a connection from current connection pool.
     * When the pool runs out, a request will wait up to 5 seconds. When time is up, if the pool
     * still has no idle connection, it throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT".
     * @return Sequoiadb the connection for using
     * @throws BaseException If error happens.
     * @throws InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     */
    public Sequoiadb getConnection() throws BaseException, InterruptedException {
        return getConnection(5000);
    }

    /**
     * Get a connection from current connection pool.
     * @param timeout the time for waiting for connection in millisecond. 0 for waiting until a connection is available.
     * @return Sequoiadb the connection for using
     * @throws BaseException when connection pool run out, throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT"
     * @throws InterruptedException Actually, nothing happen. Throw this for compatibility reason.
     * @since 2.2
     */
    public Sequoiadb getConnection(long timeout) throws BaseException, InterruptedException {
        if (timeout < 0) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "timeout should >= 0");
        }
        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            Sequoiadb sdb;
            ConnItem connItem;
            long restTime = timeout;
            while (true) {
                sdb = null;
                connItem = null;
                if (_hasClosed) {
                    throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
                }
                // when the pool is disabled
                if (!_isDatasourceOn) {
                    sdb = _newConnByNormalAddr();
                    // Use external network configuration when connection leaving the pool
                    updateConnConf(sdb, _userNwOpt);
                    return sdb;
                }
                if ((connItem = _strategy.pollConnItemForGetting()) != null) {
                    // when we still have connection in idle pool,
                    // get connection directly
                    sdb = _idleConnPool.poll(connItem);
                    // sanity check
                    if (sdb == null) {
                        _connItemMgr.releaseItem(connItem);
                        connItem = null;
                        // should never come here
                        throw new BaseException(SDBError.SDB_SYS, "no matching connection");
                    }
                } else if ((connItem = _connItemMgr.getItem()) != null) {
                    // when we have no connection in idle pool,
                    // new a connection, and wait up background thread to create connections
                    try {
                        sdb = _newConnByNormalAddr();
                    } catch (BaseException e) {
                        _connItemMgr.releaseItem(connItem);
                        connItem = null;
                        throw e;
                    }
                    // sanity check
                    if (sdb == null) {
                        _connItemMgr.releaseItem(connItem);
                        connItem = null;
                        // should never come here
                        throw new BaseException(SDBError.SDB_SYS, "create connection directly failed");
                    } else if (_sessionAttr != null) {
                        try {
                            sdb.setSessionAttr(_sessionAttr);
                        } catch (Exception e) {
                            _connItemMgr.releaseItem(connItem);
                            connItem = null;
                            _destroyConnQueue.add(sdb);
                            throw new BaseException(SDBError.SDB_SYS,
                                    String.format("failed to set the session attribute[%s]",
                                            _sessionAttr.toString()), e);
                        }
                    }
                    connItem.setAddr(sdb.getServerAddress().toString());
                    synchronized (_createConnSignal) {
                        _createConnSignal.notify();
                    }
                } else {
                    // when we can't get anything, let's wait
                    long beginTime = 0;
                    long endTime = 0;
                    // release the read lock before wait up
                    rlock.unlock();
                    try {
                        if (timeout != 0) {
                            if (restTime <= 0) {
                                // stop waiting
                                break;
                            }
                            beginTime = System.currentTimeMillis();
                            try {
                                synchronized (this) {
                                    this.wait(restTime);
                                }
                            } finally {
                                endTime = System.currentTimeMillis();
                                restTime -= (endTime - beginTime);
                            }
                            // even if the restTime is up, let it retry one more time
                            continue;
                        } else {
                            try {
                                synchronized (this) {
                                    // we have no double check of the connItem here,
                                    // so we can't use this.wait() here.
                                    // let it retry after a few seconds later
                                    this.wait(5000);
                                }
                            } finally {
                                continue;
                            }
                        }
                    } finally {
                        // let's get the read lock before going on
                        rlock.lock();
                    }
                }
                // here we get the connection, let's check whether the connection is usable
                if (sdb.isClosed() ||
                        (_dsOpt.getValidateConnection() && !sdb.isValid())) {
                    // let the item go back to _connItemMgr and destroy
                    // the connection, then try again
                    _connItemMgr.releaseItem(connItem);
                    connItem = null;
                    _destroyConnQueue.add(sdb);
                    sdb = null;
                    continue;
                } else {
                    // stop looping
                    break;
                }
            } // while(true)
            // when we can't get connection, try to report error
            if (connItem == null) {
                // make some debug info
                String detail = _getDataSourceSnapshot();
                // when the last connItem is hold by background creating thread,
                // and it failed to create the last connection, let's report network error
                if (getNormalAddrNum() == 0 && getUsedConnNum() < _dsOpt.getMaxCount()) {
                    BaseException exception = _getLastException();
                    String errMsg = "get connection failed, no available address for connection, " + detail;
                    if (exception != null) {
                        throw new BaseException(SDBError.SDB_NETWORK, errMsg, exception);
                    } else {
                        throw new BaseException(SDBError.SDB_NETWORK, errMsg);
                    }
                } else {
                    throw new BaseException(SDBError.SDB_DRIVER_DS_RUNOUT,
                            "the pool has run out of connections, " + detail);
                }
            } else {
                // insert the itemInfo and connection to used pool
                _usedConnPool.insert(connItem, sdb);
                // tell strategy used pool had add a connection
                _strategy.updateUsedConnItemCount(connItem, 1);
                // Use external network configuration when connection leaving the pool
                updateConnConf(sdb, _userNwOpt);
                return sdb;
            }
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Put the connection back to the connection pool.
     * When the data source is enable, we can't double release
     * one connection, and we can't offer a connection which is
     * not belong to the pool.
     * @param sdb the connection to come back, can't be null
     * @throws BaseException If error happens.
     * @since 2.2
     */
    public void releaseConnection(Sequoiadb sdb) throws BaseException {
        Lock rlock = _rwLock.readLock();
        rlock.lock();
        try {
            if (sdb == null) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "connection can't be null");
            }
            if (_hasClosed) {
                throw new BaseException(SDBError.SDB_SYS, "connection pool has closed");
            }
            // in case the data source is disable
            if (!_isDatasourceOn) {
                // when we disable data source, we should try to remove
                // the connections left in used connection pool
                synchronized (_objForReleaseConn) {
                    if (_usedConnPool != null && _usedConnPool.contains(sdb)) {
                        ConnItem item = _usedConnPool.poll(sdb);
                        if (item == null) {
                            // multi-thread may let item to be null, and it should never happen
                            throw new BaseException(SDBError.SDB_SYS,
                                    "the pool does't have item for the coming back connection");
                        }
                        _connItemMgr.releaseItem(item);
                        // Use internal network configuration when connection back to the pool
                        updateConnConf(sdb, _normalNwOpt);
                    }
                }
                try {
                    sdb.disconnect();
                } catch (Exception e) {
                    // do nothing
                }
                return;
            }
            // in case the data source is enable
            ConnItem item = null;
            synchronized (_objForReleaseConn) {
                // if the busy pool contains this connection
                if (_usedConnPool.contains(sdb)) {
                    // remove it from used queue
                    item = _usedConnPool.poll(sdb);
                    if (item == null) {
                        throw new BaseException(SDBError.SDB_SYS,
                                "the pool does not have item for the coming back connection");
                    }
                } else {
                    // throw exception to let user know current connection does't contained in the pool
                    throw new BaseException(SDBError.SDB_INVALIDARG,
                            "the connection pool doesn't contain the offered connection");
                }
                // Use internal network configuration when connection back to the pool
                updateConnConf(sdb, _normalNwOpt);
            }
            // we have decreased connection in used pool, let's update the strategy
            _strategy.updateUsedConnItemCount(item, -1);
            // check whether the connection can put back to idle pool or not
            if (_connIsValid(item, sdb)) {
                // let the connection come back to connection pool
                _idleConnPool.insert(item, sdb);
                // tell the strategy one connection is add to idle pool now
                _strategy.addConnItemAfterReleasing(item);
                // notify the people who waits
                synchronized (this) {
                    notifyAll();
                }
            } else {
                // let the item come back to item pool, and destroy the connection
                _connItemMgr.releaseItem(item);
                _destroyConnQueue.add(sdb);
                synchronized (this) {
                    notifyAll();
                }
            }
        } finally {
            rlock.unlock();
        }
    }

    /**
     * Put the connection back to the connection pool.
     * When the data source is enable, we can't double release
     * one connection, and we can't offer a connection which is
     * not belong to the pool.
     * @param sdb the connection to come back, can't be null
     * @throws BaseException If error happens.
     * @see #releaseConnection(Sequoiadb)
     * @deprecated use releaseConnection() instead
     */
    public void close(Sequoiadb sdb) throws BaseException {
        releaseConnection(sdb);
    }

    /**
     * Clean all resources of current connection pool.
     */
    public void close() {
        Lock wlock = _rwLock.writeLock();
        wlock.lock();
        try {
            if (_hasClosed) {
                return;
            }
            if (_isDatasourceOn) {
                _cancelTimer();
                _cancelThreads();
            }
            // close connections
            if (_idleConnPool != null)
                _closePoolConnections(_idleConnPool);
            if (_usedConnPool != null)
                _closePoolConnections(_usedConnPool);
            _isDatasourceOn = false;
            _hasClosed = true;
        } finally {
            wlock.unlock();
        }
    }

    private void _init(List<String> addrList, String username, String password,
                       ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
        // set arguments
        for (String elem : addrList) {
            if (elem != null && !elem.isEmpty()) {
                // parse coord address to the format "192.168.20.165:11810"
                String addr = _parseCoordAddr(elem);
                synchronized (_normalAddrs) {
                    if (!_normalAddrs.contains(addr)) {
                        _normalAddrs.add(addr);
                    }
                }
            }
        }
        _username = (username == null) ? "" : username;
        _password = (password == null) ? "" : password;

        if (dsOpt == null) {
            _dsOpt = new DatasourceOptions();
        } else {
            try {
                _dsOpt = (DatasourceOptions) dsOpt.clone();
            } catch (CloneNotSupportedException e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
            }
        }
        // check options
        _checkDatasourceOptions(_dsOpt);

        ConfigOptions temp = nwOpt;
        if (temp == null){
            temp = new ConfigOptions();
            temp.setConnectTimeout(100);
            temp.setMaxAutoConnectRetryTime(0);
        }
        try {
            // used inside of the connection pool
            _normalNwOpt = (ConfigOptions) temp.clone();
            _abnormalNwOpt = (ConfigOptions) temp.clone();
            // used outside of the connection pool
            _userNwOpt = (ConfigOptions) temp.clone();
        } catch (CloneNotSupportedException e) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "failed to clone connection pool options");
        }
        // set the network block timeout inside the connection pool
        // to avoid socket stuck due to network errors.
        _normalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
        _abnormalNwOpt.setSocketTimeout(_dsOpt.getNetworkBlockTimeout());
        // to reduce the connection time of abnormal address
        _abnormalNwOpt.setConnectTimeout(100);  // 100ms
        _abnormalNwOpt.setMaxAutoConnectRetryTime(0);  //0ms

        // pick up local coord address
        List<String> localIPList = ConcreteLocalStrategy.getNetCardIPs();
        _localIPs.addAll(localIPList);
        List<String> localCoordList =
                ConcreteLocalStrategy.getLocalCoordIPs(_normalAddrs, localIPList);
        _localAddrs.addAll(localCoordList);

        // if connection is shutdown, return directly
        if (_dsOpt.getMaxCount() == 0) {
            _isDatasourceOn = false;
        } else {
            _enableDatasource(_dsOpt.getConnectStrategy());
        }
        // set a hook for closing all the connection, when object is destroyed
        Runtime.getRuntime().addShutdownHook(new ExitClearUpTask());
    }

    private void _startTimer() {
        _timerExec = Executors.newScheduledThreadPool(1,
                new ThreadFactory() {
                    public Thread newThread(Runnable r) {
                        Thread t = Executors.defaultThreadFactory().newThread(r);
                        t.setDaemon(true);
                        return t;
                    }
                }
        );
        if (_dsOpt.getSyncCoordInterval() > 0) {
            _timerExec.scheduleAtFixedRate(new SynchronizeAddressTask(), 0, _dsOpt.getSyncCoordInterval(),
                    TimeUnit.MILLISECONDS);
        }
        _timerExec.scheduleAtFixedRate(new CheckConnectionTask(), _dsOpt.getCheckInterval(),
                _dsOpt.getCheckInterval(), TimeUnit.MILLISECONDS);
        _timerExec.scheduleAtFixedRate(new RetrieveAddressTask(), 60, 60, TimeUnit.SECONDS);
    }

    private void _cancelTimer() {
        _timerExec.shutdownNow();
    }

    private void _startThreads() {
        _threadExec = Executors.newCachedThreadPool(
                new ThreadFactory() {
                    public Thread newThread(Runnable r) {
                        Thread t = Executors.defaultThreadFactory().newThread(r);
                        t.setDaemon(true);
                        return t;
                    }
                }
        );
        _threadExec.execute(new CreateConnectionTask());
        _threadExec.execute(new DestroyConnectionTask());
        // stop adding task
        _threadExec.shutdown();
    }

    private void _cancelThreads() {
        _threadExec.shutdownNow();
    }

    private void _changeStrategy() {
        List<Pair> idleConnPairs = new ArrayList<Pair>();
        List<Pair> usedConnPairs = new ArrayList<Pair>();
        Iterator<Pair> itr = null;
        itr = _idleConnPool.getIterator();
        while (itr.hasNext()) {
            idleConnPairs.add(itr.next());
        }
        itr = _usedConnPool.getIterator();
        while (itr.hasNext()) {
            usedConnPairs.add(itr.next());
        }
        _strategy = _createStrategy(_dsOpt.getConnectStrategy());
        // here we don't need to offer abnormal address, for "RetrieveAddressTask"
        // will here us to add those addresses to strategy when those addresses
        // can be use again
        _strategy.init(_normalAddrs, idleConnPairs, usedConnPairs);
    }

    private void _closePoolConnections(IConnectionPool pool) {
        if (pool == null) {
            return;
        }
        // disconnect all the connections
        Iterator<Pair> iter = pool.getIterator();
        while (iter.hasNext()) {
            Pair pair = iter.next();
            Sequoiadb sdb = pair.second();
            try {
                sdb.disconnect();
            } catch (Exception e) {
                // do nothing
            }
        }
        // clear them from the pool
        List<ConnItem> list = pool.clear();
        for (ConnItem item : list)
            _connItemMgr.releaseItem(item);
        // we are not clear the info in strategy,
        // for the strategy instance is abandoned,
        // and we will create a new one next time
    }

    private void _checkDatasourceOptions(DatasourceOptions newOpt) throws BaseException {
        if (newOpt == null) {
            throw new BaseException(SDBError.SDB_INVALIDARG, "the offering datasource options can't be null");
        }

        int deltaIncCount = newOpt.getDeltaIncCount();
        int maxIdleCount = newOpt.getMaxIdleCount();
        int minIdleCount = newOpt.getMinIdleCount();
        int maxCount = newOpt.getMaxCount();
        int keepAliveTimeout = newOpt.getKeepAliveTimeout();
        int checkInterval = newOpt.getCheckInterval();
        int syncCoordInterval = newOpt.getSyncCoordInterval();
        List<Object> preferredInstanceList = newOpt.getPreferedInstanceObjects();
        String preferredInstanceMode = newOpt.getPreferedInstanceMode();
        int sessionTimeout = newOpt.getSessionTimeout();

        // 1. maxCount
        if (maxCount < 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "maxCount can't be less than 0");

        // 2. deltaIncCount
        if (deltaIncCount <= 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "deltaIncCount should be more than 0");

        // 3. maxIdleCount and minIdleCount
        if (maxIdleCount < 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "maxIdleCount can't be less than 0");
        if (minIdleCount < 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "minIdleCount can't be less than 0");
        if (minIdleCount > maxIdleCount)
            throw new BaseException(SDBError.SDB_INVALIDARG, "minIdleCount can't be more than maxIdleCount");

        // 4. keepAliveTimeout
        if (keepAliveTimeout < 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "keepAliveTimeout can't be less than 0");

        // 5. checkInterval
        if (checkInterval <= 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "checkInterval should be more than 0");
        if (0 != keepAliveTimeout && checkInterval >= keepAliveTimeout)
            throw new BaseException(SDBError.SDB_INVALIDARG, "when keepAliveTimeout is not 0, checkInterval should be less than keepAliveTimeout");

        // 6. syncCoordInterval
        if (syncCoordInterval < 0)
            throw new BaseException(SDBError.SDB_INVALIDARG, "syncCoordInterval can't be less than 0");

        if (maxCount != 0) {
            if (deltaIncCount > maxCount)
                throw new BaseException(SDBError.SDB_INVALIDARG, "deltaIncCount can't be more than maxCount");
            if (maxIdleCount > maxCount)
                throw new BaseException(SDBError.SDB_INVALIDARG, "maxIdleCount can't be more than maxCount");
        }

        // check arguments about session
        if (preferredInstanceList != null && preferredInstanceList.size() > 0) {
            // check elements of preferred instance
            for (Object obj : preferredInstanceList) {
                if (obj instanceof String) {
                    String s = (String) obj;
                    if (!"M".equals(s) && !"m".equals(s) &&
                            !"S".equals(s) && !"s".equals(s) &&
                            !"A".equals(s) && !"a".equals(s)) {
                        throw new BaseException(SDBError.SDB_INVALIDARG,
                                "the element of preferred instance should be 'M'/'S'/'A'/'m'/'s'/'a/['1','255'], but it is "
                                        + s);
                    }
                } else if (obj instanceof Integer) {
                    int i = (Integer) obj;
                    if (i <= 0 || i > 255) {
                        throw new BaseException(SDBError.SDB_INVALIDARG,
                                "the element of preferred instance should be 'M'/'S'/'A'/'m'/'s'/'a/['1','255'], but it is "
                                        + i);
                    }
                } else {
                    throw new BaseException(SDBError.SDB_INVALIDARG,
                            "the preferred instance should instance of int or String, but it is "
                                    + (obj == null ? null : obj.getClass()));
                }
            }
            // check preferred instance mode
            if (!DatasourceConstants.PREFERED_INSTANCE_MODE_ORDERED.equals(preferredInstanceMode) &&
                    !DatasourceConstants.PREFERED_INSTANCE_MODE_RANDON.equals(preferredInstanceMode)) {
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        String.format("the preferred instance mode should be '%s' or '%s', but it is %s",
                                DatasourceConstants.PREFERED_INSTANCE_MODE_ORDERED,
                                DatasourceConstants.PREFERED_INSTANCE_MODE_RANDON,
                                preferredInstanceMode));
            }
            // check session timeout
            if (sessionTimeout < -1) {
                throw new BaseException(SDBError.SDB_INVALIDARG,
                        "the session timeout can not less than -1");
            }
            _sessionAttr = newOpt.getSessionAttr();
        }

        // check networkBlockTimeout
        if (newOpt.getNetworkBlockTimeout() < 0){
            throw new BaseException(SDBError.SDB_INVALIDARG, "invalid networkBlockTimeout: " + newOpt.getNetworkBlockTimeout());
        }
    }

    private Sequoiadb _newConnByNormalAddr() throws BaseException {
        Sequoiadb sdb = null;
        String address = null;
        try {
            while (true) {
                // never forget to handle the situation of the datasourc is disable
                if (_isDatasourceOn) {
                    address = _strategy.getAddress();
                } else {
                    synchronized (_normalAddrs) {
                        int size = _normalAddrs.size();
                        if (size > 0) {
                            address = _normalAddrs.get(_rand.nextInt(size));
                        }
                    }
                }
                if (address != null) {
                    try {
                        sdb = new Sequoiadb(address, _username, _password, _normalNwOpt);
                        // when success, let's return the connection
                        break;
                    } catch (BaseException e) {
                        _setLastException(e);
                        String errType = e.getErrorType();
                        if (errType.equals("SDB_NETWORK") || errType.equals("SDB_INVALIDARG") ||
                                errType.equals("SDB_NET_CANNOT_CONNECT")) {
                            _handleErrorAddr(address);
                            address = null;
                            continue;
                        } else {
                            throw e;
                        }
                    }
                } else {
                    sdb = _newConnByAbnormalAddr();
                    break;
                }
            }

            // sanity check, should never hit here
            if (sdb == null) {
                throw new BaseException(SDBError.SDB_SYS, "failed to create connection directly");
            }
        } catch (BaseException e) {
            throw e;
        } catch (Exception e) {
            throw new BaseException(SDBError.SDB_SYS, e);
        }
        return sdb;
    }

    private Sequoiadb _newConnByAbnormalAddr() throws BaseException {
        Sequoiadb retConn = null;
        int retry = 3;
        while (retry-- > 0) {
            Iterator<String> itr = _abnormalAddrs.iterator();
            while (itr.hasNext()) {
                String addr = itr.next();
                try {
                    retConn = new Sequoiadb(addr, _username, _password, _abnormalNwOpt);
                } catch (Exception e) {
                    if (e instanceof BaseException) {
                        _setLastException((BaseException) e);
                    }
                    continue;
                }
                // TODO:
                // multi thread to do this, it's not the best way
                // to move a address from abnormal list ot normal list
                _abnormalAddrs.remove(addr);
                synchronized (_normalAddrs) {
                    if (!_normalAddrs.contains(addr)) {
                        _normalAddrs.add(addr);
                    }
                }
                if (_isDatasourceOn) {
                    _strategy.addAddress(addr);
                }
                break;
            }
            if (retConn != null) {
                break;
            }
        }
        if (retConn == null) {
            // make some debug info
            String detail = _getDataSourceSnapshot();
            BaseException exp = _getLastException();
            String errMsg = "no available address for connection, " + detail;
            if (exp != null) {
                throw new BaseException(SDBError.SDB_NETWORK, errMsg, exp);
            } else {
                throw new BaseException(SDBError.SDB_NETWORK, errMsg);
            }
        }
        return retConn;
    }

    private String _getDataSourceSnapshot() {
        String snapshot = String.format("[thread id: %d], total item: %d, idle item: %d, used item: %d, " +
                        "idle connections: %d, used connections: %d, " +
                        "normal addresses: %d, abnormal addresses: %d, local addresses: %d",
                Thread.currentThread().getId(),
                _connItemMgr.getCapacity(), _connItemMgr.getIdleItemNum(), _connItemMgr.getUsedItemNum(),
                _idleConnPool != null ? _idleConnPool.count() : null,
                _usedConnPool != null ? _usedConnPool.count() : null,
                getNormalAddrNum(), getAbnormalAddrNum(), getLocalAddrNum());
        return snapshot;
    }

    private void _setLastException(BaseException e) {
        _lastException = e;
    }

    private BaseException _getLastException() {
        BaseException exp = _lastException;
        return exp;
    }

    private void _handleErrorAddr(String addr) {
        _normalAddrs.remove(addr);
        _abnormalAddrs.add(addr);
        if (_isDatasourceOn) {
            _removeAddrInStrategy(addr);
        }
    }

    private void _removeAddrInStrategy(String addr) {
        List<ConnItem> list = _strategy.removeAddress(addr);
        Iterator<ConnItem> itr = list.iterator();
        while (itr.hasNext()) {
            ConnItem item = itr.next();
            Sequoiadb sdb = _idleConnPool.poll(item);
            _destroyConnQueue.add(sdb);
            item.setAddr("");
            _connItemMgr.releaseItem(item);
        }
    }

    private void _createConnections() {
        int createNum = _dsOpt.getDeltaIncCount() ;

        while (createNum > 0 && _idleConnPool.count() < _dsOpt.getMinIdleCount()) {
            // never let "sdb" defined out of current scope
            Sequoiadb sdb = null;
            String addr = null;
            // get item for new connection
            ConnItem connItem = _connItemMgr.getItem();
            if (connItem == null) {
                // let's stop for no item for new connection
                break;
            }
            // create new connection
            while (true) {
                addr = null;
                addr = _strategy.getAddress();
                if (addr == null) {
                    // when have no address, we don't want to report any error message,
                    // let it done by main branch.
                    break;
                }
                // create connection
                try {
                    sdb = new Sequoiadb(addr, _username, _password, _normalNwOpt);
                    break;
                } catch (BaseException e) {
                    _setLastException(e);
                    String errType = e.getErrorType();
                    if (errType.equals("SDB_NETWORK") || errType.equals("SDB_INVALIDARG") ||
                            errType.equals("SDB_NET_CANNOT_CONNECT")) {
                        // remove this address from normal address list
                        _handleErrorAddr(addr);
                        continue;
                    } else {
                        // let's stop for another error
                        break;
                    }
                } catch (Exception e) {
                    // let's stop for another error
                    break;
                }
            }
            // if we failed to create connection,
            // let's release the item and then stop
            if (sdb == null) {
                _connItemMgr.releaseItem(connItem);
                break;
            } else if (_sessionAttr != null) {
                try {
                    sdb.setSessionAttr(_sessionAttr);
                } catch (Exception e) {
                    _connItemMgr.releaseItem(connItem);
                    connItem = null;
                    _destroyConnQueue.add(sdb);
                    break;
                }
            }
            // when we create a connection, let's put it to idle pool
            connItem.setAddr(addr);
            // add to idle pool
            _idleConnPool.insert(connItem, sdb);
            // update info to strategy
            _strategy.addConnItemAfterCreating(connItem);
            // let's continue
            createNum--;
        }
    }

    private boolean _connIsValid(ConnItem item, Sequoiadb sdb) {
        // check the send/receive buffer size of the connection is out of the cache limit or not
        if (_dsOpt.getCacheLimit() > 0 && sdb.getCurrentCacheSize() > _dsOpt.getCacheLimit()) {
            return false;
        }

        // release the resource contains in connection
        try {
            sdb.releaseResource();
        } catch (Exception e) {
            try {
                sdb.disconnect();
            } catch (Exception ex) {
                // to nothing
            }
            return false;
        }

        // check timeout or not
        if (0 != _dsOpt.getKeepAliveTimeout()) {
            long lastTime = sdb.getLastUseTime();
            long currentTime = System.currentTimeMillis();
            if (currentTime - lastTime + _preDeleteInterval >= _dsOpt.getKeepAliveTimeout())
                return false;
        }
        // check version
        // for "_currentSequenceNumber" is pointed to the last
        // item which has been used, so, we need to use "<=".
        if (item.getSequenceNumber() <= _currentSequenceNumber) {
            return false;
        }
        return true;
    }

    private IConnectStrategy _createStrategy(ConnectStrategy strategy) {
        IConnectStrategy obj = null;
        switch (strategy) {
            case BALANCE:
                obj = new ConcreteBalanceStrategy();
                break;
            case SERIAL:
                obj = new ConcreteSerialStrategy();
                break;
            case RANDOM:
                obj = new ConcreteRandomStrategy();
                break;
            case LOCAL:
                obj = new ConcreteLocalStrategy();
                break;
            default:
                break;
        }
        return obj;
    }

    private void _enableDatasource(ConnectStrategy strategy) {
        _preDeleteInterval = (int) (_dsOpt.getCheckInterval() * MULTIPLE);
        // initialize idle connection pool
        _idleConnPool = new IdleConnectionPool();
        // initialize used connection pool
        if (_usedConnPool == null) {
            // when we disable data source, we won't clean
            // the connections in used pool.
            _usedConnPool = new UsedConnectionPool();
            // initialize connection item manager without anything
            _connItemMgr = new ConnectionItemMgr(_dsOpt.getMaxCount(), null);
        } else {
            // update the version, so, all the outdated caching connections in used pool
            // can not go back to idle pool any more.
            // initialize connection item manager with used items
            List<ConnItem> list = new ArrayList<ConnItem>();
            Iterator<Pair> itr = _usedConnPool.getIterator();
            while (itr.hasNext()) {
                list.add(itr.next().first());
            }
            _connItemMgr = new ConnectionItemMgr(_dsOpt.getMaxCount(), list);
            _currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
        }
        // initialize strategy
        _strategy = _createStrategy(strategy);
        _strategy.init(_normalAddrs, null, null);
        // start timer
        _startTimer();
        // start back group thread
        _startThreads();
        _isDatasourceOn = true;
    }

    private void _reduceIdleConnections(int count) {
        ConnItem connItem = null;
        long lastTime = 0;
        long currentTime = System.currentTimeMillis();
        while (count-- > 0 && (connItem = _strategy.peekConnItemForDeleting()) != null) {
            Sequoiadb sdb = _idleConnPool.peek(connItem);
            lastTime = sdb.getLastUseTime();
            if (currentTime - lastTime >= _deleteInterval) {
                connItem = _strategy.pollConnItemForDeleting();
                sdb = _idleConnPool.poll(connItem);
                try {
                    _destroyConnQueue.add(sdb);
                } finally {
                    _connItemMgr.releaseItem(connItem);
                }
            } else {
                break;
            }
        }
    }

    private void _reduceIdleConnections_bak(int count) {
        while (count-- > 0) {
            ConnItem item = _strategy.pollConnItemForDeleting();
            if (item == null) {
                // Actually, should never come here. When it happen, just let it go.
                break;
            }
            try {
                Sequoiadb sdb = _idleConnPool.poll(item);
                _destroyConnQueue.add(sdb);
            } finally {
                // release connItem
                _connItemMgr.releaseItem(item);
            }
        }
    }

    private String _parseHostName(String hostName) {
        InetAddress ia = null;
        try {
            ia = InetAddress.getByName(hostName);
        } catch (UnknownHostException e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to parse host name to ip for UnknownHostException", e);
        } catch (SecurityException e) {
            throw new BaseException(SDBError.SDB_SYS, "Failed to parse host name to ip for SecurityException", e);
        }
        return ia.getHostAddress();
    }

    private String _parseCoordAddr(String coordAddr) {
        String retCoordAddr = null;
        if (coordAddr.indexOf(":") > 0) {
            String host = "";
            int port = 0;
            String[] tmp = coordAddr.split(":");
            if (tmp.length < 2)
                throw new BaseException(SDBError.SDB_INVALIDARG, "Point 1: invalid format coord address: " + coordAddr);
            host = tmp[0].trim();
            try {
                host = InetAddress.getByName(host).toString().split("/")[1];
            } catch (Exception e) {
                throw new BaseException(SDBError.SDB_INVALIDARG, e);
            }
            port = Integer.parseInt(tmp[1].trim());
            retCoordAddr = host + ":" + port;
        } else {
            throw new BaseException(SDBError.SDB_INVALIDARG, "Point 2: invalid format coord address: " + coordAddr);
        }
        return retCoordAddr;
    }

    private void updateConnConf(Sequoiadb sdb, ConfigOptions config){
        // the difference of _normalNwOpt, _abnormalNwOpt and _userNwOpt:
        // 1. maxAutoConnectRetryTime, used to create connection, without updating
        // 2. connectTimeout, used to create connection, without updating
        // 3. socketTimeout, used for I/O socket read operations, need updating
        sdb.getConnProxy().setSoTimeout(config.getSocketTimeout());
    }
}

