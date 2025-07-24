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

   Source File Name = UsedConnectionPool.java

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
import java.util.List;
import java.util.Map;

import java.util.concurrent.locks.ReentrantLock;
import com.sequoiadb.base.Sequoiadb;

class UsedConnectionPool implements IConnectionPool{

	private HashMap<Sequoiadb, ConnItem> _conns = new HashMap<Sequoiadb, ConnItem>();
	private ReentrantLock _lockForConns = new ReentrantLock();
	
	class UsedPairIterator implements Iterator<Pair> {
		Iterator<Map.Entry<Sequoiadb, ConnItem>> _entries ;
		
		public UsedPairIterator(Iterator<Map.Entry<Sequoiadb, ConnItem>> entries) {
			_entries = entries;
		}
		
		@Override
		public boolean hasNext() {
			return _entries.hasNext();
		}
	
		@Override
		public Pair next() {
			Map.Entry<Sequoiadb, ConnItem> entry = _entries.next();
			return new Pair(entry.getValue(), entry.getKey());
		}
	
		@Override
		public void remove() {
			return;
		}
	}
	
	/**
	 * @fn Sequoiadb poll(ConnItem item)
	 * @brief Poll a connection out from the pool according to the offered ConnItem.
	 * @return a connection or null for no connection in that ConnItem
	 * @exception 
	 */
	@Override
	public Sequoiadb poll(ConnItem item) {
		return null;
	}

	@Override
	public ConnItem poll(Sequoiadb sdb) {
		_lockForConns.lock();
		try {
			return _conns.remove(sdb);
		} finally {
			_lockForConns.unlock();
		}
	}
	
	/**
	 * @fn void insert(ConnItem pos, Sequoiadb sdb)
	 * @brief Insert a connection into the pool.
	 * @return void.
	 * @exception 
	 */
	@Override
	public void insert(ConnItem item, Sequoiadb sdb) {
		_lockForConns.lock();
		try {
			_conns.put(sdb, item);
		} finally {
			_lockForConns.unlock();
		}
	}

	/**
	 * @fn Iterator<ConnItem> getConnItemIterator()
	 * @brief Return a iterator for the item of the items of the idle connections.
	 * @return the iterator
	 */
	@Override
	public Iterator<Pair> getIterator() {
		return new UsedPairIterator(_conns.entrySet().iterator());
	}
	
	/**
	 * @fn int count()
	 * @brief Return the count of idle connections in idle container.
	 * @return the count of idle connections
	 */
	@Override
	public int count() {
		_lockForConns.lock();
		try {
			return _conns.size();
		} finally {
			_lockForConns.unlock();
		}
	}

	@Override
	public boolean contains(Sequoiadb sdb) {
		_lockForConns.lock();
		try {
			return _conns.containsKey(sdb);
		} finally {
			_lockForConns.unlock();
		}
	}

	@Override
	public List<ConnItem> clear() {
		List<ConnItem> list = new ArrayList<ConnItem>();
		_lockForConns.lock();
		try {
			for( ConnItem item : _conns.values() ) {
				list.add(item);
			}
			_conns.clear();
			return list;
		} finally {
			_lockForConns.unlock();
		}
	}
	
}
