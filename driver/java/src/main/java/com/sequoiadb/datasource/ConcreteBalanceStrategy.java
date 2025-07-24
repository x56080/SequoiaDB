/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = ConcreteBalanceStrategy.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.TreeSet;
import java.util.concurrent.locks.ReentrantLock;

import com.sequoiadb.exception.BaseException;


class CountInfo implements Comparable<CountInfo>{
	private String _addr;
	private int _count;
	private boolean _available;
	
	public CountInfo(String addr, int count, boolean availdable) {
		_addr = addr;
		_count = count;
		_available = availdable;
	}
	
	public void setAddr(String addr) {
		_addr = addr;
	}
	
	public String getAddr() {
		return _addr;
	}
	
	public void setCount(int count) {
		_count = count;
	}
	
	public int getCount() {
		return _count;
	}
	
	public boolean getAvailable() {
		return _available;
	}
	
	public void setAvailable(boolean available) {
		_available = available;
	}
	
	private void _changeCount(int count) {
		_count += count;
	}
	
	public void increaseCount(int count) {
		_changeCount(count);
	}
	
	public void decreaseCount(int count) {
		_changeCount(count);
	}
	
	@Override
	public int compareTo(CountInfo other) {
		if (true == this._available && false == other._available){
			return -1;
		} else if (false == this._available && true == other._available) {
			return 1;
		} else {
				if (this._count != other._count) {
				return this._count - other._count;
			} else {
				return this._addr.compareTo(other._addr);
			}
		}
	}
}

public class ConcreteBalanceStrategy implements IConnectStrategy {
	
	private HashMap<String, LinkedList<ConnItem>> _idleConnItemMap = new HashMap<String, LinkedList<ConnItem>>();
	private HashMap<String, CountInfo> _countInfoMap = new HashMap<String, CountInfo>();
	private TreeSet<CountInfo> _countInfoSet = new TreeSet<CountInfo>();
	private ReentrantLock _lock = new ReentrantLock();
	private static CountInfo _dumpCountInfo = new CountInfo("", 0, false);
	
	@Override
	public void init(List<String> addresses, List<Pair> _idleConnPairs,
			List<Pair> _usedConnPairs) {
		// initialize info from giving addresses
		Iterator<String> itr1 = addresses.iterator();
		while(itr1.hasNext()) {
			String addr = itr1.next();
			if (!_idleConnItemMap.containsKey(addr)) {
				_idleConnItemMap.put(addr, new LinkedList<ConnItem>());
				CountInfo obj = new CountInfo(addr, 0, false);
				_countInfoMap.put(addr, obj);
				_countInfoSet.add(obj);
			}
		}
		
		// Initialize info from idle connections.
		// If a connection in idle pool has no information in _idleConnItemMap,
		// let's register this connection to _idleConnItemMap, _countInfoMap and
		// _countInfoSet.
		Iterator<Pair> itr2 = null;
		if (null != _idleConnPairs) {
			itr2 = _idleConnPairs.iterator();
			while(itr2.hasNext()) {
				Pair pair = itr2.next();
				ConnItem item = pair.first();
				String addr = item.getAddr();
				if (!_idleConnItemMap.containsKey(addr)) {
					LinkedList<ConnItem> list = new LinkedList<ConnItem>();
					_idleConnItemMap.put(addr, list);
					list.add(item);
					// we set this count info to be usable, for now we initialize from
					// idle connections, but, we don't know how many connections had been
					// used, so we it to be 0
					CountInfo info = new CountInfo(addr, 0, true);
					_countInfoMap.put(addr, info);
					_countInfoSet.add(info);
				} else {
					LinkedList<ConnItem> list = _idleConnItemMap.get(addr);
					list.add(item);
					CountInfo info = _countInfoMap.get(addr);
					if (false == info.getAvailable()) {
						_countInfoSet.remove(info);
						info.setAvailable(true);
						_countInfoSet.add(info);
					}
				}
			}
		}
		
		// Initialize info from used connections.
		// Notice that, we won't keep the info of connections whose address had been remove
		// from the pool. So, when _idleConnItemMap does't contain the address of a connection,
		// we will ignore that kind of connections.
		if (null != _usedConnPairs) {
			itr2 = _usedConnPairs.iterator();
			while(itr2.hasNext()) {
				Pair pair = itr2.next();
				ConnItem item = pair.first();
				String addr = item.getAddr();
				if (_idleConnItemMap.containsKey(addr)) {
					// should remove the original one then modify and insert again
					CountInfo info = _countInfoMap.get(addr);
					_countInfoSet.remove(info);
					info.increaseCount(1);
					_countInfoSet.add(info);
				} else {
					continue;
				}
			}
		}
	    
	}

