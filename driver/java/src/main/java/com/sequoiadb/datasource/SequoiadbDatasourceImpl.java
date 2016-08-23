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
 * @package com.sequoiadb.datasource;
 * @brief SequoiaDB Data Source
 * @author tanzhaobo
 */
package com.sequoiadb.datasource;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.net.ConfigOptions;

/**
 * @class SequoiadbDatasourceImpl
 * @brief The implements for SequoiaDB data source
 * @since v1.12.6 & v2.2
 */
public class SequoiadbDatasourceImpl
{	
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
	private Object _createConnSingal = new Object();
	// for creating connections
	private String _username = null;
	private String _password = null;
	private ConfigOptions _nwOpt = null;
	private DatasourceOptions _dsOpt = null;
	private long _currentSequenceNumber = 0;
	// for  thread
	private ExecutorService _threadExec = null;
	private ScheduledExecutorService _timerExec = null;
	// for pool status
	private boolean _isDatasourceOn = false;
	private boolean _hasClosed = false;
	// for thread safe
	private ReentrantReadWriteLock _rwLock = new ReentrantReadWriteLock();
	private Object _objForReleaseConn = new Object();
	// for others
	private Random _rand = new Random(47);
	private double MULTIPLE = 1.2;
	
	/// when client program finish running, 
	/// this task will be executed
	class ExitClearUpTask extends Thread {
	    public void run() {
	    	try {
	    		close();
	    	} catch(Exception e){
	    		// do nothing
	    	}
	    }
	}

