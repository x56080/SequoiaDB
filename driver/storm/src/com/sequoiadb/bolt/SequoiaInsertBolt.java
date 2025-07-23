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

   Source File Name = SequoiaInsertBolt.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.bolt;

import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;

import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.core.StormSequoiaObjectGrabber;

import backtype.storm.task.OutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.tuple.Tuple;

public class SequoiaInsertBolt extends SequoiaBoltBase {
	//private static Logger LOG = Logger.getLogger(SequoiaInsertBolt.class);

	private LinkedBlockingQueue<Tuple> queue = new LinkedBlockingQueue<Tuple>(
			1024);
	private SequoiaBoltTask task;
	private Thread writeThread;

	public SequoiaInsertBolt(String host, int port, String userName,
			String password, String dbName, String collectionName,
			StormSequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, collectionName, mapper);
	}

	@SuppressWarnings("serial")
	@Override
	public void prepare(Map map, TopologyContext topologyContext,
			OutputCollector outputCollector) {
		task = new SequoiaBoltTask (queue, this.sdb, this.space, this.collection, this.mapper) {
			@Override
			public void execute(Tuple tuple) {
				//Build a basic object
				BSONObject object = new BasicBSONObject();
				//Map and save the object;
				collection.insert(mapper.map(object, tuple));
			}
		};
		
		writeThread = new Thread(task);
		writeThread.start();
	}

	@Override
	public void execute(Tuple tuple) {
		queue.add(tuple);
		
		//Execute after insert action
		afterExecuteTuple(tuple);
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer) {
	}

	@Override
	public void afterExecuteTuple(Tuple tuple) {
		//No thing to do
	}

	@Override
	public void cleanup() {
		this.task.stopThread();
		this.sdb.disconnect();

	}

}