	@Override
	public ConnItem pollConnItem(Operation opr) {
		ConnItem item = null;
		_lock.lock();
		try {
			while(true) {
				CountInfo info = null;
				String addr = null;
				/// get the countInfo we wanted
				if (Operation.GET == opr) {
					// get countInfo of connection which count is the least
					try {
						info = _countInfoSet.first();
					} catch(NoSuchElementException e) {
						info = null;
					}
				} else if (Operation.DELETE == opr) {
					info = _countInfoSet.lower(_dumpCountInfo);
				} else {
					throw new BaseException("SDB_SYS", "Invalid operation: " + opr);
				}
				// if we have no countInfo or all the countInfos are unavailable
				// let's return
				if (info == null || info.getAvailable() == false) {
					return null;
				}
				addr = info.getAddr();
				/// Now, let's get the ConnItem which associated with "addr".  
				LinkedList<ConnItem> list = _idleConnItemMap.get(addr);
				if (list != null) {
					item = list.poll();
				} else {
					// should never happen
					throw new BaseException("SDB_SYS", "Invalid state in strategy");
				}
				
				/// Check the connItem can be use or not.
				if (item == null) {
					// When address "addr" has no idle connection, we get another one.
					// But, before this, let's mark the countInfo of address "addr" to be unavailable.
					// And update this countInfo
					info = _countInfoMap.get(addr);
					_countInfoSet.remove(info);
					info.setAvailable(false);
					_countInfoSet.add(info);
					continue;
				} else {
					// when we get it, let's stop
					break;
				}
			}
		} finally {
			_lock.unlock();
		}
		// finish 
		return item;
	}

	@Override
	public String getAddress() {
		String addr = null;
		CountInfo info = null;
		_lock.lock(); 
		try {
			info = _countInfoSet.higher(_dumpCountInfo);
			if (null == info) {
				try {
					info = _countInfoSet.first();
				} catch(NoSuchElementException e) {
					// in this case, _countInfoSet is empty
					return null;
				}
			}
			addr = info.getAddr();
		} finally {
			_lock.unlock();
		}
		return addr;
	}

	/*
	 * only when the amount of connections in used pool or idle pool change, 
	 * we need to update
	 * */ 
	@Override
	public void update(ItemStatus status, ConnItem item, int change) {
		String addr = item.getAddr();
		CountInfo info = null;
		LinkedList<ConnItem> list = null;
		_lock.lock();
		try {
			if (ItemStatus.IDLE == status) {
				if (!_idleConnItemMap.containsKey(addr)) {
					// maybe the information of this address was remove by "removeAddress()"
					// so let's rebuild those information
					_restoreIdleConnItemInfo(addr);
				}
				if (change > 0) {
					/// in this case, we are adding connections to idle pool
					info = _countInfoMap.get(addr);				
					if (info == null) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point1: the pool has no information about address: " + addr);
					}
					// update the countInfo which is in the state of unavailable
					if (info.getAvailable() == false) {
						_countInfoSet.remove(info);
						info.setAvailable(true);
						_countInfoSet.add(info);
					}
	
					// push connItem into list
					list = _idleConnItemMap.get(addr);
					if (list == null) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point2: the pool has no information about address: " + addr);
					}
					list.add(item);
				} else if (change < 0) {
					/// in this case, we are removing connections from idle pool
					/// when we come here, we the CLEAN TASK is working.
					list = _idleConnItemMap.get(addr);
					if (list == null) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point3: the pool has no information about address: " + addr);
					}
					if (list.size() == 0) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point4: the pool has no information about address: " + addr);
					}
					if (list.remove(item) == false) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point5: the pool has no information about address: " + addr);
					}
					// when current list has not connItem any more, let's set current address unusable.
					if (list.size() == 0) {
						info = _countInfoMap.get(addr);
						_countInfoSet.remove(info);
						info.setAvailable(false);
						_countInfoSet.add(info);
					}
				} else {
					throw new BaseException("SDB_SYS", "Point1: invalid change in idle pool");
				}
			} else if (ItemStatus.USED == status) {
				// when _countInfoMap does not contain this address, 
				// this address may be remove by user. 
				// see "removeAddress" for more detail.
				if (_countInfoMap.containsKey(addr)) {
					info = _countInfoMap.get(addr);
					// the info may be removed when strategy removed address
					if (null == info) {
						// should never happen
						throw new BaseException("SDB_SYS", "Point6: the pool has no information about address: " + addr);
					}
					_countInfoSet.remove(info);
					if (change > 0) {
						info.increaseCount(change);
					} else if (change < 0) {
						info.decreaseCount(change);
					} else {
						throw new BaseException("SDB_SYS", "Point2: invalid change in idle pool");
					}
					_countInfoSet.add(info);
				}
			} else {
				// should never happen
				throw new BaseException("SDB_SYS", "Invalid item status: " + status);
			}
		} finally {
			_lock.unlock();
		}
	}

	@Override
	public void addAddress(String addr) {
		_lock.lock();
		try {
			List<ConnItem> list = _idleConnItemMap.get(addr);
			if (list == null) {
				// when we have no info about address "addr", let't prepare that
				_idleConnItemMap.put(addr, new LinkedList<ConnItem>());
				CountInfo info = new CountInfo(addr, 0, false);
				_countInfoMap.put(addr, info);
				_countInfoSet.add(info);
			}
		} finally {
			_lock.unlock();
		}
	}

	@Override
	public List<ConnItem> removeAddress(String addr) {
		List<ConnItem> list = null;
		_lock.lock();
		try {
			list = _idleConnItemMap.remove(addr);
			if (list == null) {
				list = new ArrayList<ConnItem>();
			}
			CountInfo obj = _countInfoMap.remove(addr);
			if (obj != null)
				_countInfoSet.remove(obj);
		} finally {
			_lock.unlock();
		}
		return list;
	}
	
	private void _restoreIdleConnItemInfo(String addr) {
		addAddress(addr);
	}
	
}