	class CreateConnectionTask implements Runnable {
		public void run() {
			try {
				while(!Thread.interrupted()) {
					synchronized(_createConnSingal) {
						_createConnSingal.wait();
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
			} catch(InterruptedException e) {
				// do nothing
			}
		}
	}
	
	class DestroyConnectionTask implements Runnable {
		public void run() {
			try {
				while(!Thread.interrupted()) {
					Sequoiadb sdb = _destroyConnQueue.take();
					try {
						sdb.disconnect();
					} catch(BaseException e){
						continue;
					}
				}
			} catch(InterruptedException e) {
				try {
					Sequoiadb[] arr = (Sequoiadb[])_destroyConnQueue.toArray();
					for(Sequoiadb db : arr) {
						try {db.disconnect();}catch(Exception ex){}
					}
				} catch(Exception exp) {
				}
			}
		}
	}
	
	class CheckConnectionTask implements Runnable {
		@Override
		public void run() {
			Lock wlock = _rwLock.readLock();
			wlock.lock();
			try {
				if (Thread.interrupted()) {
					return;
				}
			    if (_hasClosed) {
			        return;
			    }
				if (false == _isDatasourceOn) {
					return ;
				}
				// check keep alive timeout
				if (_dsOpt.getKeepAliveTimeout() > 0) {
					long lastTime = 0;
					long currentTime = System.currentTimeMillis();
					Iterator<Pair> itr = _idleConnPool.getIterator();
					List<Pair> list = new ArrayList<Pair>();
					while(itr.hasNext()) {
						Pair pair = itr.next();
						Sequoiadb sdb = pair.second();
						lastTime = sdb.getConnection().getLastUseTime(); 
						if (currentTime - lastTime + MULTIPLE*_dsOpt.getCheckInterval() >= _dsOpt.getKeepAliveTimeout()) {
							list.add(pair);
						}
					}
					itr = list.iterator();
					while(itr.hasNext()) {
						Pair pair = itr.next();
						ConnItem item = pair.first();
						Sequoiadb sdb = _idleConnPool.poll(item);
						_destroyConnQueue.add(sdb);
						// We drop the connections, and the strategy 
						// doesn't know this, so we need to tell it.
						_strategy.update(ItemStatus.IDLE, item, -1);
						// let the item return to _connItemMgr
						_connItemMgr.releaseItem(item);
					}
				}
			
				// keep the amount of idle connections less than maxIdleCount
				if (_idleConnPool.count() > _dsOpt.getMaxIdleCount()) {
					int destroyCount = _idleConnPool.count() - _dsOpt.getMaxIdleCount();
					_reduceIdleConnections(destroyCount);
				}
			} finally {
				wlock.unlock();
			}
		}
	}
	
	class RetrieveAddressTask implements Runnable {
		public void run() {
			Lock rlock = _rwLock.readLock(); rlock.lock();
			try {
				if (Thread.interrupted()) {
					return;
				}
		    	if (_hasClosed) {
		    		return;
		    	}
				if (0 == _abnormalAddrs.size()) {
					return;
				}
				Iterator<String> itr = _abnormalAddrs.iterator();
				ConfigOptions nwOpt = new ConfigOptions();
				String addr = "";
				nwOpt.setConnectTimeout(100); // 100ms
				nwOpt.setMaxAutoConnectRetryTime(0);
				while(itr.hasNext()) {
					try {
						addr = itr.next();
						@SuppressWarnings("unused")
						Sequoiadb sdb = new Sequoiadb(addr, _username, _password, nwOpt);
					} catch(BaseException e) {
						continue;
					}
					_abnormalAddrs.remove(addr);
					_normalAddrs.add(addr);
					_strategy.addAddress(addr);
				}
			} finally {
				rlock.unlock();
			}
		}
	}
	
	class SynchronizeAddressTask implements Runnable {
		List<String> _addrList = new ArrayList<String>();
		Sequoiadb _sdb = null;
		
		public void run() {
			Lock wlock = _rwLock.writeLock(); wlock.lock();
			try {
				if (Thread.interrupted()) {
					return;
				}
		    	if (_hasClosed) {
		    		return;
		    	}
		    	if (0 == _dsOpt.getSyncCoordInterval()) {
		    		return;
		    	}
				if (null == _sdb || !_sdb.isValid()) {
					_sdb = null;
					// we don't need "synchronized(_normalAddrs)" here, for 
					// "wlock" tell us that nobody is using "_normalAddrs"
					Iterator<String> itr = _normalAddrs.iterator();
					while(itr.hasNext()) {
						String addr = itr.next();
						try {
							_sdb = new Sequoiadb(addr, _username, _password, _nwOpt);
							break;
						} catch(BaseException e) {
							continue;
						}
					}
					if(null == _sdb) {
						// if we can't connect to database, let's return
						return;
					} 
				}
				// get the coord addresses for catalog
				try {
					_synchronizeCoordAddr(_sdb);
				} catch(Exception e) {
					// if we failed, let's return
					return;
				}
				// get the difference of coord addresses between catalog and local
				List<String> incList = new ArrayList<String>();
				List<String> decList = new ArrayList<String>();
				String addr = null;
				if (_addrList.size() > 0) {
					Iterator<String> itr = _normalAddrs.iterator();
					for(int i = 0; i < 2; i++, itr = _abnormalAddrs.iterator()) {
						while(itr.hasNext()) {
							addr = itr.next();
							if (!_addrList.contains(addr))
								decList.add(addr);
						}
					}
					itr = _addrList.iterator();
					while(itr.hasNext()) {
						addr = itr.next();
						if (!_normalAddrs.contains(addr) &&
								!_abnormalAddrs.contains(addr))
							incList.add(addr);
					}
				}
				// check whether we need to handle the difference or not
				if (incList.size() > 0) {
					// we are going to increase some coord addresses to local
					Iterator<String> itr = incList.iterator();
					while(itr.hasNext()) {
						addr = itr.next();
						_normalAddrs.add(addr);
						_strategy.addAddress(addr);
					}
				}
				if (decList.size() > 0) {
					// we are going to remove some coord addresses from local
					Iterator<String> itr = decList.iterator();
					while(itr.hasNext()) {
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
		
		private void _synchronizeCoordAddr(Sequoiadb sdb) {
			BSONObject condition = new BasicBSONObject();
			condition.put("GroupName", "SYSCoord");
			BSONObject select = new BasicBSONObject();
			select.put("Group.HostName","");
			select.put("Group.Service","");
			DBCursor cursor = sdb.getList(Sequoiadb.SDB_LIST_GROUPS, condition, select, null);
			BaseException exp = new BaseException("SDB_SYS", "Invalid coord information got from catalog");
			_addrList.clear();
			while(cursor.hasNext()) {
				BSONObject obj = cursor.getNext();
				BasicBSONList arr = (BasicBSONList)obj.get("Group");
				if (null == arr) throw exp;
				Object[] objArr = arr.toArray();
				for(int i = 0; i < objArr.length; i++) {
					BSONObject subObj = (BasicBSONObject)objArr[i];
					String hostName = (String)subObj.get("HostName");
					if (null == hostName) throw exp;
					String svcName = "";
					BasicBSONList subArr = (BasicBSONList)subObj.get("Service");
					if (null == subArr) throw exp;
					Object[] subObjArr = subArr.toArray();
					for(int j = 0; j < subObjArr.length; j++) {
						BSONObject subSubObj = (BSONObject)subObjArr[j];
						Integer type = (Integer)subSubObj.get("Type");
						if (null == type) throw exp;
						if (0 == type) {
							svcName = (String)subSubObj.get("Name");
							if (null == svcName) throw exp;
							String ip = _parseHostName(hostName);
							_addrList.add(ip + ":" + svcName);
							break;
						}
					}
				}
			}
		}
	}
	
	/**
	 * @fn SequoiadbDatasourceImpl(List<String> urls, String username, String password,
	 *		                   ConfigOptions nwOpt, DatasourceOptions dsOpt)
	 * @brief constructor.
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
	public SequoiadbDatasourceImpl(List<String> urls, String username, String password,
			ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {
		if (null == urls || 0 == urls.size())
			throw new BaseException("SDB_INVALIDARG", "coord addresses can't be empty or null");
		
        // init connection pool
		_init(urls, username, password, nwOpt, dsOpt);
	}

	/**
	 * @fn SequoiadbDatasourceImpl(String url, String username, String password,
	 *		                   DatasourceOptions dsOpt)
	 * @brief Constructor.
	 * @param url the address of coord, can't be empty or null, e.g."ubuntu1:11810"
	 * @param username the user name for logging sequoiadb
	 * @param password the password for logging sequoiadb
	 * @param dsOpt the options for connection pool
	 * @exception com.sequoiadb.exception.BaseException
	 */
	public SequoiadbDatasourceImpl(String url, String username, String password,
			DatasourceOptions dsOpt) throws BaseException {
		if (null == url || "" == url)
			throw new BaseException("SDB_INVALIDARG", "coord address can't be empty or null");
		ArrayList<String> urls = new ArrayList<String>();
		urls.add(url);
	    _init(urls, username, password, null, dsOpt);
	}
	
	/**
	 * @fn int getIdleConnNum()
	 * @brief Get the current idle connection amount.
	 */
	public int getIdleConnNum() {
		if (_idleConnPool == null)
			return 0;
		else
			return _idleConnPool.count();
	}
	
	/**
	 * @fn int getUsedConnNum()
	 * @brief Get the current used connection amount.
	 */
	public int getUsedConnNum() {
		if (_usedConnPool == null)
			return 0;
		else
			return _usedConnPool.count();
	}
	
	/**
	 * @fn int getNormalAddrNum()
	 * @brief Get the current normal address amount.
	 */
	public int getNormalAddrNum() {
		return _normalAddrs.size();
	}
	
	/**
	 * @fn int getAbnormalAddrNum()
	 * @brief Get the current abnormal address amount.
	 */
	public int getAbnormalAddrNum() {
		return _abnormalAddrs.size();
	}
	
	/**
	 * @fn int getLocalAddrNum()
	 * @brief Get the amount of local coord node address .
	 * @return the amount of local coord node address
	 * @note this API works only when the pool is enabled and the connect 
	 *       strategy is ConnectStrategy.LOCAL,
	 *       otherwise, return 0.  
	 * @exception com.sequoiadb.Exception.BaseException
	 * @since v1.12.6 & v2.2
	 */
	public int getLocalAddrNum() {
		return _localAddrs.size();
	}
	
	/**
	 * @fn void addCoord(String url)
	 * @brief Add coord address.
	 * @param url The address in format "192.168.20.168:11810"
	 * @exception com.sequoiadb.Exception.BaseException
	 */	
	public void addCoord(String url) throws BaseException {
		Lock rlock = _rwLock.readLock();
		rlock.lock(); 
		try {
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
			}
			if (null == url || "" == url) {
				throw new BaseException("SDB_INVALIDARG", "coord address can't be empty or null");
			}
			// parse coord address to the format "192.168.20.165:11810"
			String addr = _parseCoordAddr(url);
			// check whether the url exists in pool or not
			if (_normalAddrs.contains(addr) || 
				_abnormalAddrs.contains(addr)) {
				return;
			}
			// add to local
		    _normalAddrs.add(addr);
		     if (ConcreteLocalStrategy.isLocalAddress(addr, _localIPs))
		    	 _localAddrs.add(addr);
			// add to strategy
		    if (_isDatasourceOn) {
		    	_strategy.addAddress(addr);
		    }
		} finally {rlock.unlock();}
	}

	/**
	 * @fn void removeCoord(String url)
	 * @brief Remove coord address.
	 * @since v1.12.6 & v2.2
	 */
	public void removeCoord(String url) throws BaseException {
		Lock rlock = _rwLock.readLock(); 
		rlock.lock(); 
		try {
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
			}
			if (null == url) {
				throw new BaseException("SDB_INVALIDARG", "coord address can't be null");
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
		} finally {rlock.unlock();}
	}
	
	/**
	 * @fn DatasourceOptions getDatasourceOptions()
	 * @brief Get a copy of the connection pool options
	 * @return a copy of the connection pool options
	 * @throws BaseException
	 * @since v1.12.6 & v2.2
	 */
	public DatasourceOptions getDatasourceOptions() throws BaseException {
		Lock rlock = _rwLock.readLock(); 
		rlock.lock(); 
		try {
			return (DatasourceOptions) _dsOpt.clone();
		} catch (CloneNotSupportedException e) {
			throw new BaseException("SDB_SYS", "failed to clone connnection pool options");
		} finally {
			rlock.unlock();
		}
	}
	
	/**
	 * @fn void updateDatasourceOptions(DatasourceOptions dsOpt)
	 * @brief  Update connection pool options.
	 * @return dsOpt the newly connection pool for update
	 * @exception com.sequoiadb.Exception.BaseException
	 * @since v1.12.6 & v2.2
	 */
	public void updateDatasourceOptions(DatasourceOptions dsOpt) throws BaseException {
		Lock wlock = _rwLock.writeLock();
		wlock.lock(); 
		try {
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
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
				throw new BaseException("SDB_INVALIDARG", "failed to clone connection pool options");
			}
			// when data source is disable, return directly
			if (!_isDatasourceOn) {
				return;
			}
			// when _maxCount is set to 0, disable data source and return
			if (_dsOpt.getMaxCount() == 0) {
				disableDatasource();
				return;
			}
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
					throw new BaseException("SDB_SYS", "the item manager is null");
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
			}
			else if (previousCheckInterval != _dsOpt.getCheckInterval() ||
					previousSyncCoordInterval != _dsOpt.getSyncCoordInterval()) {
				_cancelTimer();
				_startTimer();
			}
		} finally {wlock.unlock();}
	}
    
	/**
	 * @fn void enableDatasource()
	 * @brief Enable data source.
	 * @note When maxCount is 0, set it to be the default value(500).
	 * @return void
	 * @exception com.sequoiadb.Exception.BaseException
	 * @exception InterruptedException
	 * @since v1.12.6 & v2.2  
	 */
	public void enableDatasource() {
		Lock wlock = _rwLock.writeLock();
		wlock.lock(); 
		try {
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
			}
			if (_isDatasourceOn) {
				return;
			}
			if (_dsOpt.getMaxCount() ==0)
				_dsOpt.setMaxCount(500);
			_enableDatasource(_dsOpt.getConnectStrategy());
		} finally {
			wlock.unlock();
		}
		return;
	}
	
	/**
	 * @fn void disableDatasource()
	 * @brief Disable data source.
	 * @return void
	 * @exception com.sequoiadb.Exception.BaseException
	 * @exception InterruptedException
	 * @note After disable data source, the pool will not manage 
	 *       the connections again. When a getting request comes, 
	 *       the pool build and return a connection; When a connection 
	 *       is put back, the pool disconnect it directly.
	 * @since v1.12.6 & v2.2
	 */
	public void disableDatasource() {
		Lock wlock = _rwLock.writeLock();
		wlock.lock(); 
		try {
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
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
		    _isDatasourceOn=false;
		} finally {
			wlock.unlock();
		}
		return;
	}
	
	/**
	 * @fn Sequoiadb getConnection()
	 * @brief Get a connection from current connection pool.
	 * @param timeout the time for waiting for connection in millisecond. 0 for waiting until a connection is available.
	 * @return Sequoiadb the connection for using
	 * @exception com.sequoiadb.Exception.BaseException
	 * @exception InterruptedException Actually, nothing happen. Throw this for compatibility reason.
	 * @note When the pool runs out, a request will wait up to 5 seconds. When time is up, if the pool 
	 * 		 still has no idle connection, it throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT". 
	 */
	public Sequoiadb getConnection() throws BaseException, InterruptedException {
		return getConnection(5000);
	}
	
	/**
	 * @fn Sequoiadb getConnection(long timeout)
	 * @brief  Get a connection from current connection pool.
	 * @param timeout the time for waiting for connection in millisecond. 0 for waiting until a connection is available.
	 * @return Sequoiadb the connection for using
	 * @exception com.sequoiadb.Exception.BaseException
	 *            when connection pool run out, throws BaseException with the type of "SDB_DRIVER_DS_RUNOUT"
	 * @exception InterruptedException Actually, nothing happen. Throw this for compatibility reason.
	 * @since v1.12.6 & v2.2
	 */
	public Sequoiadb getConnection(long timeout) throws BaseException, InterruptedException {
		Lock rlock = _rwLock.readLock();
		rlock.lock(); 
		try {
			if (timeout < 0) {
				throw new BaseException("SDB_INVALIDARG", "timeout should not be less than 0");
			}
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
			}
			// when the pool is disabled
			if (!_isDatasourceOn) {
				return _newConnByNormalAddr();
			}

			Sequoiadb sdb = null;
			ConnItem connItem = null;
			while(true) {
				connItem = _strategy.pollConnItem(Operation.GET);
				if (connItem != null) {
					// when we still have connection in idle pool,
					// get connection directly
					sdb = _idleConnPool.poll(connItem);
					// sanity check
					if (sdb == null) {
						// should never come here
						throw new BaseException("SDB_SYS", "point 1: error happen for getting connection");
					}
				} else {
					// when we have no connection in idle pool,
					// new a connection ,
					// and wait up thread to create connections
					connItem = _connItemMgr.getItem();
					if (connItem != null) {
						sdb = _newConnByNormalAddr();
						// sanity check
						if (sdb == null) {
							// should never come here
							throw new BaseException("SDB_SYS", "point 2: error happen for getting connection");
						}
						connItem.setAddr(sdb.getServerAddress().toString());
						synchronized(_createConnSingal) {
							_createConnSingal.notify();
						}
					} else {
						long restTime = timeout;
						long beginTime = 0;
						long endTime = 0;
						synchronized(this) {
							while((connItem = _strategy.pollConnItem(Operation.GET)) == null) {
								try {
									if (timeout != 0) {
										if (restTime <= 0)
											break;
										beginTime = System.currentTimeMillis();
										this.wait(restTime);
										endTime = System.currentTimeMillis();
										restTime -= (endTime - beginTime);
									} else {
										this.wait();
									}
								} catch(InterruptedException e) {
									if (timeout != 0) {
										endTime = System.currentTimeMillis();
										restTime -= (endTime - beginTime);
									}
									continue;
								}
							}
						}
						if (connItem == null) {
							throw new BaseException("SDB_DRIVER_DS_RUNOUT", "connection pool has run out");
						} else {
							sdb = _idleConnPool.poll(connItem);
							// sanity check
							if (sdb == null) {
								// should never come here
								throw new BaseException("SDB_SYS", "point 3: error happen for getting connection");
							}
						}
					}
				}
				// here we get the connection, let's check whether the connection is usable
				if (sdb.isClosed() || 
						(_dsOpt.getValidateConnection() && !sdb.isValid())) {
					// let the item go back to _connItemMgr and destroy 
					// the connection, then try again
					_connItemMgr.releaseItem(connItem);
					_destroyConnQueue.add(sdb);
					continue;
				} else {
					// stop looping
					break;
				}	
			}
			// insert the itemInfo and connection to used pool
			_usedConnPool.insert(connItem, sdb);
			// tell strategy used pool had add a connection
			_strategy.update(ItemStatus.USED, connItem, 1);

			return sdb;
		} finally {rlock.unlock();}
	}
	
