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

   Source File Name = ConnectionItemMgr.java

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

import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;
import java.util.concurrent.locks.ReentrantLock;


class ConnItem implements Comparable<ConnItem> {
	private String _addr;
	private long _sequenceNumber;
	
	public ConnItem(String addr, long sequenceNumber) {
		_addr = addr;
		_sequenceNumber = sequenceNumber;
	}
	public String getAddr() {
		return _addr;
	}
	public void setAddr(String addr) {
		_addr = addr;
	}
	public long getSequenceNumber() {
		return _sequenceNumber;
	}
	public void setSequenceNumber(long sequenceNumber) {
		_sequenceNumber = sequenceNumber;
	}
	@Override
	public int compareTo(ConnItem other) {
		if (_sequenceNumber != other._sequenceNumber)
			return (int) (_sequenceNumber - other._sequenceNumber);
		else
			return _addr.compareTo(other._addr);
	}
}

public class ConnectionItemMgr {
	private int _capacity;
	private static long _sequenceNumber = -1;
	private TreeSet<ConnItem> _idleItem = null;
	private TreeSet<ConnItem> _usedItem = null;
	private ReentrantLock _lock = new ReentrantLock();
	
	public ConnectionItemMgr(int capacity, List<ConnItem> usedItems) {
		_capacity = capacity;
		_idleItem = new TreeSet<ConnItem>();
		_usedItem = new TreeSet<ConnItem>();
		int initNum = 0;
		if (usedItems != null) {
			Iterator<ConnItem> itr = usedItems.iterator();
			while(itr.hasNext()) {
				_usedItem.add(itr.next());
			}
			initNum = (capacity > _usedItem.size()) ? (capacity - _usedItem.size()) : 0;
		} else {
			initNum = capacity;
		}
		for (int i = 0; i < initNum; i++) {
			// we must give a sequence number to the instance of ConnItem,
			// for _idleItem is TreeSet, it won't save two instances with
			// the same content
			_idleItem.add(new ConnItem("", ++_sequenceNumber));
		}
	}

	public long getCurrentSequenceNumber() {
		_lock.lock();
		try {
			return _sequenceNumber;
		} finally {
			_lock.unlock();
		}
	}
	
	public int getCapacity() {
		_lock.lock();
		try {
			return _capacity;
		} finally {
			_lock.unlock();
		}
	}
	
	public int getIdleItemNum() {
		_lock.lock();
		try {
			return _idleItem.size();
		} finally {
			_lock.unlock();
		}
	}
	
	public int getUsedItemNum() {
		_lock.lock();
		try {
			return _usedItem.size();
		} finally {
			_lock.unlock();
		}
	}
	
	public void resetCapacity(int capacity) {
		_lock.lock();
		try {
			if (_capacity < capacity) {
				// increase items
				for (int i = _capacity; i < capacity; i++) {
					_idleItem.add(new ConnItem("", ++_sequenceNumber));
				}
			} else {
				// decrease items
				int deltaNum = _capacity - capacity;
				while(deltaNum-- != 0) {
					ConnItem connItem = _idleItem.pollFirst();
					if (connItem == null) {
						break;
					}
				}
			}
			_capacity = capacity;
		} finally {
			_lock.unlock();
		}
	}
	
	public ConnItem getItem() {
		ConnItem connItem = null;
		_lock.lock();
		try {
			connItem = _idleItem.pollFirst();
			// reset sequence number
			if (connItem != null) {
				connItem.setSequenceNumber(++_sequenceNumber);
				_usedItem.add(connItem);
			}
		} finally {
			_lock.unlock();
		}
		return connItem;
	}
	
	public void releaseItem(ConnItem item) {
		_lock.lock();
		try {
			_usedItem.remove(item);
			// _usedItem has remove one, so we must use "<" here
			if (_usedItem.size() + _idleItem.size() < _capacity) {
				item.setAddr("");
				_idleItem.add(item);
			}
		} finally {
			_lock.unlock();
		}
	}
}
