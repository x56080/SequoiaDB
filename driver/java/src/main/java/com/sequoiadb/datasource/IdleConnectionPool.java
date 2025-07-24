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

   Source File Name = IdleConnectionPool.java

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

class IdleConnectionPool implements IConnectionPool{

	private HashMap<ConnItem, Sequoiadb> _conns = new HashMap<ConnItem, Sequoiadb>();
	private ReentrantLock _lockForConns = new ReentrantLock();
	
	
	class IdlePairIterator implements Iterator<Pair> {
		Iterator<Map.Entry<ConnItem, Sequoiadb>> _entries ;
		
		public IdlePairIterator(Iterator<Map.Entry<ConnItem, Sequoiadb>> entries) {
			_entries = entries;
		}
		
		@Override
		public boolean hasNext() {
			return _entries.hasNext();
		}
	
		@Override
		public Pair next() {
			Map.Entry<ConnItem, Sequoiadb> entry = _entries.next();
			return new Pair(entry.getKey(), entry.getValue());
		}
	
		@Override
		public void remove() {
			return;
		}
	}
	
	/**
	 * @fn Sequoiadb poll(ConnItem pos)
	 * @brief Poll a connection out from the pool according to the offered ConnItem.
	 * @return a connection or null for no connection in that ConnItem
	 * @exception 
	 */
	@Override
	public Sequoiadb poll(ConnItem pos) {
		_lockForConns.lock();
		try {
			return _conns.remove(pos);
		} finally {
			_lockForConns.unlock();
		}
	}

	@Override
	public ConnItem poll(Sequoiadb sdb) {
		return null;
	}
	
	/**
	 * @fn void insert(ConnItem pos, Sequoiadb sdb)
	 * @brief Insert a connection into the pool.
	 * @return void.
	 * @exception 
	 */
	@Override
	public void insert(ConnItem pos, Sequoiadb sdb) {
		_lockForConns.lock();
		try {
			_conns.put(pos, sdb);
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
		return new IdlePairIterator(_conns.entrySet().iterator());
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
		return false;
	}

	@Override
	public List<ConnItem> clear() {
		List<ConnItem> list = new ArrayList<ConnItem>();
		_lockForConns.lock();
		try {
			for( ConnItem item : _conns.keySet()) {
				list.add(item);
			}
			_conns.clear();
			return list;
		} finally {
			_lockForConns.unlock();
		}
	}
		
}