	/**
	 * @fn void releaseConnection(Sequoiadb sdb)
	 * @brief Put the connection back to the connection pool.
	 * @param sdb the connection to come back, can't be null 
	 * @note When the data source is enable, we can't double release
	 *       one connection, and we can't offer a connection which is
	 *       not belong to the pool.
	 * @exception com.sequoiadb.Exception.BaseException
	 * @since v1.12.6 & v2.2
	 */
	public void releaseConnection(Sequoiadb sdb) throws BaseException {
		Lock rlock = _rwLock.readLock();
		rlock.lock(); 
		try {
			if (sdb == null) {
				throw new BaseException("SDB_INVALIDARG", "connection can't be null");
			}
			if (_hasClosed) {
				throw new BaseException("SDB_SYS", "connection pool has closed");
			}
			// in case the data source is disable
			if (!_isDatasourceOn) {
				// when we disable data source, we should try to remove 
				// the connections left in used connection pool
				synchronized(_objForReleaseConn) {
					if (_usedConnPool != null && _usedConnPool.contains(sdb)) {
						ConnItem item = _usedConnPool.poll(sdb);
						if (item == null)
							// multi-thread may let item to be null,   
							// and it should never happen
							throw new BaseException("SDB_SYS", 
									"Point 1: connection pool does't have item for the coming back connection");
						_connItemMgr.releaseItem(item);
					}
				}
				sdb.disconnect();
				return;
			}
			// in case the data source is enable
			ConnItem item = null;
			synchronized(_objForReleaseConn) {
				// if the busy pool contains this connection
				if (_usedConnPool.contains(sdb)) {
					// remove it from busy queue
					item = _usedConnPool.poll(sdb);
					if (item == null)
						throw new BaseException("SDB_SYS", 
								"Point 2: connection pool does't have item for the coming back connection");
				} else {
					// throw exception to let user know current connection does't contained in the pool
					throw new BaseException("SDB_INVALIDARG", 
							"the connection pool doesn't contain the offered connection");
				}
			}
			// tell the strategy there is a connection returning now
			_strategy.update(ItemStatus.USED, item, -1);
			// check whether the connection can put back to idle pool or not
			if (_connIsValid(item, sdb)) {
				// release the resource contains in connection
				sdb.releaseResource();
				// let the connection come back to connection pool
				_idleConnPool.insert(item, sdb);
				// tell the strategy one connection is add to idle pool now
				_strategy.update(ItemStatus.IDLE, item, 1);
				// notify the people who waits
				synchronized(this) {
					notifyAll();
				}
			} else {
				// let the item come back to item pool, and destroy the connection 
				_connItemMgr.releaseItem(item);
				_destroyConnQueue.add(sdb);
			}
		} finally {rlock.unlock();}
	}
	
