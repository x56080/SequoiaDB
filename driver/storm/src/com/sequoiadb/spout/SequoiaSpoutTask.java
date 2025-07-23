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

   Source File Name = SequoiaSpoutTask.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.spout;

import java.io.Serializable;
import java.util.concurrent.Callable;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

import org.apache.log4j.Logger;
import org.bson.BSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class SequoiaSpoutTask implements Callable<Boolean>, Serializable,
		Runnable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1481907266524773498L;
	private static Logger LOG = Logger.getLogger(SequoiaSpoutTask.class);

	private LinkedBlockingQueue<BSONObject> queue;
	private Sequoiadb sdb;
	private CollectionSpace space;
	private DBCollection collection;
	private DBCursor cursor;
	private String[] collectionNames;
	private BSONObject query;
	private BSONObject selector;

	// Keeps the running state
	private AtomicBoolean running = new AtomicBoolean(true);

	/**
	 * @fn SequoiaSpoutTask(LinkedBlockingQueue<BSONObject> queue, String host,
	 *     int port, String userName, String password, String dbName, String[]
	 *     collectionNames, BSONObject query)
	 * @brief Constructor
	 * @param queue
	 *            The queue for push object
	 * @param host
	 *            The host name.
	 * @param port
	 *            The port for sequoiadb's coord node.
	 * @param userName
	 *            The user name for sequoiadb
	 * @param password
	 *            The password for sequoiadb
	 * @param dbName
	 *            The Collection space name for sequoiadb
	 * @param collectionNames
	 *            The collection name list
	 * @param query
	 *            The query condition
	 */
	public SequoiaSpoutTask(LinkedBlockingQueue<BSONObject> queue, String host,
			int port, String userName, String password, String dbName,
			String[] collectionNames, BSONObject query, BSONObject selector) {
		this.queue = queue;
		this.collectionNames = collectionNames;
		this.query = query;
		this.selector = selector;

		initSequoia(host, port, userName, password, dbName);
	}

	/**
	 * @fn initSequoia(String host, int port, String userName, String password,
	 *     String dbName)
	 * @brief connect to sequoiadb
	 * @param host
	 *            The host name.
	 * @param port
	 *            The port for sequoiadb's coord node.
	 * @param userName
	 *            The user name for sequoiadb
	 * @param password
	 *            The password for sequoiadb
	 * @param dbName
	 *            The Collection space name for sequoiadb
	 */
	private void initSequoia(String host, int port, String userName,
			String password, String dbName) {
		try {
			Sequoiadb sdb = new Sequoiadb(host, port, userName, password);
			space = sdb.getCollectionSpace(dbName);
		} catch (BaseException e) {
			LOG.error("SequoiaDB Exception:", e);
			// Die fast
			throw new RuntimeException(e);
		}
	}

	/**
	 * @fn stopThread()
	 * @brief Stop query thread
	 */
	public void stopThread() {
		running.set(false);
	}

	@Override
	public void run() {
		try {
			call();
		} catch (Exception e) {
			LOG.error(e);
		}
	}

	@Override
	public Boolean call() throws Exception {
		try {
			String collectionName = this.collectionNames[0];
			// Set up the collection
			this.collection = space.getCollection(collectionName);
			// provide the query object
			cursor = this.collection.query(query, selector, null, null);

			while (running.get()) {
				if (cursor.hasNext()) {
					BSONObject obj = cursor.getNext();
					if (LOG.isInfoEnabled()) {
						LOG.info("Fetching a new item from Sequoiadb cursor:"
								+ obj.toString());
					}

					queue.put(obj);
				} else {
					// Sleep for 50ms and then wake up
					// For get new insert records.
					// TODO: sequoiadb cursor cann't get the new insert
					// record after
					// cursor reach eof.
					Thread.sleep(50);
				}
			}

		} catch (Exception e) {
			LOG.error("Failed to fetch record from sequoiadb, exception:" + e);
			if (running.get()) {
				throw new RuntimeException(e);
			}
		} finally {
			cursor.close();
			sdb.disconnect();
		}

		return true;
	}
}
