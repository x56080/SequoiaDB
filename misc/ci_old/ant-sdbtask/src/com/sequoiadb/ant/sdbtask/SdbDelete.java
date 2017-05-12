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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * @author qiushanggao
 *
 */
public class SdbDelete extends Task {

	private String uuid = null;
	private String CSName = null;
	private String CLName = null;
	private boolean failonerror = true;
	private JsonElement matcher = null;
	
	
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
		
	public void setFailonerror(String value)
	{
		failonerror = Boolean.parseBoolean(value);
	}
	
	public JsonElement createMatcher()
	{
		if (matcher != null)
		{
			throw new BuildException("Error: cannt set more than one record.");
		}
		
		matcher = new JsonElement();
		return matcher;
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
			Sequoiadb sdb = (Sequoiadb) obj;
			CollectionSpace cs = sdb.getCollectionSpace(CSName);
			DBCollection cl= cs.getCollection(CLName);
			
			if (matcher != null)
			{
				cl.delete(matcher.toBSONObj());
			}else {
				cl.delete((BSONObject)null);
			}
					
		}
		catch(BaseException e)
		{
			if (failonerror)
			{
				throw new BuildException(e);
			}
			else
			{
				log("Failed to delete record. exception=" + e);
			}
		}
		
	}

}