	/**
	 * @fn void close(Sequoiadb sdb)
	 * @brief Put the connection back to the connection pool.
	 * @param sdb the connection to come back, can't be null 
	 * @note When the data source is enable, we can't double release
	 *       one connection, and we can't offer a connection which is
	 *       not belong to the pool.
	 * @exception com.sequoiadb.Exception.BaseException
	 * @deprecated
	 * @see releaseConnection, use releaseConnection instead
	 */
	public void close(Sequoiadb sdb) throws BaseException {
		releaseConnection(sdb);
	}
	
	/**
     * @fn void close()
     * @brief clean all resources of current connection pool
     */
	public void close() {
		Lock wlock = _rwLock.writeLock(); wlock.lock();
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
		    _isDatasourceOn=false;
		    _hasClosed=true;
		} finally {wlock.unlock();}
	}
	
	private void _init(List<String> urls, String username, String password, 
			ConfigOptions nwOpt, DatasourceOptions dsOpt) throws BaseException {        
		// set arguments
		for (String url : urls) {
			if (null != url && "" != url) {
				// parse coord address to the format "192.168.20.165:11810"
				String addr = _parseCoordAddr(url);
				if (!_normalAddrs.contains(addr))
					_normalAddrs.add(addr);
			}
		}
	    _username = (null == username) ? "" : username;
		_password = (null == password) ? "" : password;
		if (null == nwOpt) {
			ConfigOptions temp = new ConfigOptions();
			temp.setConnectTimeout(100);
			temp.setMaxAutoConnectRetryTime(0);
			_nwOpt = temp;
		} else {
			_nwOpt = nwOpt;
		}
		if (null == dsOpt) {
			_dsOpt = new DatasourceOptions();
		} else {
			try {
				_dsOpt = (DatasourceOptions)dsOpt.clone();
			} catch (CloneNotSupportedException e) {
				throw new BaseException("SDB_INVALIDARG", "failed to clone connection pool options");
			}
		}
		
		// pick up local coord address
		List<String> localIPList = ConcreteLocalStrategy.getNetCardIPs();
		_localIPs.addAll(localIPList);
		List<String> localCoordList = 
				ConcreteLocalStrategy.getLocalCoordIPs(_normalAddrs, localIPList);
		_localAddrs.addAll(localCoordList);
		
		// check options
		_checkDatasourceOptions(_dsOpt);
		
		// if connection is shutdown, return directly
		if (0 == _dsOpt.getMaxCount()) {
			_isDatasourceOn = false;
		} else {
			_enableDatasource(_dsOpt.getConnectStrategy());
		}
		// set a hook for closing all the connection, when object is destroyed
		Runtime.getRuntime().addShutdownHook(new ExitClearUpTask());
	}
	
