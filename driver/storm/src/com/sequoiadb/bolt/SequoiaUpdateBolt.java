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

   Source File Name = SequoiaUpdateBolt.java

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
//import org.apache.log4j.Logger;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import backtype.storm.task.OutputCollector;
import backtype.storm.task.TopologyContext;
import backtype.storm.topology.OutputFieldsDeclarer;
import backtype.storm.tuple.Tuple;
import com.sequoiadb.core.StormSequoiaObjectGrabber;
import com.sequoiadb.core.UpdateQueryCreator;


public class SequoiaUpdateBolt extends SequoiaBoltBase {
	//private static Logger LOG = Logger.getLogger(SequoiaUpdateBolt.class);
	private static final long serialVersionUID = -3179653776895938041L;
	private UpdateQueryCreator  updateQueryCreator;
	private LinkedBlockingQueue<Tuple> queue = new LinkedBlockingQueue<Tuple>(1024);
	private SequoiaBoltTask task;
	private Thread writeThread;
	
	public SequoiaUpdateBolt(String host, int port, String userName,
			String password, String dbName, String collectionName,
			UpdateQueryCreator updateQueryCreator, StormSequoiaObjectGrabber mapper) {
		super(host, port, userName, password, dbName, collectionName, mapper);
		this.updateQueryCreator = updateQueryCreator;
	}
	
	@SuppressWarnings("serial")
	@Override
	public void prepare(Map map, TopologyContext topologyContext, OutputCollector outputCollector) {
		super.prepare(map, topologyContext, outputCollector);
		
		task = new SequoiaBoltTask(queue, sdb, space, collection, updateQueryCreator, mapper) {
			@Override
			public void execute(Tuple tuple) {
				//Unpack the query
				BSONObject updateQuery = this.updateQueryCreator.createQuery(tuple);
				BSONObject mappedUpdateObject = new BasicBSONObject();
				mappedUpdateObject = this.mapper.map(mappedUpdateObject, tuple);
				//create the update statement
				collection.upsert(updateQuery, mappedUpdateObject, null);
			}
		};
		
		
		writeThread = new Thread(task);
		writeThread.start();
		
	}

	@Override
	public void execute(Tuple tuple) {
		queue.add(tuple);
		this.afterExecuteTuple(tuple);
	}

	@Override
	public void declareOutputFields(OutputFieldsDeclarer declarer) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void afterExecuteTuple(Tuple tuple) {
	}

	@Override
	public void cleanup() {
		task.stopThread();
		sdb.disconnect();
	}

}
