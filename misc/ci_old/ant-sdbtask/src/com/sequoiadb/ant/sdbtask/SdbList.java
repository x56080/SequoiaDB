/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.bson.BSONObject;

import com.sequoiadb.ant.datatype.JsonElement;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 *
 */
public class SdbList extends Task {

	private String uuid = null;
	private String CSName = null;
	private String CLName = null;
	private JsonElement query = null;
	private long skipRows = 0;
	private long returnRows = 0;

	public void setSdbhandle(String value) {
		uuid = value;
	}

	public void setCsname(String value) {
		CSName = value;
	}

	public void setClname(String value) {
		CLName = value;
	}

		
	public void setSkip(String value){
		skipRows = Long.parseLong(value);
	}
	
	public void setLimit(String value){
		returnRows = Long.parseLong(value);
	}
	
	public JsonElement createQuery()
	{
		if (query != null)
		{
			throw new BuildException("Error: cannt set more than one record.");
		}
		
		query = new JsonElement();
		return query;
	}

	public void execute() {
		Object obj = this.getProject().getReference(uuid);
		if (!(obj instanceof Sequoiadb)) {
			throw new BuildException("The SdbUUID" + uuid
					+ " cannot get Sequoiadb Object.");
		}

		DBCursor cursor = null;
		try {
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace cs = sdb.getCollectionSpace(CSName);
			DBCollection cl = cs.getCollection(CLName);

			BSONObject queryobj = null;
			if (query != null)
			{
				queryobj = query.toBSONObj();
			}
			cursor = cl.query(queryobj, null, null, null, skipRows, returnRows);
			
			log("list collection:" + cl.getFullName() + " with query:" + query );
			while(cursor != null && cursor.hasNext())
			{
				BSONObject record = cursor.getNext();
				log(record.toString());
			}
			
			
		} catch (BaseException e) {
			throw new BuildException(e);
		} finally {
			if (cursor != null)
			{
				cursor.close();
			}
		}
	}

}