	private void _startTimer() {
		_timerExec = Executors.newScheduledThreadPool(1);
		if (_dsOpt.getSyncCoordInterval() > 0)
			_timerExec.scheduleAtFixedRate(new SynchronizeAddressTask(), 0, _dsOpt.getSyncCoordInterval(), TimeUnit.MILLISECONDS);
		_timerExec.scheduleAtFixedRate(new CheckConnectionTask(), _dsOpt.getCheckInterval(), 
				_dsOpt.getCheckInterval(), TimeUnit.MILLISECONDS);
		_timerExec.scheduleAtFixedRate(new RetrieveAddressTask(), 60, 60, TimeUnit.SECONDS);
	}

	private void _cancelTimer() {
		_timerExec.shutdownNow();
	}
	
	private void _startThreads() {
		_threadExec = Executors.newCachedThreadPool();
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
		while(itr.hasNext()) {
			idleConnPairs.add(itr.next());
		}
		itr = _usedConnPool.getIterator();
		while(itr.hasNext()) {
			usedConnPairs.add(itr.next());
		}
		_strategy = _createStrategy(_dsOpt.getConnectStrategy());
		// here we don't need to offer abnormal address, for "RetrieveAddressTask"
		// will here us to add those addresses to strategy when those addresses
		// can be use again
		_strategy.init(_normalAddrs, idleConnPairs, usedConnPairs);
	}
	
