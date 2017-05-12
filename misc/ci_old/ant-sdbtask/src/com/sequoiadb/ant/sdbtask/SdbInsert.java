/**
 * 
 */
package com.sequoiadb.ant.sdbtask;

import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.bson.BSONObject;
import com.sequoiadb.ant.datatype.JsonElement;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;


/**
 * @author qiushanggao
 *
 */
public class SdbInsert extends Task {
	
	private String uuid = null;
	private String CSName = null;
	private String CLName = null;
	private boolean failonerror = true;
	private int   insertflag = 0;
	
	private List<JsonElement> lstRecords = new ArrayList<JsonElement>();
	
	public void setSdbhandle(String value)
	{
		uuid = value;
	}
	public void setCsname(String value)
	{
		CSName = value;
	}
	public void setClname(String value)
	{
		CLName = value;
	}
	
	public JsonElement createRecord()
	{
		JsonElement record = new JsonElement();
		lstRecords.add(record);
		return record;
	}
	
	public void setFailonerror(String value)
	{
		failonerror = Boolean.parseBoolean(value);
	}
	
	public void setInsertFlag(String value)
	{
		insertflag = Integer.parseInt(value);
	}
	
	public void execute()
	{
		Object obj = this.getProject().getReference(uuid);
		if (! (obj instanceof Sequoiadb))
		{
			throw new BuildException("The SdbUUID" + uuid + " cannot get Sequoiadb Object.");			
		}
		
		try
		{
			if (lstRecords.size() == 0)
			{
				throw new BuildException("Error: must at least one record for insert.");
			}
			
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace cs = sdb.getCollectionSpace(CSName);
			DBCollection cl= cs.getCollection(CLName);
			
			if (cl == null)
			{
				if (failonerror)
				{
					throw new BuildException("Error: the cl is not exist.");
				}
				else
				{
					log("The cl:" + CLName + " is not exist.");
					return;
				}
			}
			
			if (lstRecords.size() == 1)
			{
				cl.insert(lstRecords.get(0).toBSONObj());
			}
			else
			{
				List<BSONObject> lstBSONObj = new ArrayList<BSONObject>();
				for(JsonElement record : lstRecords)
				{
					lstBSONObj.add(record.toBSONObj());
				}
				
				cl.bulkInsert(lstBSONObj, insertflag);
			}
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				e.printStackTrace();
				throw new BuildException(e);
			}
			else
			{
				log("Failed to insert record. exception=" + e);
			}
		}
		
	}
}
