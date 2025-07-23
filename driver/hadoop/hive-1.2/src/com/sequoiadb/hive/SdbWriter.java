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

   Source File Name = SdbWriter.java

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
package com.sequoiadb.hive;


import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.hive.ql.exec.FileSinkOperator.RecordWriter;
import org.apache.hadoop.io.Writable;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.*;

public class SdbWriter implements RecordWriter {
	public static final Log LOG = LogFactory.getLog(SdbWriter.class.getName());
	
	static private int bufferMaxSize = 1024;
	private Sequoiadb sdb = null;
	private DBCollection collection = null;
	private List<BSONObject> objectBuffer = new ArrayList<BSONObject>(
			bufferMaxSize * 2);


	public SdbWriter(String connAddr, String userName, String passwd, String spaceName, String colName,
			int RecoredNum) {
		
		LOG.debug("Entry:SdbWriter");
		
		if (RecoredNum > 0)
		{
			bufferMaxSize = RecoredNum;
		}


		List<String> addrList = ConfigurationUtil.getDBAddrs (connAddr);

		this.sdb = new Sequoiadb (addrList, userName, passwd, null);
		CollectionSpace space = null;
		try{
			if( sdb.isCollectionSpaceExist(spaceName) )
				space = sdb.getCollectionSpace(spaceName);
			else
				space = sdb.createCollectionSpace(spaceName);
		}catch(BaseException e){
			LOG.error(e.getMessage());
		}
		
		try{
			if( space.isCollectionExist(colName) )
				collection = space.getCollection(colName);
			else
				collection = space.createCollection(colName);
		}catch(BaseException e){
			LOG.error(e.getMessage());
		}
	}

	public void close(boolean abort) throws IOException {

		LOG.debug("Enter SdbWriter close");
		// flush buffer.
		if (objectBuffer.size() > 0) {
			collection.bulkInsert(objectBuffer,
					DBCollection.FLG_INSERT_CONTONDUP);
			objectBuffer.clear();
		}

		if (sdb != null)
			sdb.disconnect();
	}

	public void write(Writable w) throws IOException {

		LOG.debug("Enter SdbWriter write");
		BSONWritable t_bsonwriable = (BSONWritable) w ;
		BSONObject dbo = t_bsonwriable.getBson();

		
		if( dbo != null && dbo.toMap().size() != 0){
			objectBuffer.add(dbo);
		}
		
		if (objectBuffer.size() >= bufferMaxSize &&
			collection != null) {
			collection.bulkInsert(objectBuffer,
					DBCollection.FLG_INSERT_CONTONDUP);
			objectBuffer.clear();
		}
	}



	

}