	private void _closePoolConnections(IConnectionPool pool) {
		if (pool == null)
			return;
		// disconnect all the connections
		Iterator<Pair> iter = pool.getIterator();
		while(iter.hasNext()) {
		    Pair pair = iter.next();
		    Sequoiadb sdb = pair.second();
			try {
				sdb.disconnect();
			} catch(BaseException e) {
				// do nothing
			}
		}
		// clear them from the pool
		List<ConnItem> list = pool.clear();
		for(ConnItem item : list)
			_connItemMgr.releaseItem(item);
		// we are not clear the info in strategy,
		// for the strategy instance is abandoned,
		// and we will create a new one next time
	}
	
	private void _checkDatasourceOptions(DatasourceOptions newOpt) throws BaseException {
		if (null == newOpt) {
			throw new BaseException("SDB_INVALIDARG", "the offering datasource options can't be null");
		}

    	int deltaIncCount = newOpt.getDeltaIncCount();
    	int maxIdleCount = newOpt.getMaxIdleCount();
    	int maxCount = newOpt.getMaxCount();
    	int keepAliveTimeout = newOpt.getKeepAliveTimeout();
    	int checkInterval = newOpt.getCheckInterval();
    	int syncCoordInterval = newOpt.getSyncCoordInterval();
  	    	
		// 1. maxCount
		if (maxCount < 0)
			throw new BaseException("SDB_INVALIDARG", "maxCount can't be less then 0");
		
		// 2. deltaIncCount
		if (deltaIncCount <= 0)
			throw new BaseException("SDB_INVALIDARG", "deltaIncCount should be more then 0");
		
		// 3. maxIdleCount
		if (maxIdleCount < 0)
			throw new BaseException("SDB_INVALIDARG", "maxIdleCount can't be less then 0");
	
		// 4. keepAliveTimeout
		if (keepAliveTimeout < 0)
			throw new BaseException("SDB_INVALIDARG", "keepAliveTimeout can't be less than 0");

		// 5. checkInterval
		if (checkInterval <= 0)
			throw new BaseException("SDB_INVALIDARG", "checkInterval should be more than 0");
		if ( 0 != keepAliveTimeout && checkInterval >= keepAliveTimeout)
			throw new BaseException("SDB_INVALIDARG", "when keepAliveTimeout is not 0, checkInterval should be less than keepAliveTimeout" );

		// 6. syncCoordInterval
		if (syncCoordInterval < 0)
			throw new BaseException("SDB_INVALIDARG", "syncCoordInterval can't be less than 0");
			
    	if (0 != maxCount) {
			if (deltaIncCount > maxCount)
				throw new BaseException("SDB_INVALIDARG", "deltaIncCount can't be great then maxCount" );
			if (maxIdleCount > maxCount)
				throw new BaseException("SDB_INVALIDARG", "maxIdleCount can't be great then maxCount" );
    	}
	}
	
