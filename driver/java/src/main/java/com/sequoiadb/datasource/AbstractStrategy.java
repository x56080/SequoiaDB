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

   Source File Name = AbstractStrategy.java

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
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


abstract class AbstractStrategy implements IConnectStrategy{
    
	protected LinkedList<ConnItem> _idleConnItemList = new LinkedList<ConnItem>();
	protected ArrayList<String> _addrs = new ArrayList<String>();
	protected Lock _lockForConnItemList = new ReentrantLock();
	protected Lock _lockForAddr = new ReentrantLock();
	
	
	@Override
	public void init(List<String> addresses, List<Pair> _idleConnPairs, List<Pair> _usedConnPairs) {
		// Notice that, we won't depend on the address in used queue, for
		// some addresses may have been removed, but, they may be still in used pool.
		
		// get addresses to local
		Iterator<String> itr1 = addresses.iterator();
		while(itr1.hasNext()) {
			String addr = itr1.next();
			if (!_addrs.contains(addr)) {
				_addrs.add(addr);	
			}
		}
		// get idle connections information
		if (_idleConnPairs != null) {
			Iterator<Pair> itr2 = _idleConnPairs.iterator();
			while(itr2.hasNext()) {
				Pair pair = itr2.next();
				String addr = pair.first().getAddr();
				_idleConnItemList.add(pair.first());
				if (!_addrs.contains(addr)) {
					_addrs.add(addr);
				}
			}
		}
	}

	@Override
	public abstract String getAddress();
	
	
	@Override
	public ConnItem pollConnItem(Operation opt) {
		_lockForConnItemList.lock();
		try {
			return _idleConnItemList.poll();
		} finally {
			_lockForConnItemList.unlock();
		}
	}
	
	@Override
	public void addAddress(String addr) {
		_lockForAddr.lock();
		try {
			if(!_addrs.contains(addr)) {
				_addrs.add(addr);
			}
		} finally {
			_lockForAddr.unlock();
		}
	}

	@Override
	public List<ConnItem> removeAddress(String addr) {
		List<ConnItem> list = new ArrayList<ConnItem>();
		_lockForAddr.lock();
		try {
			if (_addrs.contains(addr)) {
				_addrs.remove(addr);
			}
		} finally {
			_lockForAddr.unlock();
		}
		_lockForConnItemList.lock();
		try {
			// Prepare the return ConnItem.
			// We will remove the returning positions.
			Iterator<ConnItem> itr = _idleConnItemList.iterator();
			while(itr.hasNext()) {
				ConnItem item = itr.next();
				if (addr == item.getAddr()) {
					list.add(item);
					itr.remove();//TODO:test it
				}
			}
		} finally {
			_lockForConnItemList.unlock();
		}
		return list;
	}

	@Override
	public void update(ItemStatus type, ConnItem item, int change) {
		_lockForConnItemList.lock();
		try {
			if (ItemStatus.IDLE == type && change > 0) {
				_idleConnItemList.add(item);
			}
		} finally {
			_lockForConnItemList.unlock();
		}
		return;
	}

}