	/**
	 * @fn Sequoiadb _newConnByNormalAddr()
	 * @brief Get a connection directly.
	 * @return the newly build connection or null
	 * @exception com.sequoiadb.Exception.BaseException              
	 */	
    private Sequoiadb _newConnByNormalAddr() throws BaseException {
    	Sequoiadb sdb = null;
		String addr = null;
		while (true) {
			if (_isDatasourceOn) {
				addr = _strategy.getAddress();
			} else {
				synchronized(_normalAddrs) {
					int size = _normalAddrs.size();
					if (size > 0)
					addr = _normalAddrs.get(_rand.nextInt(size));
				}
			}
			if (addr != null) {
				try {
					sdb = new Sequoiadb(addr, _username, _password, _nwOpt);
					break;
				} catch (BaseException e) {
					String errType = e.getErrorType();
					if (errType.equals("SDB_NETWORK") || errType.equals("SDB_INVALIDARG") ||
						errType.equals("SDB_NET_CANNOT_CONNECT")) {
						_handleErrorAddr(addr);
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
    	if (null == sdb) {
    		throw new BaseException("SDB_SYS", "failed to create connection directly");
    	}
    	
		return sdb;
	}
	
	/**
	 * @fn Sequoiadb _newConnByAbnormalAddr()
	 * @brief try to get connection from abnormal address
	 * @return a sequoiadb connection
	 * @exception com.sequoiadb.Exception.BaseException              
	 */
	private Sequoiadb _newConnByAbnormalAddr() throws BaseException {
		Sequoiadb retConn = null;
		int retry = 3;
		while(retry-- > 0) {
			Iterator<String> itr = _abnormalAddrs.iterator();
			while(itr.hasNext()) {
				String addr = itr.next();
				try {
					retConn = new Sequoiadb(addr, _username, _password, _nwOpt);
				} catch(BaseException e) {
					continue;
				}
				_abnormalAddrs.remove(addr);
				_normalAddrs.add(addr);
				if (_isDatasourceOn) {
					_strategy.addAddress(addr);
				}
				break;
			}
			if (retConn != null) {
				break;
			}
		}
		if (retConn == null)
			throw new BaseException("SDB_INVALIDARG", "no available address for connection");
		return retConn;
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
		while(itr.hasNext()) {
			ConnItem item = itr.next();
			Sequoiadb sdb = _idleConnPool.poll(item);
			_destroyConnQueue.add(sdb);
			item.setAddr("");
			_connItemMgr.releaseItem(item);
		}
	}
	
	private void _createConnections() {
		int count = _dsOpt.getDeltaIncCount();
		while(count > 0) {
			// never let "sdb" defined out of current scope 
			Sequoiadb sdb = null;
			String addr = null;
			// get item for new connection
			ConnItem item = _connItemMgr.getItem();
			if (item == null) {
				// let's stop for no item for new connection
				break;
			}
			// create new connection
			while(true) {
				addr = _strategy.getAddress();
				if (addr == null) {
					// when have no address, we don't want to report any error message,
					// let it done by main branch.
					break;
				}
				// create connection
				try {
					sdb = new Sequoiadb(addr, _username, _password, _nwOpt);
					break;
				} catch(BaseException e) {
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
				} catch(Exception e) {
					// let's stop for another error
					break;
				}
			}
			// if we failed to create connection, 
			// let's release the item and then stop
			if (sdb == null) {
				_connItemMgr.releaseItem(item);
				break;
			}
			// when we create a connection, let's put it to idle pool
			item.setAddr(addr);
			// add to idle pool
			_idleConnPool.insert(item, sdb);
			// update info to strategy
			_strategy.update(ItemStatus.IDLE, item, 1);
			// let's continue
			count--;
		}
	}
	
	private boolean _connIsValid(ConnItem item, Sequoiadb sdb) {
		// check timeout or not
		if (0 != _dsOpt.getKeepAliveTimeout()) {
			long lastTime = sdb.getConnection().getLastUseTime();
			long currentTime = System.currentTimeMillis();
			if ((currentTime - lastTime) + MULTIPLE*_dsOpt.getCheckInterval() >= _dsOpt.getKeepAliveTimeout())
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
		switch(strategy) {
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
			while(itr.hasNext()) {
				list.add(itr.next().first());
			}
			_connItemMgr = new ConnectionItemMgr(_dsOpt.getMaxCount(), list);
			_currentSequenceNumber = _connItemMgr.getCurrentSequenceNumber();
		}
		// initialize strategy
		Iterator<String> itr = _normalAddrs.iterator();
		List<String> addrList = new ArrayList<String>();
		while(itr.hasNext()) {
			addrList.add(itr.next());
		}
		_strategy = _createStrategy(strategy);
		_strategy.init(addrList, null, null);
        // start timer
		_startTimer();
		// start back group thread
		_startThreads();
		_isDatasourceOn = true;
	}
	
	private void _reduceIdleConnections(int count) {
		while(count-- > 0) {
			// Once we poll a connection out by Operation.DELETE mode,
			// we don't need to update strategy again, pollConnItem 
			// had already help us to do this.
			ConnItem item = _strategy.pollConnItem(Operation.DELETE);
			if (item == null) {
				// Actually, should never come here.
				// When it happen, just let it go.
				break;
			}
			Sequoiadb sdb = _idleConnPool.poll(item);
			_destroyConnQueue.add(sdb);
			// let the item return to _connItemMgr
			_connItemMgr.releaseItem(item);
		}
	}
	
	private String _parseHostName(String hostName) {
		InetAddress ia = null;
		try {
			ia = InetAddress.getByName(hostName);
		} catch (UnknownHostException e) {
			throw new BaseException("SDB_SYS", "Failed to parse host name to ip for UnknownHostException");
		} catch (SecurityException e) {
			throw new BaseException("SDB_SYS", "Failed to parse host name to ip for SecurityException");
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
				throw new BaseException("SDB_INVALIDARG", "Point 1: invalid format coord address: " + coordAddr);
			host = tmp[0].trim();
			try {
				host = InetAddress.getByName(host).toString().split("/")[1];
			} catch (Exception e) {
				throw new BaseException("SDB_INVALIDARG", e);
			}
			port = Integer.parseInt(tmp[1].trim());
			retCoordAddr = host + ":" + port;
		} else {
			throw new BaseException("SDB_INVALIDARG", "Point 2: invalid format coord address: " + coordAddr);
		}
		return retCoordAddr;
	}
	
	protected void finalize() throws Throwable {
	    try {
	        close();
	    } catch(Exception e) {}
	    super.finalize();
	}
}

